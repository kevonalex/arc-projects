# Capstone M3 — 4FSK modem, channel, sync and BER

**Tier:** Capstone, milestone 3 of 4. **Time:** 6–8 h. **Code:** `code/` (stubs `include/modem.hpp`, reference `solutions/modem.hpp`, tests `tests/test_modem.cpp`, Monte-Carlo demo `demo/ber_sweep.cpp`).
**Safety/legal:** **synthetic signals only.** The modem runs on generated complex baseband samples; there is no antenna, SDR or capture code. Do not receive live public-safety traffic (in the UK that also runs into the Wireless Telegraphy Act 2006).

---

## §1 Why this milestone exists

This is the "radio" in *radio-to-IP gateway*. Pipeline position (M3 owns the middle):

```
WAV in -> 20 ms frames -> [M1] 2338-bit frame -> [M3] bits -> Gray symbols -> 4FSK baseband IQ
       -> [M3] channel: delay + CFO + AWGN -> sync -> CFO estimate -> demodulate -> bits
       -> [M1] parse/repair -> [M2] mu-law RTP over UDP multicast -> jitter buffer -> WAV out
       -> [M4] gRPC StartCall/EndCall + streaming stats (BER, loss, jitter) + floor control
```

M3 turns *signal-to-noise ratio* into *bit error rate*, which is the one number that decides whether M1's FEC design works. By the end you can: modulate and demodulate continuous-phase 4FSK, build an AWGN + frequency-offset + delay channel with a correctly calibrated noise level, find a frame by non-coherent correlation, estimate carrier offset, compare simulated BER with closed-form theory, and answer "how many bits do I need to measure a BER of 10^-6?".

## §2 Prerequisites

* Complex numbers as rotating phasors (`e^{jθ}`); sums of phasors; Gaussian noise. DSP pack (05) is the best preparation. M1 for the frame and sync word.
* Build: `cmake -S code -B build -DCMAKE_CXX_FLAGS="-Wall -Wextra -fsanitize=address,undefined"`, `cmake --build build`, `ctest --test-dir build`. `student.*` fail until implemented; `solution.*` pass. Optional: run `build/ber_sweep`.

## §3 Concept build-up

### §3.1 Bits to symbols, Gray mapping (W 3.1)

Two bits select one of four tones: a **symbol** carries 2 bits, so the symbol rate is half the bit rate. Gray mapping (neighbouring tones differ in exactly one bit):

| bits (b1 b0) | tone index | frequency (cycles/symbol) |
|---|---|---|
| 00 | 0 | −1.5 |
| 01 | 1 | −0.5 |
| 11 | 2 | +0.5 |
| 10 | 3 | +1.5 |

Test: `{0,0, 0,1, 1,1, 1,0}` → symbols `{0, 1, 2, 3}`. An odd trailing bit is ignored.

Why Gray: when the *likeliest* error is an adjacent tone (frequency offset, a discriminator-type detector, a fade), Gray makes it cost one bit instead of two. See the honesty note in §3.7: with *orthogonal* tones in pure AWGN, all three wrong tones are equally likely, and Gray does not change the average BER.

### §3.2 Continuous-phase 4FSK and non-coherent detection (W 3.2)

Concrete first. At 8 samples per symbol (`sps = 8`), tone *k* is `x[n] = e^{jφ[n]}` with phase advancing `2π·f_k/8` per sample, where `f_k ∈ {−1.5, −0.5, +0.5, +1.5}` cycles per symbol. Unit amplitude, so symbol energy `Es = Σ|x|² = 8`.

