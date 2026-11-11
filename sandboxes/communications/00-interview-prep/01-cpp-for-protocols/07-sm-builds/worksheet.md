# Pack 07 - State machines and fixed-size building blocks

**Time:** 2-4 hours. **Code:** `code/` (stubs in `include/es_sm/`, reference in `solutions/`). C++17 (the `overloaded` visitor and `std::launder` are C++17).
**Tooling:** `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Also run once with `-DENABLE_TSAN=ON` (GCC/Clang on Linux/macOS/WSL2): the `SpscQueue` and integration tests only prove their memory orders under ThreadSanitizer. Every test has a 60 s timeout and all thread loops have deadlines, so a wrong queue fails instead of hanging.
**What you build:** a call state machine (enum + switch, then variant + visit), `RingBuffer<T,N>`, `SpscQueue<T,N>`, `FixedVector<T,N>` - the vocabulary of every embedded radio/gateway code base.

---

## 1. Why this matters

Radio protocols are **state machines**. A push-to-talk call is Idle, then Requesting the floor, then Talking or Listening, then Hang (the "hang time" after a transmission that keeps the channel reserved so a reply needs no new set-up). Every message from the air or the network means different things in different states: a "floor granted" is meaningful while *Requesting* and an error anywhere else.

Real consequences of getting this wrong:

- A gateway tracks the call with three booleans (`requesting`, `talking`, `listening`). Eight combinations exist; five states are legal; a race produces `talking && listening` and nobody can say what that means (§3.1).
- A new state `Emergency` is added; one `switch` has a `default:` branch that silently treats it as Idle (§3.1, Q14).
- A ring buffer between the radio driver and the protocol thread uses `% 6`; after 4 billion frames (or 256, with 8-bit counters on a small MCU) it silently skips two slots (§3.3).
- A container that heap-allocates is fine on a Linux gateway and forbidden in the radio interrupt/real-time path (pack 06 §3.5): you need *fixed capacity, no heap*, and sometimes element types that are not default-constructible (§3.5).

This pack builds those pieces from first principles and connects them: radio thread -> `SpscQueue<Msg>` -> protocol thread -> `CallMachine`.

> **Safety:** all call events, ids and payloads here are synthetic; never connect these exercises to live public-safety traffic. Timing figures for hang time and floor control are system-configured; **VERIFY** against the actual standard/system before using any number.

---

## 2. Prerequisites check

- **P1.** Why is `uint8_t{3} - uint8_t{250}` an `int` with value -247, and what do you get by casting it back to `uint8_t`? *(Pack 02 `../02-types-basics/` §3.1-3.2.)*
- **P2.** What does `enum class` give you over a plain `enum`? *(Pack 02 §3.5: scoped names, no implicit conversion to int.)*
- **P3.** How do you test "is a power of two" and what is `x & (x - 1)`? *(Pack 05 `../05-ub-layout-bit/` §3.4 `is_pow2`.)*
- **P4.** What do `memory_order_release` and `memory_order_acquire` promise, and what does "a plain write followed by a release store" guarantee to the reader? *(Pack 06 `../06-concurrency-rt/` §3.4.)*
- **P5.** What is the difference between *allocating storage* and *creating an object*? What happens when a `std::string` is destroyed? *(Pack 03 `../03-lifetime-move/` §3.1.)*

---

## 3. Concept build-up

### 3.1 State machines with `enum class` and `switch`

**Concrete first.** The call, as a table (`code/include/es_sm/call_state.hpp`):

| state | event | next state | action |
|---|---|---|---|
| Idle | PttPress | Requesting | SendFloorRequest |
| Idle | RemoteTalkStart | Listening | StartRx |
| Requesting | FloorGranted | Talking | StartTx |
| Requesting | FloorDenied | Idle | PlayBusyTone |
| Requesting | PttRelease | Idle | CancelRequest |
| Talking | PttRelease | Hang | StopTxStartHang |
| Listening | RemoteTalkEnd | Hang | StopRxStartHang |
| Hang | PttPress | Requesting | SendFloorRequest |
| Hang | RemoteTalkStart | Listening | StartRx |
| Hang | HangTimeout | Idle | ReleaseChannel |

All 25 other (state, event) pairs are *ignored*: no state change, no action. Why does each row exist? *Hang* exists because after a talker releases PTT, someone usually replies: keeping the channel reserved for a short period avoids a full call set-up per sentence (value is configured per system - **VERIFY**). *Requesting* exists because granting the floor takes a round trip to the infrastructure; until the answer arrives we must not transmit. *PTT while Listening is ignored* because the floor is busy (a real system plays a busy tone; here, ignored).

**Derive why a state machine, not booleans.** The "state" of a call is a summary of *history*: what has happened so far that still matters. Try booleans `requesting`, `talking`, `listening` plus `hang`: four booleans have 2^4 = 16 combinations; only 5 are legal (including "all false" = Idle). Eleven representable-but-meaningless combinations (e.g. `talking && listening`) are bugs waiting for a race or a forgotten `= false`. One `enum class CallState` with 5 enumerators can hold *only* legal values. **Make illegal states unrepresentable** is the first design rule.

**Derive the structure.**
1. **Pure transition function** `step(state, event) -> {next, action, handled}`: no hidden data, no I/O. Pure functions are trivial to test *exhaustively*: 5 x 7 = 35 pairs, a loop (see `AllTransitionsExhaustive`).
2. **A thin stateful wrapper** (`CallMachine`) stores the current state, calls `step`, counts ignored events, and returns the *action* for the caller to perform. The machine decides; the caller does the work (sends the packet, starts the timer). That separation is what makes the logic testable without radios.
3. **Actions are data** (`enum class Action`), not callbacks fired from inside the table: the machine is deterministic and has no side effects to mock.

**`switch` on an `enum class` and the compiler's help.** Write the outer `switch (state)` **without a `default:`**. With `-Wall` (`-Wswitch`) the compiler then warns *"enumeration value 'Emergency' not handled"* the day someone adds a state. A `default:` branch silences exactly the warning you want (Q14). Inner `switch (event)` can use `default: break;` meaning "every other event is ignored in this state", since *ignoring* is the protocol rule - a test (`AllTransitionsExhaustive`) then counts handled transitions to catch a forgotten one.

**`to_string` for logs**: also a switch with no default, plus a `return "?"` after it for out-of-range values (a corrupted byte cast to the enum is *legal* to hold, just not a named value).

**Table-driven alternative.** `Transition table[5][7]` indexed by `(state, event)` is compact and data-driven (and could be generated from a spec), at the cost of losing the compiler's exhaustiveness warnings and of needing a careful enum-to-index mapping. The `switch` form is easier to read and to review line by line against a protocol spec; the table form scales to hundreds of transitions. Q7 compares sizes.

**Properties worth testing.** (a) *Reachability*: every state can be reached from Idle (no dead code). (b) *No trap states*: from every state there is a path back to Idle (the radio can't get stuck in a call forever). (c) *Safety*: Talking is only entered via Requesting + FloorGranted (a radio can't start transmitting without being granted the floor!). The tests do (a)-(c) by walking the graph.

> **Common misconceptions**
> - *"A state machine is just a switch."* The point is the **table of legal behaviour**, testable on its own and checkable against the standard.
> - *"Ignoring an unexpected event is a bug."* Often it is the specified behaviour (a late `FloorGranted` after we cancelled). Make it explicit and *count* it - a spike in ignored events is a diagnostic.
> - *"`default:` makes the switch safe."* It makes it *silent*.
> - *"Put the actions in the transition code."* Then testing needs a radio. Return actions as data.
> - *"More booleans are simpler."* Combinations grow as 2^n.

### 3.2 `std::variant` and `std::visit`: messages that carry data

**Concrete first.** `FloorGranted` carries a talker id; `FloorDenied` a reason code; `PttPress` carries nothing. A plain `Event` enum cannot hold those. The classic C fix is a tagged union:

```c
struct Message { enum Kind { Granted, Denied } kind; union { uint32_t talker; uint8_t reason; } u; };
```

Nothing stops you reading `u.talker` when `kind == Denied`: you read bytes that were never written as a `uint32_t` (and in C++ reading the inactive union member is UB, pack 05 §3.3). The tag and the union are two things the programmer must keep in sync.

**What `std::variant` is.** The same tagged union, but with the tag and the storage inside one type that guarantees they stay in sync: `std::variant<PttPress, PttRelease, FloorGranted, ...>` holds *exactly one* alternative at a time; its size is the largest alternative plus the index (compute it: Q6); copy/move/destroy all act on the active alternative. If every alternative is trivially copyable, so is the variant - that is why it can travel through the `SpscQueue` by plain copy.

**Getting at the value** (the tools, from safest to least safe):
- `std::holds_alternative<T>(v)`: boolean test.
- `std::get_if<T>(&v)`: pointer or `nullptr`. No exception - fine on hot paths.
- `std::get<T>(v)`: the value, or **throws `std::bad_variant_access`** if the active alternative differs. Exceptions are forbidden on real-time paths (pack 06 §3.5), and in `-fno-exceptions` builds it aborts.
- `std::visit(visitor, v)`: calls the visitor overload matching the active alternative. **The compiler checks that the visitor can accept every alternative**, so adding a message type without handling it is a *compile error*. This is the variant's equivalent of `-Wswitch`.

**The `overloaded` trick, derived.** A lambda is an object of an unnamed class with one `operator()`. A class can inherit from several lambdas; `using Ts::operator()...;` brings all their `operator()` into one overload set; the deduction guide lets us write `overloaded{lambda1, lambda2}` without naming the types:

```cpp
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::visit(overloaded{
    [](const FloorGranted& g) { return g.talker_id; },
    [](const auto&)           { return 0u; }          // catch-all: compiles, but LOSES exhaustiveness
}, msg);
```

All lambdas must return the same type. **Avoid catch-all `const auto&` lambdas where you want the compile-time check**: they accept the new type silently, exactly like `default:`.

**Enum + switch or variant + visit?** *Enum*: no payload, hot-path, protocol-state values (`CallState`, `Event`). *Variant*: messages with different payloads. The solution uses both: `to_event(Msg)` maps a message to the payload-free `Event`, `step()` decides, and the machine picks the payload (the talker id) out with `get_if`.

**Caveat (taken on trust):** a variant can become *valueless by exception* if a converting assignment throws midway; it never occurs with trivially copyable alternatives like ours.

> **Common misconceptions**
> - *"`variant` is a `union` with extra steps."* The extra step is safety: the type tracks which member is alive.
> - *"`std::get` is the normal way."* It throws; prefer `visit` or `get_if`.
> - *"A generic lambda catches everything, which is convenient."* It also swallows the new message type you forgot.
> - *"A default-constructed variant is empty."* It holds a default-constructed *first* alternative (`PttPress`).

### 3.3 Ring buffers and why a power-of-two size allows a mask

**Concrete first.** A radio driver receives frames in bursts; the protocol thread consumes them in a steady stream. A **ring buffer** is a fixed array used circularly: writes go at the *head*, reads come from the *tail*, and both wrap from the last slot back to slot 0. No heap, no moving elements.

**Naive design:** store `head` and `tail` as slot numbers 0..N-1. Wrapping: `head = (head + 1) % N`. Problem: `head == tail` means both *empty* and *full*; the usual fix wastes one slot or adds a flag.

**Better design - free-running counters.** `head` = total elements ever pushed; `tail` = total popped. Never reduce them. Then
- `size = head - tail`; `empty` when 0; `full` when `size == N` (no wasted slot, no flag);
- the slot used is `counter mod N`.

**Derive why a mask works.** Take N = 8 (`0b1000`). Write a counter in binary: `13 = 0b1101`. Dividing by 8 = shifting right by 3; the *remainder* is the three bits shifted out: `0b101 = 5`. And `13 & 0b111 = 0b101 = 5`. In general, for N = 2^k, `x mod N` is the low k bits of x, which `x & (N - 1)` extracts: `N - 1` is k ones. One AND instruction instead of a division. (Honest note: with a *compile-time constant* power-of-two N and unsigned x, compilers turn `%` into `&` themselves. The mask spelling matters for runtime-sized buffers and, more importantly, for the next point.)

**The real reason: wrap-around correctness.** A counter has finite width, e.g. 8 bits: after 255 comes 0. Slots visited by consecutive counter values must stay consecutive across that wrap:

| counter | `% 6` (N = 6) | `& 7` (N = 8) |
|---|---|---|
| 254 | 2 | 6 |
| 255 | 3 | 7 |
| 0 (wrapped) | **0** | 0 |

With N = 6 the ring jumps from slot 3 to slot 0 at the wrap: slots 4 and 5 are *skipped* (and elements overwritten/lost). With N = 8 it goes 6, 7, 0: perfect. Reason: the counter wraps at 2^bits; slot numbering stays continuous only if the wrap point is a multiple of N, which holds exactly when N is a power of two. (With `size_t` counters the wrap takes 2^64 operations - never in practice - but on a small MCU with `uint16_t` counters it happens every 65,536 frames. `RingBuffer<T,N,Index>` lets the tests use `uint8_t` so the wrap happens after 256 pushes. Test `RingDemo...` prints the table above.)

**Unsigned subtraction handles the wrap.** `size = head - tail` is computed modulo 2^bits, so it is right even after `head` wrapped and `tail` has not: head = 3, tail = 250 (8-bit) -> (3 - 250) mod 256 = 9 elements. **Trap (pack 02):** for `uint8_t`, `head - tail` is done in `int` after integer promotion = -247. You must cast back to the counter type (`static_cast<Index>(head - tail)`) to get 9; returned as `size_t` unchanged it becomes a gigantic number (Q15).

**Overwrite mode.** For diagnostic trails ("last 8 transitions") `push_overwrite` drops the oldest when full: `if (full) ++tail`.

**Failure if ignored.** Non-power-of-two with free-running counters: silent data loss at the wrap, found by nobody because it happens after hours. (Alternatively with `% N` and *reduced* indices it works for any N - but then you lose the full/empty-by-subtraction property and the mask.)

> **Common misconceptions**
> - *"Power of two is only an optimisation."* It is a *correctness* condition for free-running counters.
> - *"Wrap-around never happens: 2^64 is huge."* With `uint8_t`/`uint16_t` counters on small targets it happens constantly.
> - *"`head == tail` means empty, so use it for full too."* Free-running counters distinguish them by `size == N`.
> - *"`head - tail` is always right."* Not for narrow types without the cast.
> - *"Capacity is N - 1."* Only in the reduced-index design; here it is N.

### 3.4 `SpscQueue`: the ring buffer made lock-free

**Concrete first.** Radio thread produces frames, protocol thread consumes them. A mutex would work (pack 06 `BoundedQueue`) but locks are banned on the radio's real-time path. With **exactly one producer and one consumer** we can use the ring buffer with two atomics and no lock.

**The design principle: every shared variable has exactly one writer.** `head_` is written only by the producer; `tail_` only by the consumer. Each thread may read *its own* index with `relaxed` (nobody else changes it) and the *other's* index with `acquire`.

```
try_push (producer)                      try_pop (consumer)
h = head_.load(relaxed)   // mine        t = tail_.load(relaxed)   // mine
t = tail_.load(acquire)   // theirs      h = head_.load(acquire)   // theirs
if (h - t == N) return false  // full    if (h == t) return false      // empty
buf[h & mask] = value     // plain write  out = buf[t & mask]          // plain read
head_.store(h + 1, release)  // publish   tail_.store(t + 1, release)   // hand back
```

**Why each order (derive from pack 06 §3.4):**
- Producer's `release` store of `head_` publishes the element write: a consumer whose `acquire` load sees the new `head_` is guaranteed to see the element contents (not stale). Without it the consumer could read `buf[t]` before the data arrived (§3.4 failure table).
- Consumer's `release` store of `tail_` + producer's `acquire` load: the producer sees the slot is free only *after* the consumer has finished reading it, so it cannot overwrite data still being read.
- Own-index loads are `relaxed`: a thread always sees its own latest write (program order).

**Why a stale value is safe.** The producer may see an *old* `tail_` (consumer already advanced): it believes the queue is fuller than it is, and returns false ("full") spuriously - conservative, never unsafe. Similarly for the consumer's view of `head_`. Safety needs "never believe *more* room/data than exists", and staleness only errs the other way.

**Why exactly one producer and one consumer.** With two producers both could read the same `head_`, write the *same slot*, and both store `h + 1`: a data race and a lost element. Multiple producers need a CAS on the head (MPSC queue) - more complex; out of scope.

**False sharing - a performance rule.** CPUs move memory between cores in cache lines (typically 64 bytes - **VERIFY** for your target). If `head_` and `tail_` live in the same line, each store by one thread invalidates the other thread's copy even though they touch different variables, and the line ping-pongs between cores on *every* operation. `alignas(64)` puts `head_`, `tail_`, and the buffer on separate lines. (Test `IndicesLiveOnSeparateCacheLines` checks the layout.)

**Properties.** Wait-free (no loops, no locks), O(1), no heap, `try_*` functions never block - the caller decides what to do when full (drop, count, retry later). `size_approx()` loads `tail_` *before* `head_` so the unsigned difference cannot underflow, then clamps to N (it may be momentarily stale).

> **Common misconceptions**
> - *"Atomic indices make the queue thread-safe."* Only with the right memory orders on the *element hand-over*, and only with one producer/one consumer.
> - *"relaxed is fine, it's just an index."* Then the element data is not published: the consumer may read an unwritten slot (found by TSan, not by x86 hardware).
> - *"A full queue should block."* On a real-time path blocking is exactly what is forbidden: return false and decide.
> - *"Padding is wasted memory."* 128 bytes of padding buys not fighting over cache lines.
> - *"MPMC is just SPSC twice."* No.

### 3.5 `FixedVector`: placement new, explicit destructors, and `std::launder`

**Concrete first.** We need "a vector with capacity 16, no heap" for, say, the list of participants in a call. Try `std::array<Participant, 16>`: it *constructs all 16* up front (needs a default constructor, runs 16 constructors even if only 3 are used, and 13 destructors run at the end for objects that never meant anything). We want to construct only on demand.

**Derive the idea.** Pack 03: *storage* and *object lifetime* are different things. Allocating bytes does not create an object; an object begins when a constructor completes and ends when the destructor starts. So `FixedVector` keeps **raw bytes**:

```cpp
alignas(T) std::byte storage_[sizeof(T) * N];   // raw, uninitialised, correctly aligned for T
std::size_t size_ = 0;                          // the first size_ slots hold live objects
```

and manages lifetimes by hand:

1. **Create** with *placement new*: `::new (address) T(args...)` runs the constructor at the given address and allocates nothing.
2. **Destroy** with an explicit destructor call: `p->~T();` runs the destructor and frees nothing (the bytes stay).
3. **Align**: `alignas(T)` on the byte array; slot `i` is at `storage_ + i * sizeof(T)`, which is aligned because `sizeof(T)` is a multiple of `alignof(T)` (pack 05 §3.4). Without `alignas`, a `double` or `alignas(16)` type could sit on a misaligned address (UB, slow, or a fault on some CPUs).
4. **Access** the element through a pointer obtained from placement new, or via `std::launder(reinterpret_cast<T*>(...))`. Intuition for `launder`: the compiler tracks which object lives where; a pointer cast out of a *byte array* does not formally point at the `T` created inside it, and the compiler may assume things about the old byte object. `launder` says "an object of type T really lives here now - forget what you assumed". (The precise rules: taken on trust; follow the pattern.)

**What goes wrong without it - failure first.** If you wrote `T* p = reinterpret_cast<T*>(storage_); p[size_] = v;`, you call `operator=` on an object that **does not exist**. For `std::string` that `operator=` first frees the old buffer - which is garbage bytes - and crashes or corrupts memory. Pushing never constructs; `pop_back` without `~T()` leaks (the string's heap block is never freed). Both are caught by `Probe`'s `live()` counter and by ASan.

**Exception safety (the one that is easy to miss).**
- In `emplace_back`, increment `size_` only **after** the constructor succeeded; otherwise a throwing constructor leaves `size_` counting a non-object, and the destructor later destroys garbage.
- In the **copy constructor**, if copying element 3 of 5 throws, the objects 0-2 exist but the destructor of a *half-constructed* `FixedVector` never runs (the object was never fully constructed). So the constructor must `try { ... } catch (...) { clear(); throw; }` itself.
- Destructors and `pop_back` must not throw (`noexcept`).

**Rule of five (pack 03).** We own raw lifetimes, so copy and move must be written: copy copies each live element; move *moves each element* (elements are inline: they cannot be "stolen" like a heap pointer) and then clears the source so a moved-from `FixedVector` is empty. Assignment is `= delete`d here to keep the focus: implementing it safely (self-assignment, partial failure) is an extension exercise.

**Return values instead of exceptions.** `emplace_back` returns `T*` or `nullptr` when full; `push_back` returns `bool`. Full is an expected condition on a fixed-capacity container; the real-time rule says no throwing (pack 06 §3.5).

> **Common misconceptions**
> - *"`reinterpret_cast` the buffer to `T*` and assign."* No object exists yet: UB.
> - *"`new T[N]` is the same."* It default-constructs all N and allocates on the heap.
> - *"The destructor of the container frees the elements' memory."* The memory is part of the container; only the *objects* need destroying - and only the live ones.
> - *"`alignas` is decoration."* Wrong alignment is UB for typed access (pack 05).
> - *"Placement new allocates."* It constructs at a place you chose.

---

## 4. Fully worked example: a frame travels from the radio thread to the call machine

**Context.** The radio thread has recognised `FloorGranted{7}` and `RemoteTalkStart{9}`; the protocol thread owns the `CallMachine`. We use `SpscQueue<Msg, 4>` (N = 4, mask = 3) to keep the numbers small. `sizeof(Msg)` is 8 (Q6).

**Step 1 - the indices (free-running, N = 4, mask `0b11`).** `h` = head_, `t` = tail_ before the operation; the slot is `counter & 3`.

| # | operation | h | t | check | slot | head_ / tail_ after | result |
|---|---|---|---|---|---|---|---|
| 1 | push `PttPress` | 0 | 0 | 0-0 != 4 | 0 | head 1 | true |
| 2 | push `FloorGranted{7}` | 1 | 0 | 1 != 4 | 1 | head 2 | true |
| 3 | push `PttRelease` | 2 | 0 | 2 != 4 | 2 | head 3 | true |
| 4 | pop | 3 | 0 | 3 != 0 | 0 | tail 1 | `PttPress` |
| 5 | push `RemoteTalkStart{9}` | 3 | 1 | 2 != 4 | 3 | head 4 | true |
| 6 | push `RemoteTalkEnd` | 4 | 1 | 3 != 4 | **0** (4 & 3) | head 5 | true |
| 7 | push `HangTimeout` | 5 | 1 | 4 == 4 | - | unchanged | **false** (full) |
| 8 | pop | 5 | 1 | 5 != 1 | 1 | tail 2 | `FloorGranted{7}` |

Step 6 is the wrap: counter 4 uses slot `4 & 3 = 0`, the slot that step 4 freed. The producer knew it was free because `h - t = 4 - 1 = 3`, which is not `N`. At step 7, `h - t = 5 - 1 = 4 == N`, so the queue is full: the *caller* gets `false` and nothing blocks.

**Step 2 - the memory-order story (steps 5 and 8).** The producer wrote slot 3 (plain stores of the `RemoteTalkStart` bytes), then `head_.store(4, release)`. The consumer's `head_.load(acquire)` that returns 4 (or more) *synchronizes-with* that store, so reading slot 3 later sees the full message. The consumer's `tail_.store(2, release)` after reading slot 1 lets the producer's `tail_.load(acquire)` (returning 2) know slot 1 may be rewritten - it will not overwrite bytes the consumer is still reading.

**Step 3 - the machine.** The protocol thread feeds each popped `Msg` to `CallMachine::handle(const Msg&)`: `to_event` (a `std::visit`) turns it into an `Event`; `step` looks up the table; if handled and the message was `FloorGranted`/`RemoteTalkStart`, `get_if` copies the talker id. From Idle: `PttPress` -> Requesting (`SendFloorRequest`), `FloorGranted{7}` -> Talking (`StartTx`, talker = 7), `PttRelease` -> Hang (`StopTxStartHang`), `RemoteTalkStart{9}` -> Listening (`StartRx`, talker = 9), `RemoteTalkEnd` -> Hang, `HangTimeout` -> Idle (`ReleaseChannel`, talker cleared).

**Step 4 - what to log without allocating.** A `RingBuffer<Action, 8>` with `push_overwrite` keeps the last 8 actions for diagnostics; the actions go into a `FixedVector<Action, 16>` per batch. Neither touches the heap (pack 06 §3.5).

---

## 5. Questions

### 5.1 Explain why

**Q1.** *Context: a junior engineer models the call with booleans `requesting`, `talking`, `listening`, `hang`.* How many combinations can the four booleans represent, how many are legal, and why does an `enum class` fix the problem? Give one concrete illegal combination and a race that produces it.

**Q2.** *Context: code review of `switch (state)` with a `default: return "Idle";`.* Explain why the outer switch of a state machine should have no `default`, what warning you gain, and where a `default` *is* acceptable (inner switch over events).

**Q3.** *Context: a ring buffer with free-running 8-bit counters on a small MCU.* Derive why `x & (N - 1)` equals `x % N` when N is a power of two, and show with N = 6 vs N = 8 what breaks at counter wrap-around.

**Q4.** *Context: the radio thread pushes frames into an `SpscQueue`.* Which exact stale read would make a *relaxed* `head_` publication wrong, and why is a stale `tail_` seen by the producer harmless?

### 5.2 Calculate

**Q5.** *Context: reading a ring from a debugger.* (a) For N = 16, compute the slot (`counter & 15`) of counters 37, 255, 16 and 4294967295 + 1 (uint32 wrap). (b) With `uint8_t` counters, N = 32, `head = 3`, `tail = 250`: how many elements are stored, which slots do they occupy (in order), and which slot is written next?

**Q6.** *Context: sizing the RX queue.* Compute `sizeof(Msg)` (alternatives listed in `msg.hpp`: five tiny structs, two with a `uint32_t`, one with a `uint8_t`; the variant index fits in one byte), then the sizes of `SpscQueue<Msg,16>` (cache line 64) and `RingBuffer<Msg,16>` (default `size_t` indices). Show the layout.

**Q7.** *Context: choosing between the `switch` and a table.* The table version stores `Transition{CallState, Action, bool}` (3 bytes) for every pair. (a) Table size for 5 states x 7 events. (b) What fraction are real transitions? (c) How many pairs must be reviewed if the spec adds the state `Emergency` and the event `Preempt`?

### 5.3 Predict

**Q8.** *Context: replaying a log.* Starting in Idle, apply: `RemoteTalkStart`, `PttPress`, `RemoteTalkEnd`, `PttPress`, `FloorDenied`, `FloorGranted`, `PttRelease`. Give the state and the `Action` after each event, and the final `ignored()` count.

**Q9.** *Context: a handler written by someone who likes `std::get`.*

```cpp
es::sm::Msg m = es::sm::PttPress{};
auto& g = std::get<es::sm::FloorGranted>(m);   // what happens?
auto idx = es::sm::Msg{es::sm::HangTimeout{}}.index();
const auto* p = std::get_if<es::sm::FloorDenied>(&m);
```

What does each line do? Which would you ban on the real-time path, and why?

**Q10.** *Context: the diagnostic trail.* `RingBuffer<int,4> r;` then `push(1)`, `push(2)`, `push(3)`, `push(4)`, `push(5)`, `pop()`, `push_overwrite(6)`, `push_overwrite(7)`. For each call give the return value; then give the logical contents oldest-to-newest, the physical array contents, `head_` and `tail_`.

### 5.4 Trace by hand

**Q11.** *Context: talker bookkeeping.* From Idle apply the messages `RemoteTalkStart{9}`, `RemoteTalkEnd`, `PttPress`, `FloorGranted{3}`, `HangTimeout`, `PttRelease`, `HangTimeout`. For each, give: handled?, state after, `talker()`, `ignored()`.

**Q12.** *Context: the SPSC index walk.* `SpscQueue<int,4>`: push 1, push 2, push 3, pop, push 4, push 5, push 6, pop, pop. For each operation give `h`, `t`, the check, the slot used, `head_`/`tail_` after, and the result. (Follow the table of §4.)

**Q13.** *Context: ctor/dtor accounting with `Probe` (counts `ctors`, `copies`, `moves`, `dtors`).* `FixedVector<Probe,3> v;` then `v.emplace_back(1)`, `v.emplace_back(2)`, `v.push_back(Probe(3))`, `v.emplace_back(4)`, `v.pop_back()`, `{ FixedVector<Probe,3> w(v); }`, end of scope for `v`. Give the four counters after each step.

### 5.5 Find the bug

**Q14.** *Context: a state is added.*

```cpp
enum class CallState { Idle, Requesting, Talking, Listening, Hang, Emergency };   // Emergency is new

