# Pack 07 answers - state machines and fixed-size building blocks

Each answer: reasoning first, then **common wrong answers** (why tempting / why wrong). Try the question yourself before reading. Section references are to `worksheet.md`. (Numbers that depend on the standard library/platform are marked VERIFY.)

---

## 5.1 Explain why

### Q1 - booleans vs enum
**Answer.** Four booleans = 2^4 = **16** combinations. Legal ones: Idle (all false), only `requesting`, only `talking`, only `listening`, only `hang` = **5**. **11** representable combinations mean nothing. Concrete illegal state: `talking && listening`. A race that produces it: the RX thread handles `RemoteTalkStart` and sets `listening = true` at the same moment the protocol thread handles `FloorGranted` and sets `talking = true`; each checked the *other* flag first and found it false. (Even without threads: forgetting `requesting = false` when granting leaves `requesting && talking`.) An `enum class CallState` with 5 enumerators can only hold the 5 legal values; every transition is an assignment of one whole state, so there is no half-updated in-between (§3.1: "make illegal states unrepresentable").

**Common wrong answers.**
- *"Booleans are fine if I'm careful."* Tempting for small cases. Wrong: invariants then live in everyone's head; the compiler cannot check them, and the number of combinations doubles with each flag.
- *"An enum has 5 states so it is also 5 bits."* The point is not storage but that *only* legal values exist.

### Q2 - no `default:` on the outer switch
**Answer.** With no `default`, `-Wswitch` (in `-Wall`) warns whenever an enumerator is not handled, so adding a state forces you to update every switch - it turns "forgot to handle the new state" into a build-time message. A `default` branch makes the compiler consider all cases handled and the warning disappears; the new state silently takes the default behaviour. Acceptable `default`: the inner `switch (event)` where the specified behaviour for all other events *is* "ignore"; a test that counts handled transitions (here exactly 10) catches an accidental omission.

**Common wrong answers.**
- *"`default` is defensive programming."* Tempting from "always handle unexpected input". Wrong for closed enums: the right defence is the compiler warning; defensive handling of *corrupted* values is the `return "?"` after the switch.
- *"Without default the switch can fall off the end, which is UB."* Falling off the end of a function returning a value is UB, hence the explicit `return` after the switch for out-of-range values.

### Q3 - masks and wrap-around
**Answer.** For N = 2^k: dividing by N shifts right by k bits; the remainder is exactly the k bits shifted out, i.e. the low k bits. `N - 1` is k ones in binary, so `x & (N - 1)` keeps the low k bits = `x mod N`. Example: `13 = 0b1101`; `13 mod 8 = 5 = 0b101 = 13 & 0b111`. Wrap-around: an 8-bit counter goes 254, 255, 0. N = 8: slots `& 7` = 6, 7, 0 - consecutive. N = 6: `% 6` = 2, 3, 0 - slots 4 and 5 are skipped, because 256 is not a multiple of 6 (256 mod 6 = 4: the sequence of slots is "shifted" by 4 each time the counter wraps). The wrap point (2^bits) must be a multiple of N, true exactly for powers of two. (With `size_t` counters it takes 2^64 operations - practically never - but narrow counters on small targets wrap often.)

**Common wrong answers.**
- *"The mask is just faster than `%`."* Tempting: it is the textbook line. For compile-time constants compilers do that conversion themselves; the real constraint is wrap-around correctness with free-running counters.
- *"`& (N - 1)` works for any N."* For N = 6, `& 5` = `0b101` selects bits 0 and 2: slots 0,1,4,5 only (2 and 3 are unreachable).