* **Continuous phase (CPM):** one *running* phase accumulator carries across symbol boundaries, so there are no phase jumps. Jumps would splatter spectrum into neighbouring channels. (Test: the increment between *every* adjacent sample pair, including across boundaries, equals the tone's.)
* **Constant envelope:** `|x| = 1` always. A power amplifier can be driven into saturation (efficient, cheap, battery-friendly) without distorting the signal. This is why FSK is so common in handheld radios.
* **Orthogonal tones:** tones spaced by exactly `1/T` (1 cycle per symbol): over one symbol the correlation of tone *a* against tone *k* is `Σ e^{j2π(a−k)n/8} = 0` for `a ≠ k` (a full number of cycles cancels) and `= 8` for `a = k`. Energies: `64` on the right tone, `0` on others (test: `TonesAreOrthogonalOverOneSymbol`).
* **Non-coherent detection:** the receiver does not know the carrier phase, so it uses the *magnitude* of each correlation, `|Σ w[n] e^{−j2πf_k n/8}|²`, and picks the largest of the four. Phase rotates the complex correlation but not its magnitude. Cost: ~1 dB worse than coherent detection at the same BER (VERIFY the figure) in exchange for no phase recovery.

> **Misconception box.** *"4FSK has four frequencies, so it needs four filters of a channel's bandwidth."* The tones sit within one channel; it is the *spacing* (1/T) and the symbol rate that set bandwidth, roughly 5/T null-to-null here (tones at ±1.5/T plus a ±1/T main lobe). *"A bigger spacing is always better."* Wider spacing = more bandwidth for no BER gain once tones are orthogonal. *"Non-coherent means no synchronisation."* You still need symbol timing (that's the sync of §3.4); you skip only carrier *phase*.

Honesty: this toy has `1/T`-spaced tones (modulation index 1), roughly 4–5 × the symbol rate of bandwidth. Narrowband land-mobile systems use a much smaller deviation so as to fit 12.5 kHz or 6.25 kHz channels (VERIFY any specific standard), accept non-orthogonal tones, and use a frequency-discriminator-style receiver. Therefore adjacent-tone errors dominate there, which is where Gray coding really earns its keep.

### §3.3 The channel: AWGN with the *right* amount of noise, plus CFO (W 3.3)

**Es/N0 calibration (the common bug).** Signal energy per symbol: `Es = sps = 8` (unit-amplitude samples). For *complex* noise of total variance `σ²` per sample, `N0 = σ²`. So `Es/N0 = 8/σ²` → `σ² = 8/g` with `g = 10^(dB/10)`, and each of I and Q gets `σ_comp = √(σ²/2)`. At 10 dB: `σ² = 0.8`, `σ_comp = 0.632`. At 0 dB: `σ_comp = 2`. (Test checks `√(0.5 × 8/10)`.) Forgetting the factor 2 between total and per-component variance shifts every result by 3 dB.

A useful consequence: summing 8 samples with unit weight multiplies signal amplitude by 8 and noise variance by 8, so the *correlator output* SNR is `64/(8σ²) = 8/σ² = Es/N0`. The correlator gives full symbol energy.

**Carrier frequency offset (CFO).** Transmitter and receiver oscillators disagree (ppm × carrier = kHz). In baseband it multiplies the signal by `e^{j2πδn/sps}`, with δ in cycles per symbol. A CFO `δ` shifts every tone by `δ`:

* Correlation with the right tone is multiplied by `sinc(δ)`: energy loss `sinc²(δ)` = −0.21 dB at δ = 0.12, −0.58 dB at 0.2, **−3.9 dB at 0.5**.
* Leakage into the neighbouring tone grows as `sinc²(1−δ)`: 1.8 % at 0.12, 5.5 % at 0.2, and **equal to the right tone at δ = 0.5**.

So the decision breaks at **half the tone spacing** (δ = 0.5, i.e. half a symbol rate): the test shows 0.2 harmless (noiseless), 0.6 putting about 3/4 of symbols onto the next tone. In practice the noise margin erodes well before 0.5, hence the estimator of §3.5.

**Delay.** The receiver doesn't know where the frame starts: the channel prepends `delay` zeros (and appends `4·sps` zeros) so the receiver must *find* it.

### §3.4 Finding the frame: non-coherent sync (W 3.4)

The 24 sync bits become **12 symbols**. At each candidate sample offset *t*, correlate each 8-sample window with the expected tone and add up the magnitudes squared:

`metric(t) = Σ_i |⟨w_i, tone_i⟩|² / (sps × energy of the 12·sps samples)`

* Perfect alignment, no noise: `12 × 64 / (8 × 96) = 1.0`. Pure noise: about `1/sps = 0.125`.
* Because it uses magnitudes per symbol, it works with unknown carrier phase, and tolerates modest CFO (loss `sinc²`).
* `find_frame_start` returns the offset with the highest metric if it exceeds a **threshold** (0.5 here), else `-1`. Threshold trade: too low → false frames in noise; too high → missed frames at low SNR (the chain test lowers it to 0.4 at 10 dB). Peak within ±1 sample of the true delay (test: 53 ± 1).

### §3.5 Estimating CFO from the known sync (W 3.5)

You know what was sent (sync symbols `s`), so strip the data: for the received `r` and known `s = modulate(known)`:

`δ̂ = angle( Σ [ r[n+1]·conj(r[n]) ] · conj( s[n+1]·conj(s[n]) ) ) · sps / (2π)`

Why it works: `r[n+1] conj(r[n]) = s[n+1] conj(s[n]) · e^{j2πδ/sps}` (the unknown carrier phase cancels in the *product of neighbours*), and multiplying by the conjugate of the known product leaves `e^{j2πδ/sps}`; the angle is `2πδ/sps`, so scale by `sps/2π`. At δ = 0.12 the per-sample rotation is `2π × 0.12/8 = 0.094 rad = 5.4°`. The estimate is unambiguous for `|δ| < sps/2` (4 cycles per symbol), far beyond what the tone detector tolerates. Accuracy depends on SNR and the 96 samples of sync (test tolerance 0.03 at 15 dB). The receiver then **derotates a copy** of the input by `−δ̂`.

### §3.6 The receive chain (W 3.6)

`find sync → estimate CFO on the sync symbols → derotate a copy → demodulate total_bits/2 symbols from the frame start → Gray-decode to bits`. Return `found = false` with empty bits if no sync. Always return what was found (start, metric, cfo) so that M4 can report them. Test: a 2338-bit frame (M1's length) at Es/N0 = 16 dB with CFO 0.12 and delay 53 is received with **zero** bit errors; the start is 53 ± 1 and the CFO estimate within 0.03.

### §3.7 BER: theory, measurement, and how many bits you need (W 3.7)

* **Es/N0 vs Eb/N0:** `Eb/N0 = Es/N0 − 10·log10(bits per symbol)`: for 4FSK, 3.01 dB lower (Es/N0 = 10 dB ↔ Eb/N0 = 6.99 dB). Use Eb/N0 to compare modulations fairly; Es/N0 is what the simulator is calibrated in.
* **Closed form** (non-coherent orthogonal 4FSK, `g = Es/N0` linear): `Ps = 3/2·e^{−g/2} − e^{−2g/3} + 1/4·e^{−3g/4}`, `Pb = (2/3)·Ps`. The 2/3 comes from orthogonal signalling: a wrong symbol is equally likely to be any of the other 3 tones, so a given bit is wrong in 2 of the 3 → `(M/2)/(M−1) = 2/3`.
* **Measure:** count differing bits (`count_bit_errors`; missing bits count as errors). The statistical rule of thumb: you need **~100 errors** for a ±20 % (roughly 95 %) estimate (1/√N relative standard deviation: 10 % at 100 errors). So `bits_needed = errors / BER`: 10^5 bits for BER 10^-3; **10^8 for 10^-6**.

Monte-Carlo vs theory (`ber_sweep`, 200 000 bits per point, deterministic seeds):

| Es/N0 (dB) | Eb/N0 (dB) | BER measured | BER theory | errors seen |
|---|---|---|---|---|
| 0 | −3.01 | 3.456e-01 | 3.430e-01 | 69121 |
| 4 | 0.99 | 1.857e-01 | 1.852e-01 | 37150 |
| 8 | 4.99 | 3.422e-02 | 3.418e-02 | 6845 |
| 10 | 6.99 | 5.810e-03 | 5.982e-03 | 1162 |
| 12 | 8.99 | 3.450e-04 | 3.457e-04 | 69 (too few) |
| 14 | 10.99 | 0 | 3.477e-06 | 0 (too few) |

At 14 dB the sweep saw zero errors in 2×10^5 bits *because the expected count was 0.7*, not because the BER is zero. To see ~100 errors at 14 dB you need `100 / 3.5×10^-6 = 2.9×10^7` bits.

### §3.8 Linking back to M1: the operating point

The M1 FEC design point was BER ≈ 10^-3. Solving the theory for `Pb = 10^-3` gives **Es/N0 ≈ 11.4 dB (Eb/N0 ≈ 8.4 dB)**. At that point M1's Hamming-protected body fails in about 0.7 % of frames (the figure in M1 §4(c)). Moving along the curve (independent bit errors, Hamming words only, binomial): at Es/N0 = 10 dB (BER 6×10^-3) about **21 %** of frames fail; at 11 dB (BER 1.7×10^-3) about **2 %**; at 12 dB (BER 3.5×10^-4) about **0.08 %**; at 14 dB about 10^-7. **Each extra dB cuts frame loss by roughly 10–25× in this region.** That steepness (the "waterfall") is why link budgets need margin, and why M4 reports BER.

> **Misconception box.** *"BER 10^-3 is bad."* It's the pre-FEC design point; after FEC and CRC the frame error rate is what matters. *"I measured zero errors, so BER is 0."* Only an upper bound: with N bits and 0 errors, BER < ~3/N at 95 % confidence ("rule of three"). *"More bits per symbol is free."* More tones need more Es/N0 per bit (or bandwidth): it's a trade.

### §3.9 Misconceptions summarised

| Claim | Reality |
|---|---|
| Noise σ is just 1/√SNR | Calibrate against Es = sps and complex variance (factor 2 per component). |
| FSK needs carrier recovery | Non-coherent magnitude detection needs only symbol timing. |
| CFO just shifts the whole signal, harmless | Harmless below ~0.2 tone spacing; breaks at 0.5. |
| Sync correlation is coherent | The metric sums *magnitudes*, so it ignores phase. |
| Gray coding changes BER of orthogonal AWGN | Not on average; it helps when adjacent-tone errors dominate. |
| Es/N0 = Eb/N0 | Differ by 10·log10(bits/symbol). |

## §4 Worked examples

**(a) Calibrate noise.** Target Es/N0 = 6 dB: `g = 3.98`, `σ² = 8/3.98 = 2.01`, `σ_comp = 1.00`. The test confirms `E|n|² ≈ 8/10^0.6 = 2.01` within 3 %. At that point theory BER is 9.8×10^-2 (measured 9.86×10^-2).

**(b) One symbol, by hand.** Send tone 2 (+0.5 cycles/sym) with no noise: correlations are `[0, 0, 64, 0]`. Add noise at Es/N0 = 10 dB (σ² = 0.8): the right output has amplitude 8 and noise variance `8σ² = 6.4`, the wrong ones are pure noise of the same variance. An error happens when a wrong tone's `|·|²` exceeds the right one: that happens about 0.9 % of the time per symbol (`g = 10`: `Ps = 3/2 e^{−5} − e^{−6.67} + 1/4 e^{−7.5} ≈ 0.90 %`), and 2/3 of those symbol errors flip a given bit → `Pb = 0.60 %` (theory 5.98×10^-3).

**(c) Frame timing.** M1 frame = 2338 bits = 1169 symbols = 9352 samples at `sps = 8`. The sync is the first 12 symbols = 96 samples (1.0 % of the frame): the CFO estimator and sync detector both work from those 96 samples only, which is why a CFO that drifts *during* a long frame is a different problem (see Q17).

## §5 Questions

Do §5 on paper first. **Story** questions are written answers of 150–250 words that become interview stories: *context → decision → alternatives rejected → numbers → what I'd change*.

**Q1 — Map.** Gray-map the bit string `1 0 0 1 1 1 0 0` to tone indices and to frequencies in cycles/symbol. Which adjacent-tone error would turn the symbol from `10` into `11`, and how many bit errors does it cost? Why would a natural binary mapping (00, 01, 10, 11) be worse in that case?
*Context: W 3.1.*

**Q2 — Explain.** In three sentences, why does a symbol window of 8 samples and tone spacing of exactly 1 cycle/symbol give *zero* cross-correlation between different tones? What breaks if the spacing is 0.8 cycles/symbol?
*Context: W 3.2.*

**Q3 — Calculate.** Given `sps = 8` and unit-amplitude samples: `Es`; the noise standard deviation per I/Q component for Es/N0 = 0 dB, 10 dB and 20 dB; and the SNR at the correlator output.
*Context: W 3.3.*

**Q4 — Debug.** A colleague's `add_awgn` uses `σ_comp = √(8/g)` (they forgot the ½). By how many dB is their effective Es/N0 off, and in which direction? What symptom would you see in the BER sweep?
*Context: W 3.3, test `MeasuredNoisePowerMatchesEsN0`.*

**Q5 — Calculate.** For a CFO of 0.12 cycles/symbol: the per-sample phase rotation (rad and degrees), the energy loss `sinc²(0.12)` in dB, and the CFO in Hz for a symbol rate of 4 800 sym/s. For δ = 0.5 what is the loss and what happens at the decision?
*Context: W 3.3.*

**Q6 — Predict.** The test `CfoBelowHalfToneSpacingIsHarmlessAboveItBreaksEverything` shows 0.2 error-free and 0.6 with more than 60 % wrong symbols (about 3/4). Explain, with the neighbour-tone leakage, why 0.6 sends symbols to the *next tone up*, and why tone 3 behaves differently from tones 0–2 (hint: there is no tone 4).
*Context: W 3.3.*

**Q7 — Calculate.** The sync metric: show that a perfectly aligned noiseless sync gives 1.0, and that noise alone averages about `1/sps`. Why is a threshold of 0.5 a reasonable first guess and what does lowering it to 0.4 trade?
*Context: W 3.4.*

**Q8 — Trace.** Walk through `estimate_cfo` for a CFO of 0.12: write what `r[n+1] conj(r[n])` becomes, what multiplying by the conjugate of the known product leaves, and how the angle becomes cycles per symbol. What is the largest CFO the estimator can represent unambiguously?
*Context: W 3.5.*

**Q9 — Order.** Give the receive-chain steps and justify the order. What goes wrong if you demodulate *before* derotating at CFO 0.45? Why must `receive` derotate a *copy* and not mutate the caller's buffer?
*Context: W 3.6.*

**Q10 — Calculate.** Using the closed form, compute `Ps` and `Pb` at Es/N0 = 8 dB (`g = 6.31`) and confirm against the 3.42×10^-2 in the table. Convert to Eb/N0.
*Context: W 3.7.*

**Q11 — Calculate.** How many bits do you need to measure BER = 10^-3, 10^-6 and 10^-9 with ~100 errors? Convert each to the number of M1 frames and to minutes of 20 ms-frame audio. If a test runs at 100 frames/second of simulation, how long is the 10^-9 test?
*Context: W 3.7.*

**Q12 — Interpret.** `ber_sweep` printed 0 errors at Es/N0 = 14 dB for 200 000 bits. What can and can't you conclude? Give the 95 % upper bound on BER (rule of three) and the number of bits needed to get 100 expected errors.
*Context: W 3.7.*

**Q13 — Story (design).** *Why non-coherent 4FSK for this link rather than QPSK/QAM?* Cover: constant envelope (saturated PA, battery), no carrier-phase recovery, robustness to fading/phase noise, simplicity, the ~1 dB non-coherent penalty (VERIFY), spectral efficiency (the toy uses roughly 4–5 × symbol-rate bandwidth), and when you would pick something else (spectral crowding → narrower deviation, linear modulation).

**Q14 — Story (design).** *Why Gray mapping if the theory says orthogonal-tone errors are equiprobable?* Cover: the 2/3 factor derivation, where adjacent-tone errors dominate (CFO leakage, discriminator receivers, non-orthogonal narrow deviation), the free cost of Gray, and the interaction with the M1 FEC (Hamming sees mostly single-bit errors per symbol).

**Q15 — Story (design).** *How much CFO can this modem tolerate and how did you design for it?* Cover: the half-spacing limit, the sinc losses, oscillator ppm → kHz arithmetic (e.g. ±2 ppm at 450 MHz is ±900 Hz; compare with a 4 800 sym/s tone spacing; VERIFY your numbers), why you estimate from sync and derotate, residual error, and drift within a frame.

**Q16 — Story (design).** *Es/N0 vs Eb/N0 and how I proved the modem is correct.* Cover: calibration of the channel (the test that measured noise power), comparison with theory with an explicit statistical tolerance (0.8–1.25× at 40 000 bits), the bit budget (~100 errors rule), deterministic seeds so CI is reproducible, and the "zero errors ≠ zero BER" trap.

**Q17 — Story (design).** *Defend the sync threshold and the structure of the receiver.* Cover: the metric's noise floor (≈ 0.125) vs 0.5/0.4 thresholds, false-alarm vs miss trade, a one-shot sync at the start vs periodic re-sync or tracking, what happens when CFO drifts across a 9352-sample frame, and why a CRC (M1) is still the final arbiter.

**Q18 — Etherstack / integration.** Link M3 to M1 and M4: what Es/N0 does the gateway need so that M1's frame loss is below 1 %? What would you export to the M4 stats stream (sync metric, CFO estimate, BER via M1's correction counts)? How do you test the whole chain in CI with no SDR and no live traffic? State explicitly why captured public-safety signals are not part of the test corpus.

## §6 Self-assessment

You are done when you can, without notes:

* [ ] Gray-map bits to tones and explain adjacent-tone errors.
* [ ] Show tone orthogonality and non-coherent detection on one symbol.
* [ ] Derive σ per component from Es/N0 and spot the factor-2 bug.
* [ ] Explain the CFO breakdown at half the tone spacing and the sync-based estimator.
* [ ] Quote the BER table (8 dB → 3.4×10^-2, 10 dB → 6×10^-3, 12 dB → 3.5×10^-4) and the ~11.4 dB design point.
* [ ] State the ~100-error rule: 10^5 bits for 10^-3, 10^8 for 10^-6.
* [ ] `ctest` shows all `solution.*` green with ASan/UBSan clean, `student.*` red until you implement.

**VERIFY list (not tested here):**

1. The non-coherent penalty versus coherent FSK (~1 dB at these BERs) and the closed form's textbook derivation (Proakis, Digital Communications: non-coherent M-ary orthogonal signalling).
2. Typical deviation, symbol rate and channel bandwidth for any real public-safety radio standard; this toy's modulation index 1 is not any of them.
3. The ±20 % at ~100 errors rule (a Poisson/normal approximation) and the rule-of-three bound.
4. Oscillator accuracy (ppm) figures you quote for real handsets or base stations.
5. The M1 interplay numbers (frame-loss vs Es/N0) assume independent bit errors and Hamming/Golay thresholds only; they ignore sync loss and burst errors.
