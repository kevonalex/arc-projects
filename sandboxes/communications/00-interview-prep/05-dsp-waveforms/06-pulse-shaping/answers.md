# Pack 06 — Answers (Pulse shaping)

Hand-computed with a calculator; nothing executed. Roll-off values of real standards are **VERIFY** items.

## Part A

**A1.** Passband two-sided bandwidth = (1+β)·R_s = 1.2 × 4800 = **5.76 kHz** (baseband single-sided: 2.88 kHz).

**A2.** g(mT) = sinc(m) = sin(πm)/(πm) = 0 for m ≠ 0, 1 for m = 0 ✓. G(f) = T·rect(fT): a rectangle of height T from −1/(2T) to +1/(2T). Shifted copies at multiples of 1/T abut exactly → Σₖ G(f − k/T) = T ✓ (minimum-bandwidth Nyquist pulse).

**A3.** h_RC(T) = sinc(1)·cos(π·0.35)/(1−(0.7)²) = 0 · … = **0** ✓. h_RC(0.5T) = sinc(0.5)·cos(π·0.35·0.5)/(1−(0.35)²) = 0.63662 × 0.85264 / 0.8775 = **0.6186**. For β = 0.5, t = T/(2β) = T: limit (π/4)·sinc(1/(2β)) = (π/4)·sinc(1) = **0** (consistent with the zero-ISI property at t = T).

**A4.** With β = 0.35, τ = 0.1 T:
h(0.1) = 0.9825; h(−0.9) = 0.0996; h(1.1) = −0.0776; h(−1.9) = −0.0334; h(2.1) = 0.0272.
Σ_{m≠0} |h| (m = −2..2) ≈ 0.0996 + 0.0776 + 0.0334 + 0.0272 = **0.238** (a few hundredths more from |m| ≥ 3). Worst-case eye height ≈ 2(0.9825 − 0.238) ≈ **1.49** (out of 2.0): a 0.1 T timing error costs ≈ 25 % of the vertical opening.

**A5.** Time-domain: the sinc factor places zeros at every mT. Frequency-domain: with β = 1 and 0 ≤ f ≤ 1/T, H(f) = (T/2)(1+cos πfT); the copy shifted by 1/T is H(f − 1/T) = (T/2)(1 + cos(πfT − π)) = (T/2)(1 − cos πfT); the sum is **T** ✓. For 0 ≤ β < 1 the overlap region is only (1−β)/(2T) to (1+β)/(2T) and the same odd symmetry about 1/(2T) holds.

**A6.** From A4, 0.1 T already costs ~25 % of the eye; for β = 0.2 the tails are larger (slower decay) so the penalty at 0.15 T is larger than at β = 0.35 but the eye is still open (positive height) — run the notebook's cell 3.2 to see the numbers (the cell prints worst-case heights for β = 0.2…1.0). For a *noisy* link the usable margin is much smaller: the eye height is also eaten by noise.

**A7.** h_RRC(0) = 1 − β + 4β/π = 1 − 0.35 + 0.4456 = **1.0956** (in units of 1/T). Normalising Σh² = 1 makes (RRC ∗ RRC)[0] = Σh² = 1 for any β, sps or span, so the end-to-end gain is 1 and signal power / Es/N0 bookkeeping in Pack 08 is trivial.

**A8.** QPSK at 4800 sym/s, β = 0.35: (1+β)·R_s = 1.35 × 4800 = **6.48 kHz** passband. 4FSK (±1800 Hz outer deviation plus shaped spectrum around 2.4 kHz either side): ≈ 8.4 kHz (Carson-style estimate). Linear modulation is more bandwidth-efficient, but FSK allows a constant-envelope PA.

## Part B — reference notes
* **B1 `raised_cosine(β, sps, span)`**: length 2·span·sps+1, t = (n − span·sps)/sps; three cases: t = 0 → 1; |t| = 1/(2β) (β > 0) → (π/4)·sinc(1/(2β)); else sinc(t)·cos(πβt)/(1−(2βt)²). Compare floating values with a tolerance (1e-9), not `==`.
* **B2 `root_raised_cosine`**: t = 0 → 1 − β + 4β/π; |t| = 1/(4β) → (β/√2)[(1+2/π) sin(π/4β) + (1−2/π) cos(π/4β)]; else [sin(πt(1−β)) + 4βt cos(πt(1+β))]/[πt(1−(4βt)²)]; finally scale to Σh² = 1. Test checks (RRC ∗ RRC) centre = 1 and ISI residual < 5e-3 (truncation error at span = 8).
* **B3 `pulse_shape`**: zero-stuff by sps then full convolution ⇒ length nsym·sps + ntaps − 1. A polyphase implementation (Pack 04) skips the zeros.
* **B4 `eye_traces`**: every trace has 2·sps+1 points, starts at offset + k·sps; stop when the window no longer fits.
* **B5 `eye_height`**: samples at offset + k·sps (skipping `guard` symbols at both ends for the filter transients); height = min(positive samples) − max(negative samples); 0 if either rail is empty. Expected ≈ 2 (>1.98) at the correct phase with RC and < 1 half a symbol away.
* **B6 `isi_residual`**: max over m ≠ 0 of |p[c + m·sps]| / |p[c]| with c the centre tap; ≈ 1e-4 for RRC ∗ RRC, ≈ 0.08 for RRC alone (β = 0.35: h_RRC(T) = (sin 0.65π + 1.4 cos 1.35π)/(π(1−1.96)) = 0.2553/(−3.016) = −0.0847, over h_RRC(0) = 1.0956 → 7.7 %).

## Part C — model answers
1. **Time:** g(mT) = δ[m]. **Frequency:** Σₖ G(f − k/T) = T. Derivation: sampling g at mT multiplies by an impulse train, whose spectrum is the periodic sum (1/T)Σ G(f − k/T); the samples being a unit impulse means the periodic sum must be the constant 1 (⇒ Σ G = T).
2. RC satisfies Nyquist but we also need matched filtering for max SNR (Pack 07). Splitting it as RRC at Tx and RRC at Rx gives |H_Tx|·|H_Rx| = H_RC (zero ISI) and H_Rx matched to H_Tx. Also each side only has to deliver half the stopband attenuation, so truncation is less critical.
3. Spectral efficiency (occupied BW = (1+β)R_s) vs sensitivity to timing/sampling error and filter length (small β = long, slowly decaying impulse response). Also PAPR: smaller β → higher envelope peaks in linear modulations.
4. ISI, timing margin (horizontal opening), jitter, SNR margin (vertical opening) and distortion — all *before* decisions; a constellation shows only the sampled values (assuming the right timing). The eye can show *where* the best sampling instant is and whether the timing is drifting.
5. With a fast clock the sampling instant drifts progressively earlier relative to the symbol; the eye "walks" across the display: in a long persistence view the crossing point slides from one side to the other, or a periodic cycle slip appears (data bits are lost/gained). In the receiver, a Gardner TED output has a non-zero mean (S-curve offset) that drives the loop integrator.