### Q4 - which stale read matters
**Answer.** If `head_` is published with `relaxed`, the consumer can read the *new* `head_` while the element write is still invisible (stale slot - e.g. the previous frame's bytes or uninitialised data): it hands a torn or old message to the state machine. The release/acquire pair on `head_` guarantees the element write happens-before the read. A stale `tail_` on the producer side only makes it think the queue is fuller than it is, so `try_push` returns false spuriously - conservative, never unsafe; the next call sees the newer value. (Danger would be the opposite: believing there is *more* room or data than exists.)

**Common wrong answers.**
- *"A stale `tail_` could make the producer overwrite unread data."* Tempting because "stale" sounds dangerous. A stale (older, smaller) `tail_` means *less* free space is believed, not more.
- *"x86 won't reorder, so relaxed is fine."* The compiler may, and the code is a data race on the slot; TSan reports it.

---

## 5.2 Calculate

### Q5 - mask arithmetic
(a) N = 16, mask 15: `37 = 0b100101` -> low 4 bits `0101` = **5**; `255 & 15` = **15**; `16 & 15` = **0**; `4294967295 + 1` wraps to 0 in `uint32_t` -> slot **0** (and `4294967295 & 15 = 15`, so 15 -> 0 stays consecutive because 16 divides 2^32).
(b) `uint8_t`, N = 32: size = (3 - 250) mod 256 = -247 + 256 = **9** elements (the `int` result -247 must be cast to `uint8_t`). Counters 250..255 then 0..2 -> slots `& 31`: **26, 27, 28, 29, 30, 31, 0, 1, 2** (oldest first). Next write: counter 3 -> slot **3**. (Continuous across the wrap since 32 divides 256.)

**Common wrong answers.** *"size = 3 - 250 = -247"* (forgetting the modular arithmetic); *"size = 247"* (taking the absolute value: 247 would exceed N = 32, impossible); *"slots 250..258"* (forgetting the mask).

### Q6 - sizes
`Msg`: the largest alternative is 4 bytes (`uint32_t`, alignment 4); the variant adds a 1-byte index after the storage: 4 + 1 = 5, padded to alignment 4 -> `sizeof(Msg) == 8` (typical libstdc++/MSVC layout; VERIFY with `static_assert` on your target).
`SpscQueue<Msg,16>`: `head_` at offset 0 occupies a full 64-byte line (8 bytes + padding); `tail_` at offset 64; `buf_` aligned to 64 at offset 128, 16 x 8 = 128 bytes -> total **256 bytes**, alignment 64.
`RingBuffer<Msg,16>` (size_t indices): `buf_` 128 bytes at offset 0 (alignment 4), `head_` at 128 (alignment 8), `tail_` at 136 -> **144 bytes**.
The 112 extra bytes in the SPSC queue buy the absence of false sharing.

**Common wrong answers.** *"`sizeof(Msg)` = 4 + 1 = 5"* (forgets padding to the alignment of 4); *"`SpscQueue` = 8 + 8 + 128 = 144"* (that is the `RingBuffer` figure; it ignores the `alignas(64)` padding).

### Q7 - table vs switch
(a) 5 x 7 x 3 = **105 bytes** (assuming no padding in a 3-byte struct of two 1-byte enums and a bool). (b) 10/35 = **28.6%** real transitions (71.4% default "ignored" entries - a sparse table). (c) 6 x 8 = **48** pairs; 13 are new (7 involving Emergency x old events + 5 old states x Preempt + 1 for both); all 48 deserve a decision, and the new 13 *must* be specified (the `switch` form will make the compiler flag the new state in each outer switch; the table form won't).

**Common wrong answers.** *"6 + 8 = 14 new rows"* (additive instead of multiplicative); *"only the new state's 8 pairs"* (forgets old states x the new event).

---

## 5.3 Predict

### Q8 - replay
| event | state after | action | handled |
|---|---|---|---|
| RemoteTalkStart | Listening | StartRx | yes |
| PttPress | Listening | None | **ignored** (floor busy) |
| RemoteTalkEnd | Hang | StopRxStartHang | yes |
| PttPress | Requesting | SendFloorRequest | yes |
| FloorDenied | Idle | PlayBusyTone | yes |
| FloorGranted | Idle | None | **ignored** (no request outstanding) |
| PttRelease | Idle | None | **ignored** |

`ignored() == 3`.

**Common wrong answers.** *"PttPress in Listening starts a request"* (the table says ignored; real systems would signal busy). *"FloorGranted in Idle starts transmitting"* (the safety property: Talking is only reached from Requesting).

### Q9 - `get`, `index`, `get_if`
Line 1: `std::get<FloorGranted>(m)` while `m` holds `PttPress` **throws `std::bad_variant_access`**; uncaught -> `std::terminate`; with `-fno-exceptions` the library aborts. Line 2: `Msg{HangTimeout{}}.index()` is **6** (zero-based: PttPress 0, PttRelease 1, FloorGranted 2, FloorDenied 3, RemoteTalkStart 4, RemoteTalkEnd 5, HangTimeout 6). Line 3: `get_if<FloorDenied>(&m)` returns **`nullptr`**. Ban `std::get` on the real-time path (exceptions, pack 06 §3.5) - use `visit` (compile-time exhaustive) or `get_if` (explicit null check).

**Common wrong answers.** *"`get` returns a default FloorGranted"* or *"UB"*: it is well defined, it throws. *"`index()` is 7"* (off by one).

### Q10 - overwrite ring
Return values: `push(1)` true, `push(2)` true, `push(3)` true, `push(4)` true, `push(5)` **false** (full), `pop()` -> **1**, `push_overwrite(6)` (void; size was 3, so no drop), `push_overwrite(7)` (void; full -> drops 2, stores 7).
Free-running counters: after the four pushes head = 4; `pop` -> tail = 1; `push_overwrite(6)` -> slot `4 & 3 = 0`, head = 5; `push_overwrite(7)`: full, so `++tail` (tail = 2), then slot `5 & 3 = 1`, head = 6.
- logical contents oldest to newest: **3, 4, 6, 7**
- physical array: **[6, 7, 3, 4]**
- `head_ = 6`, `tail_ = 2` (size 4)

**Common wrong answers.** *"5 gets stored after the pop"* (no - the push of 5 happened before the pop and was rejected); *"the oldest is 2"* (2 was dropped by the second overwrite); *"physical array [3,4,6,7]"* (physical order is by slot, not age).

---

## 5.4 Trace by hand

### Q11 - talker bookkeeping
| # | message | handled | state after | `talker()` | `ignored()` |
|---|---|---|---|---|---|
| 1 | RemoteTalkStart{9} | yes | Listening | 9 | 0 |
| 2 | RemoteTalkEnd | yes | Hang | 9 (kept during hang) | 0 |
| 3 | PttPress | yes | Requesting | 9 (carries no id; kept) | 0 |
| 4 | FloorGranted{3} | yes | Talking | 3 | 0 |
| 5 | HangTimeout | **no** (Talking) | Talking | 3 | 1 |
| 6 | PttRelease | yes | Hang | 3 | 1 |
| 7 | HangTimeout | yes | Idle | none (cleared on Idle) | 1 |

**Common wrong answers.** *"Step 3 clears the talker"* (only Idle clears it in this design). *"Step 5 goes Idle"* (HangTimeout is meaningful only in Hang).

### Q12 - SPSC index walk (N = 4, mask 3)
| # | op | h | t | check | slot | head/tail after | result |
|---|---|---|---|---|---|---|---|
| 1 | push 1 | 0 | 0 | 0 != 4 | 0 | 1 / 0 | true |
| 2 | push 2 | 1 | 0 | 1 != 4 | 1 | 2 / 0 | true |
| 3 | push 3 | 2 | 0 | 2 != 4 | 2 | 3 / 0 | true |
| 4 | pop | 3 | 0 | 3 != 0 | 0 | 3 / 1 | 1 |
| 5 | push 4 | 3 | 1 | 2 != 4 | 3 | 4 / 1 | true |
| 6 | push 5 | 4 | 1 | 3 != 4 | 0 | 5 / 1 | true |
| 7 | push 6 | 5 | 1 | 4 == 4 | - | 5 / 1 | **false** |
| 8 | pop | 5 | 1 | 5 != 1 | 1 | 5 / 2 | 2 |
| 9 | pop | 5 | 2 | 5 != 2 | 2 | 5 / 3 | 3 |

**Common wrong answer.** *"Push 5 fails because slot 0 holds element 1"* - slot 0's content was consumed in step 4; fullness is `h - t == N`, not "slot index reached 4".

### Q13 - Probe counters (ctors, copies, moves, dtors)
| step | counters | note |
|---|---|---|
| start | 0, 0, 0, 0 | no slot constructed |
| `emplace_back(1)` | 1, 0, 0, 0 | in place |
| `emplace_back(2)` | 2, 0, 0, 0 | |
| `push_back(Probe(3))` | 3, 0, 1, 1 | temp constructed; moved into slot 2; temp destroyed |
| `emplace_back(4)` | 3, 0, 1, 1 | full: returns nullptr, nothing constructed (the int 4 never becomes a Probe) |
| `pop_back()` | 3, 0, 1, 2 | element 3 destroyed; v = [1, 2] |
| `w(v)` | 3, 2, 1, 2 | two element copies |
| `w` destroyed | 3, 2, 1, 4 | |
| `v` destroyed | 3, 2, 1, 6 | live = 3 + 2 + 1 - 6 = 0 |

**Common wrong answers.** *"`emplace_back(4)` on a full vector constructs then destroys"* (it must not construct at all); *"`push_back(Probe(3))` is one ctor"* (the temporary plus a move plus the temporary's destructor).

---

## 5.5 Find the bug

### Q14 - `default` hides the new state
`Emergency` prints **"Idle"** - silently wrong, in a log you will read during an incident. Removing `default`: GCC/Clang `-Wall` warn *"enumeration value 'Idle' not handled in switch"* **and** `'Emergency'` - note `Idle` is also reported: it only worked *because* of the default; `Idle` has no case of its own, so it is indistinguishable from "anything unknown". Fix: add `case CallState::Idle: return "Idle";`, no `default`, and `return "?"` after the switch.

**Common wrong answers.** *"Add `case Emergency` and keep the `default`."* Fixes today's bug; tomorrow's new state repeats it. *"The compiler would have errored anyway."* Only the warning, and only without `default`.

### Q15 - the 3-bug ring
1. **Integer promotion in `size()`**: with `Index = uint8_t`, `head_ - tail_` is an `int`; once `head_` has wrapped (e.g. 2 vs 254) the result is -252, converted to `size_t` = a huge number: `size() == N` is false and `push` overwrites live data. Fix: `static_cast<Index>(head_ - tail_)`.
2. **`% N` with N = 6 and free-running counters**: at the 8-bit wrap the slot sequence jumps (3 -> 0), skipping slots 4 and 5. Fix: require N to be a power of two and use `& (N - 1)`.
3. **No compile-time guards**: nothing rejects non-power-of-two N or `N > max(Index)` (e.g. `Ring<int, 300>` with `uint8_t`), so (2) compiles. Add `static_assert`s (see `RingBuffer`).
(Also: `T buf_[N]` default-constructs all elements - fine for ints, a restriction for others.)

**Common wrong answers.** *"`++head_` overflows - UB."* Unsigned wrap-around is defined; the bug is the promotion and the modulus. *"Use `%` consistently - it already is."* It's consistently *wrong* for this N.

### Q16 - the "optimised" SPSC queue
1. **All `relaxed`**: nothing publishes the element: the consumer may see `head_` advance before the slot content (stale/torn data); the producer may see `tail_` advance and overwrite a slot the consumer has not finished reading. Fix: `head_.store(..., release)` with consumer `head_.load(acquire)`; `tail_.store(..., release)` with producer `tail_.load(acquire)`.
2. **`head_` and `tail_` adjacent**: same cache line -> false sharing, the line ping-pongs between the cores on every call (performance bug, not correctness). Fix: `alignas(64)` each (and the buffer).
3. **Two producers**: the queue is single-producer. Both can read the same `h`, write the same slot, and store `h + 1`: data race (UB), lost element. Fix: one producer (one queue per producer), or a real MPSC design with CAS.

**Common wrong answers.** *"Add a mutex to the push."* Then it's not lock-free and the real-time rules are broken; also leaves the consumer racing. *"Make `buf_` atomic."* Not the issue: ordering of hand-over is.

### Q17 - the naive FixedVector
1. `p[size_] = v` calls `operator=` on an object that does not exist (bytes are uninitialised): for `std::string` that frees a garbage pointer. Fix: `::new (p + size_) T(v)` (placement new).
2. `pop_back` doesn't call `~T()`: the element's resources leak (and its lifetime never ends).
3. `--size_` when `size_ == 0` wraps to `SIZE_MAX`: later writes go far outside the array. Guard `if (size_ == 0) return;`.
4. `~FV() {}` doesn't destroy the live elements: leak. Fix: `clear()`.
Extra: no `alignas(T)` on `storage_` (misaligned `double`/over-aligned types: UB); no `std::launder` when reading through a cast pointer; no copy/move constructors written: the compiler-generated ones copy the *bytes* of non-trivial objects, producing double-frees (rule of five).

**Common wrong answers.** *"Initialise `storage_` to zero."* Zero bytes are not an object; construction is what matters. *"Make it `T storage_[N]`."* That default-constructs all N.

---

## 5.6 Implement

### Q18 - `to_string`
Switch over every enumerator, no `default`, `return "?"` after. The test `OutOfRangeValueIsQuestionMark` casts `200` to the enum (legal to hold: the underlying type is `uint8_t`) and expects `"?"`.

**Wrong:** a `default: return "?"` inside the switch (compiles, silences `-Wswitch`; a new enumerator prints "?" with no warning - Q2/Q14). **Wrong:** returning `std::string` from a `constexpr` function in C++17 (allocation not allowed): use `std::string_view` of literals.

### Q19 - `step`
Reference: `solutions/es_sm/call_state.hpp`. Structure: outer `switch (s)` over all five states with no default; inner `switch (e)` per state listing only its meaningful events, `default: break;`; fall out of the outer switch to `return {s, Action::None, false};`. `constexpr` and `noexcept`: a pure table lookup.
**Common bugs:** typing a transition wrong (the `Step.EveryDocumentedTransition` test names the pair); returning `{next, None, true}` for ignored events (breaks `AllTransitionsExhaustive`: handled must be exactly 10); allowing `Idle + FloorGranted -> Talking` ("optimistic") - violates `TalkingOnlyReachableViaFloorGranted`, a *safety* bug (transmitting without a grant).

### Q20 - `to_event` and `describe`
`std::visit(overloaded{ one lambda per alternative }, m)`. All lambdas of `to_event` return `Event`; those of `describe` are annotated `-> std::string` (otherwise the literal lambdas would return `const char*` and the others `std::string`: *visitor return types must agree*). Print `reason` as a number: `std::to_string(static_cast<unsigned>(x.reason))` - `std::to_string(uint8_t)` would pick an integral promotion overload and work, but streaming a `uint8_t` into an `ostream` prints a character (test `DescribeReasonIsNumberNotCharacter` uses 65 = 'A').
**Wrong:** a catch-all `[](const auto&) { ... }`: compiles, loses the "new message type is a compile error" guarantee (§3.2). **Wrong:** `switch (m.index())`: works but nothing ties the numbers to the types - reordering the variant silently breaks it.

### Q21 - `CallMachine`
`handle(Event)`: `t = step(state_, e)`; if `!t.handled` -> `++ignored_`, return (state unchanged); else `state_ = t.next`; if Idle -> `talker_.reset()`. `handle(const Msg&)`: `t = handle(to_event(m))`; if `t.handled`, `get_if<FloorGranted>` / `get_if<RemoteTalkStart>` -> `talker_ = id`.
**Common bugs:** recording the talker *before* checking `handled` (test `IgnoredMessageDoesNotChangeTalker`: a `FloorGranted{1234}` ignored in Listening must not overwrite 9); forgetting to count ignored; clearing the talker on every handled event (test: talker survives Hang).

### Q22 - `RingBuffer` core
Reference: `solutions/es_sm/ring_buffer.hpp`. `size() = static_cast<Index>(head_ - tail_)`; `full() = size() == N`; `push`: reject when full, `buf_[head_ & kMask] = v; ++head_`; `pop`: move out of `buf_[tail_ & kMask]`, `++tail_`; `clear`: `tail_ = head_`.
**Common bugs:** `size()` without the cast (fails `EightBitCounters...` only after the counters wrap - the very reason that test exists); `%` instead of `&` (works here but wrong for narrow Index and non-powers - the `static_assert` stops the latter); storing `head_ % N` back into `head_` (reduced indices: breaks `head - tail`).

### Q23 - overwrite and subscript
`push_overwrite`: `if (full()) ++tail_; push(v);`. `operator[](i)`: `buf_[(tail_ + i) & kMask]` (use `size_t` arithmetic so `tail_ + i` can't be narrowed). `EightBitCountersFullAndEmptyAcrossWrap` advances counters to 254, then fills across the wrap.
**Common bugs:** `operator[]` returning `buf_[i]` (physical, not logical); computing `(tail_ + i) % N` with `i` narrow.

### Q24 - `SpscQueue`
Orders as in §3.4: own index `relaxed`, other's index `acquire`, own index store `release`. One sentence each: (producer `tail_` acquire) "see the consumer's finished reads before reusing a slot"; (producer `head_` release) "publish the element"; (consumer `head_` acquire) "see published elements"; (consumer `tail_` release) "hand the slot back"; relaxed own-index loads "only I write it". `size_approx`: `tail_` then `head_`, clamp to N.
**Common bugs:** `seq_cst` everywhere (correct, but you can't justify it as the minimum); masking the indices *before* comparing (`(h & mask) == (t & mask)` cannot tell empty from full); the "one slot always empty" design `((h + 1) & mask) == (t & mask)` works but gives capacity N-1, so `Spsc.EmptyAndFullSingleThread` fails; storing `h + 1` into `head_` *before* writing the element (publishes an unwritten slot).

### Q25 - `FixedVector` core
`emplace_back`: `if (size_ == N) return nullptr; T* p = ::new (static_cast<void*>(storage_ + size_ * sizeof(T))) T(std::forward<Args>(args)...); ++size_; return p;`. `pop_back`: guard empty, `--size_`, `elem(size_)->~T()`. `clear`: loop `pop_back`. Destructor: `clear()`.
**Common bugs:** `++size_` before construction; missing `static_cast<void*>` (works, but picks up class-specific placement `operator new` overloads); destroying `N` elements instead of `size_`; `emplace_back` taking `const T&` instead of perfect forwarding (forces a temporary: `ctors` + `moves` instead of one `ctor`).

### Q26 - copy and move
Copy: `try { for each: emplace_back(other[i]); } catch (...) { clear(); throw; }`. Move: same with `std::move(other[i])`, then `other.clear()`. `noexcept(std::is_nothrow_move_constructible_v<T>)` so containers of `FixedVector` can choose move (pack 03 `move_if_noexcept`).
**Common bugs:** no `try/catch` (leak on exception - the destructor does not run for a constructor that throws); moving but leaving `other.size_` unchanged (the source would destroy moved-from objects later - harmless for `Probe`, wrong for types where moved-from destructors assume state); using `memcpy` (breaks non-trivially-copyable `T`).

### Q27 - integration + TSan
Edges for `FloorGranted{7}`: (1) RX thread's plain writes of the message bytes into slot `s` happen-before (program order) its `head_.store(h+1, release)`. (2) The protocol thread's `head_.load(acquire)` reads that stored value -> **synchronizes-with** -> the message writes happen-before the protocol thread's read `out = buf_[t & mask]`. (3) After reading, the protocol thread's `tail_.store(t+1, release)` -> RX thread's later `tail_.load(acquire)` reads it -> the read of slot `s` happens-before RX's next write of that slot. (4) The `CallMachine` is touched only by the protocol thread; the results (`actions`, `machine`) are read by the test thread only after `proto.join()`, which gives a join->return happens-before edge. TSan reports nothing because all conflicting accesses are ordered by (1)-(4).
**Common wrong answer.** Forgetting edge (3) (slot reuse) or (4) (`join` is a synchronization too).

---

## 5.7 Connect

### Q28 - `Mailbox` vs `SpscQueue`
Same protocol: single writer per variable; publish with `release`, observe with `acquire`; plain accesses to the payload in between; `try_*` calls that never block. `Mailbox` is one slot and one flag (`full_`) that flips between "producer owns" and "consumer owns". `SpscQueue` generalises: two free-running counters, N slots, producer and consumer may be up to N apart, so it absorbs bursts. Cost: N x `sizeof(T)` memory, padding for false sharing, mask arithmetic and the wrap-around rule. Each side reads its **own** index `relaxed`: only that thread writes it, so program order guarantees it sees its latest value.

**Weak answers:** "The queue is just many mailboxes" (it is one ring with two indices, not N flags); "relaxed is wrong, always use acquire" (not for your own single-writer variable).

### Q29 - alignment and masks
`alignas(T)` in `FixedVector`: **correctness** (misaligned typed access is UB; on some CPUs a fault). `alignas(64)` in `SpscQueue`: **performance** only (false sharing; correctness holds without it). `is_pow2`/`align_up` (pack 05): **correctness** of mask arithmetic (`align_up(x, a) = (x + a - 1) & ~(a - 1)` requires power-of-two `a`; the ring's `& (N - 1)` requires power-of-two `N`). The tricks are complementary views of the same mask: `x & (a - 1)` = offset inside an `a`-block (= `x mod a`, the ring slot); `x & ~(a - 1)` = start of the block (= `align_down`).

**Weak answers:** "alignas(64) is required for atomics" (only natural alignment is); "all alignment is performance".

### Q30 - `Buffer` vs `FixedVector` move
`Buffer` (heap pointer): move steals the pointer in O(1) and nulls the source; copy allocates a new block and copies bytes. `FixedVector` has elements **inline** in its own storage, so there is no pointer to steal: move must move-construct each element in the destination (O(n)), after which the source still holds moved-from elements - we destroy them so the moved-from vector is **empty** (a deliberate, documented state). Copy copy-constructs each element via placement new, with exception cleanup. Both follow the rule of five because both own a resource whose lifetime the compiler can't manage: heap memory vs live-object bookkeeping.

**Weak answers:** "move of `FixedVector` is O(1)" (only for trivially relocatable *and* heap-based containers).

---

## 5.8 Interview-style

### Q31 - queue between radio and protocol thread
**Model answer.** Use an SPSC ring: one producer (radio/real-time context), one consumer (protocol thread). Fixed capacity N, a power of two, sized by Little's law plus burst margin (e.g. 50 frames/s x 70 ms = 3.5 in flight, plus burst -> 8). Elements trivially copyable (a `variant` of POD messages) so push is a plain copy. **Full policy** decided by the *caller* (`try_push` returns false): drop the newest, count it (`EventCounter`), never block the real-time side. Memory orders: own index relaxed, other's acquire, publish release; `alignas(64)` indices against false sharing. No heap, no locks, `noexcept`. **Testing:** single-thread semantics (full/empty/wrap), a threaded stress test with integrity invariants and deadlines, run under TSan; narrow-counter instantiation to force wrap-around. Mention: if there are several producers, use one queue per producer or an MPSC design.
**Weak answers:** "mutex + condition variable" (not on the real-time path); "std::queue with a lock-free allocator" (still not safe); omitting the full policy.

### Q32 - enum+switch vs variant+visit
**Model answer.** `enum class` + `switch`: closed set of *labels* without data (states, event kinds); the compiler's `-Wswitch` flags unhandled enumerators if there is no `default`; zero overhead; ideal for state and hot-path decisions. `variant` + `visit`: closed set of *types* with different payloads (messages); `visit` is checked at compile time to handle every alternative; no hand-synchronised tag/union; trivially copyable alternatives allow lock-free queue transport. Typical combination: message variant -> `to_event` -> enum event -> `step(state, event)`. Cautions: avoid `std::get` (throws), avoid catch-all lambdas / `default` (lose the check).

### Q33 - why power of two
**Model answer.** Not only speed. (1) `x mod 2^k` = low k bits = `x & (2^k - 1)` (derive from binary: the remainder of division by a power of two is the discarded low bits). (2) With free-running counters that wrap at 2^bits, slot continuity across the wrap requires the buffer length to divide 2^bits, i.e. be a power of two; otherwise slots are skipped after the wrap (show N = 6 table). (3) Free-running counters give `size = head - tail` and "full" without wasting a slot. Speed: a mask is one cycle versus a division that may be tens of cycles or a library call on small cores (VERIFY), but compilers already convert constant power-of-two moduli.
**Weak answer:** "because it's faster."

### Q34 - implementing and testing the call machine
**Model answer.** Pure `step(state, event) -> {next, action, handled}`; `enum class` state and event, `switch` on state with no `default`; a thin `CallMachine` wrapper; actions as data. Tests: (a) table test of all 10 documented transitions, (b) exhaustive loop over all 35 pairs asserting that ignored pairs leave the state unchanged and produce `Action::None` and that the handled count is exactly 10, (c) **graph properties** by BFS: every state reachable from Idle, every state can reach Idle (no dead ends), `Talking` is entered only through `Requesting` + `FloorGranted` (safety: no transmit without a grant), (d) scenario tests of full calls and of error paths (`FloorDenied`, cancel, hang-time reply), (e) property/fuzz test: random event sequences never leave the enum's valid range and never panic. Mention spec traceability: each row maps to a clause of the standard (**VERIFY** each against the actual spec).
**Weak answers:** "test a few scenarios by hand" (no exhaustiveness); "assert in production code that the state is valid" (doesn't prove reachability/safety).
