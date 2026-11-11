# 05-dsp — Etherstack interview DSP worksheet packs

Ten self-contained packs. Each has a worksheet (`worksheet.md` or `worksheet.ipynb`), `answers.md`, `cheatsheet.md` and `code/` (CMake + stubs + GoogleTest + reference solutions). All signals are synthetic; all implementation exercises are C++17.

| # | Pack | Worksheet | C++ exercises |
|---|---|---|---|
| 01 | `01-sampling-quant` | notebook | alias frequency, sampling, quantiser, SQNR, band-pass sampling |
| 02 | `02-iq-baseband` | md | complex mixing, I/Q down-conversion, frequency estimator, image rejection |
| 03 | `03-dft-fft` | notebook | DFT, bit-reversal, radix-2 FFT/IFFT, Hann, peak interpolation |
| 04 | `04-filters` | md | convolution, windowed-sinc FIR, biquad, polyphase decimate/interpolate |
| 05 | `05-modulation` | md | 4FSK/C4FM, QPSK, π/4-DQPSK, 16-QAM, OFDM + CP, FM |
| 06 | `06-pulse-shaping` | notebook | RC/RRC, pulse shaping, eye diagram, residual ISI |
| 07 | `07-receiver` | md | matched filter, sync correlator, Gardner, Costas, AGC |
| 08 | `08-channels-ber` | notebook | AWGN, Eb/N0, Doppler, Rayleigh/Rician (Clarke), BER simulation |
| 09 | `09-hardware-q15` | md | Q15/Q31 arithmetic, fixed-point FIR, DC removal, IQ-imbalance correction, Friis |
| 10 | `10-octave-builds` | md + Octave | FFT, streaming FIR, 4FSK, 48-bit sync, CFO, QPSK BER, Q15 FIR vs Octave vectors |

Suggested order: 01 → 10 (each pack refers back to earlier ones). Notebooks need Python 3 + NumPy + Matplotlib; they only visualise — the exercises are C++.

## Building a pack

```
cd <pack>
cmake -S code -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

* **The stubs fail by design.** Implement the `TODO`s in `code/stubs/*.cpp` until `ctest` passes.
* **Check your work against the reference:** `cmake -S code -B build -DUSE_SOLUTIONS=ON`.
* Flags: `-Wall -Wextra -fsanitize=address,undefined` (MSVC: `/W4 /fsanitize=address`). The sanitisers need GCC/Clang on Linux/macOS/WSL; MinGW has no runtime for them, so CMake prints a warning and builds without (`-DENABLE_SANITIZERS=OFF` to silence).
* GoogleTest v1.14.0 is fetched with `FetchContent` (internet needed on first configure).
* Pack 10 additionally: `octave --no-gui octave/gen_vectors.m` creates `vectors/*.txt`; Octave-vector tests skip until then.

## Status / honesty
* **Nothing here has been compiled or executed.** The authoring environment had no C++ compiler, CMake, Python or Octave. Every expected value and tolerance was derived by hand (several were cross-checked by independent hand calculation); expect to adjust a threshold or fix a typo on first build.
* Standards figures (P25 C4FM, DMR, TETRA, sync words, roll-offs, test conditions) are written from memory and flagged **VERIFY** in each worksheet — check them against TIA-102 / ETSI TS 102 361 / ETSI EN 300 392 before quoting them in an interview.