const char* name(CallState s) {
    switch (s) {
        case CallState::Requesting: return "Requesting";
        case CallState::Talking:    return "Talking";
        case CallState::Listening:  return "Listening";
        case CallState::Hang:       return "Hang";
        default:                    return "Idle";
    }
}
```

What is printed for `Emergency`? What does the compiler say if the `default` is removed? Is there another problem in this function for `Idle`?

**Q15.** *Context: a ring buffer for a 16-bit-MCU port.* Find **three** bugs.

```cpp
template <class T, std::size_t N, class Index = std::uint8_t>
class Ring {
    T buf_[N]; Index head_ = 0, tail_ = 0;
public:
    std::size_t size() const { return head_ - tail_; }
    bool push(const T& v) { if (size() == N) return false; buf_[head_ % N] = v; ++head_; return true; }
    bool pop(T& out)      { if (size() == 0) return false; out = buf_[tail_ % N]; ++tail_; return true; }
};
Ring<int, 6> r;   // instantiated with N = 6
```

**Q16.** *Context: an "optimised" SPSC queue.* Find **three** problems.

```cpp
struct Q {
    std::atomic<size_t> head_{0}, tail_{0};          // declared next to each other
    int buf_[8];
    bool push(int v) {
        size_t h = head_.load(std::memory_order_relaxed);
        if (h - tail_.load(std::memory_order_relaxed) == 8) return false;
        buf_[h & 7] = v;
        head_.store(h + 1, std::memory_order_relaxed);
        return true;
    }
    bool pop(int& out) {
        size_t t = tail_.load(std::memory_order_relaxed);
        if (head_.load(std::memory_order_relaxed) == t) return false;
        out = buf_[t & 7];
        tail_.store(t + 1, std::memory_order_relaxed);
        return true;
    }
};
// used by TWO producer threads and one consumer
```

**Q17.** *Context: a first attempt at a fixed-capacity vector.* Find **four** bugs.

```cpp
template <class T, std::size_t N>
class FV {
    unsigned char storage_[sizeof(T) * N];
    std::size_t size_ = 0;
public:
    bool push_back(const T& v) {
        if (size_ == N) return false;
        T* p = reinterpret_cast<T*>(storage_);
        p[size_] = v;               // (1)
        ++size_;
        return true;
    }
    void pop_back() { --size_; }    // (2)  (3) also: what if size_ == 0?
    ~FV() {}                        // (4)
};
```

### 5.6 Implement

(All in `code/`; do **not** open `solutions/` until you have tried. Build with the stubs, watch the tests fail, make them pass.)

**Q18.** `include/es_sm/call_state.hpp` - the three `to_string` functions: `switch` with no default, plus the `"?"` fallback. Tests: `Names.*`.

**Q19.** Same file - `step(CallState, Event)` implementing the 10-row table (outer switch without default, inner switch with `default: break;`). Tests: `Step.*`, `Graph.*`.

**Q20.** `include/es_sm/call_machine.hpp` - `to_event(const Msg&)` and `describe(const Msg&)` with `std::visit` and `overloaded`, no catch-all lambda. Careful: print the `uint8_t` reason as a number. Tests: `Msg.*`.

**Q21.** Same file - `CallMachine::handle(Event)` and `handle(const Msg&)`: apply `step`, count ignored, record/clear the talker id. Tests: `Machine.*`, `MachineMsg.*`.

**Q22.** `include/es_sm/ring_buffer.hpp` - `size`, `push`, `pop`, `clear` with free-running counters and a mask. Mind integer promotion in `size()`. Tests: `Ring.*` (first six).

**Q23.** Same file - `push_overwrite` and `operator[]`; make the `uint8_t`-counter tests (`EightBit...`) pass; read `RingDemo...`.

**Q24.** `include/es_sm/spsc_queue.hpp` - `try_push`, `try_pop`, `size_approx` with the orders of §3.4. Run `Spsc.*` under ASan/UBSan and then TSan; write one sentence per memory order.

**Q25.** `include/es_sm/fixed_vector.hpp` - destructor, `emplace_back` (placement new), `pop_back`, `clear`. Tests: `FixedVectorTest.*` (counters must balance: `live() == 0`).

**Q26.** Same file - copy and move constructors, with exception safety (`try/catch` and `clear()`), moved-from source left empty.

**Q27.** Run `Integration.*`. Then run it under TSan and write down, for the message `FloorGranted{7}`, every happens-before edge between the radio thread writing it and the protocol thread acting on it.

### 5.7 Connect

**Q28.** *(Pack 06 §3.4.)* Compare `Mailbox<T>` (pack 06) and `SpscQueue<T,N>`: what is the common protocol, what does the extra capacity cost, and why does each side read its own index relaxed?

**Q29.** *(Pack 05 §3.4.)* Connect `alignas(T)` in `FixedVector`, `alignas(64)` in `SpscQueue`, and `is_pow2`/`align_up` from pack 05: which are about *correctness*, which about *performance*? Why is `x & (N - 1)` the same trick as `align_down`?

**Q30.** *(Pack 03 §3.3.)* `Buffer` in pack 03 owns a heap block; `FixedVector` owns inline raw storage with live objects. Compare their copy and move constructors, and the state of the moved-from object. Why can't `FixedVector`'s move "steal" like `Buffer`'s?

### 5.8 Interview-style

**Q31.** "Design a queue between a radio interrupt/real-time thread and the protocol thread." Cover capacity, full policy, memory orders, false sharing, and testing.

**Q32.** "When do you use `enum` + `switch` and when `variant` + `visit`? What can the compiler check in each?"

**Q33.** "Why must a ring buffer's size be a power of two? Is it just speed?"

**Q34.** "How would you implement and test a call state machine so you are sure no state is a dead end and no transmit can start without a granted floor?"

---

## 6. Self-assessment

- [ ] I can explain why booleans give 2^n states and an enum gives only legal ones.
- [ ] I write state `switch`es without `default` and know what warning that buys me.
- [ ] I keep `step()` pure and return actions as data; I can test all (state, event) pairs in a loop.
- [ ] I can test reachability, no-trap and safety properties of a state machine by walking its graph.
- [ ] I can use `variant`, `get_if`, `holds_alternative` and `visit` + `overloaded`, and know why `std::get` and catch-all lambdas are risky.
- [ ] I can derive `x % 2^k == x & (2^k - 1)` and show what breaks at counter wrap-around for other sizes.
- [ ] I handle integer promotion for narrow counters (`static_cast<Index>(head - tail)`).
- [ ] I can write an SPSC queue with the right acquire/release orders and justify each, and explain false sharing.
- [ ] I can implement a `FixedVector` with placement new, explicit destructor calls, `alignas` and exception-safe copy.
- [ ] I ran everything under ASan/UBSan **and** TSan.
- [ ] I implemented every stub before reading the solutions.
