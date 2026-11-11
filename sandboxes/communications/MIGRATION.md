# Migration map — awaiting final go-ahead

**Status:** **MIGRATION EXECUTED** (2026-11-11). Skeleton + moves + pack.yaml retrofit + phase/twins scaffolds + PROGRESS.md done.  
**Scope root:** `sandboxes/communications/`. Quant: `sandboxes/quant/`.  
**Git:** simple separate commits (no special `git mv` process). Flag name remains `USE_SOLUTIONS`.

---

## Decisions recorded (your answers)

| # | Decision |
|---|----------|
| 1 | Keep learning repo in `sandboxes/communications/` |
| 2 | Keep **fine-grained** sub-packs under each module |
| 3 | **Leave DSP layout as-is** (`01`…`10`); do **not** regroup into `05a`–`05d` |
| 4 | **Keep** empty `networking/` and `waveforms/` for later |
| 5 | Move existing `projects/quant/` → **`sandboxes/quant/`** (separate sandbox) |
| 6 | Keep flag name **`USE_SOLUTIONS`** (same meaning as prompt’s BUILD_SOLUTIONS) |
| 7 | **Keep git simple:** ordinary moves + clear separate commits; no special `git mv` process |

---

## 1. Inventory (current)

### 1.1 Top level — `sandboxes/communications/`

| Path | Contents |
|------|----------|
| `modules/` | **64** interview sub-packs (all have `worksheet` + `answers.md` + `cheatsheet.md` + `code/`) |
| `modules/README.md` | Pack index, tiers, dependencies |
| `modules/STUDY_PATH.md` | Study order + interview story bank |
| `modules/VERIFY.md` | Auto-extracted VERIFY checklist (~560 rows) |
| `networking/` | **Empty** |
| `waveforms/` | **Empty** |
| `MIGRATION.md` | This file |

**Missing vs target template (all 64 packs):** no `pack.yaml`, no per-pack `README.md`, no jupytext pairing, no root `CMakeLists.txt` / `STYLE.md` / `ROADMAP.md` / `PROGRESS.md` / CI / `shared/` / `tools/` / `twins/`.

### 1.2 Interview sub-packs (64)

| Current module | Sub-packs |
|----------------|-----------|
| `modules/01-cpp/` | `01-toolchain` … `07-sm-builds` (7) |
| `modules/02-bits/` | `01-bit-ops` … `06-conv-viterbi` (6) |
| `modules/03-net/` | `01-headers-layering` … `06-cellular-encodings` (6) |
| `modules/04-audio/` | `01-sampling-pcm` … `04-builds` (4) |
| `modules/05-dsp/` | `01-sampling-quant` … `10-octave-builds` (10) |
| `modules/06-cellular/` | `01-2g-3g` … `04-mcptt-iwf` (4) |
| `modules/07-lmr/` | `01-p25-phase1` … `06-builds` (6) |
| `modules/08-embedded/` | `01-bare-metal` … `04-builds` (4) |
| `modules/leetcode/` | `01-bits` … `13-dp` (13) |
| `modules/capstone/` | `M1-framing-fec` … `M4-grpc-floor` (4) |

**Notebooks (4):**  
`05-dsp/{01-sampling-quant,03-dft-fft,06-pulse-shaping,08-channels-ber}/worksheet.ipynb` — no jupytext `.py` pair yet.

### 1.3 Related material *outside* this folder (not moved unless you say so)

| Path | Notes |
|------|-------|
| `projects/quant/` | 6 files (price/portfolio / backtesting notebooks) — Phase 3 quant-finance adjacent |
| `projects/documentation/c++/C++_CHEAT_SHEET.md` | Referenced by pack 01-toolchain |
| `projects/tutorials/` | C++/Rust tutorials |
| `projects/agentics/PROMPTFORMAT.md` | Your learning/answer style (feeds `STYLE.md`) |

### 1.4 Git reality (affects `git mv`)

- Repo root is `projects/` (`main...origin/main`).
- Entire `sandboxes/` is currently **untracked** (`?? sandboxes/`).
- `git mv` cannot preserve history that was never committed. First step after approval must be either:
  - **A)** commit current tree as-is, then `git mv`, or
  - **B)** `git mv`-style moves in the working tree, then one “import + restructure” commit (history starts at that commit).

---

## 2. Conflicts — need your choice before moving

### Q1 — Repo root
**Proposal:** treat `sandboxes/communications/` as the learning-repo root (target `/` in your prompt).  
**Alt:** promote to `projects/learning/` or a new standalone git repo.

### Q2 — Keep fine-grained sub-packs vs collapse to one pack per module
Your target lists `01-cpp-for-protocols` as a single pack id, but the tree already has **7** C++ packs (and similarly for bits/net/…). Collapsing would mix worksheets and destroy the 2–4 h sizing.

**Proposal (non-destructive):**  
`00-interview-prep/01-cpp-for-protocols/` is a **module folder** containing the existing sub-packs unchanged (`01-toolchain/`, …). Same pattern for 02–09 and capstone. Module-level `README.md` + optional rolled-up `pack.yaml`; each sub-pack gets its own `pack.yaml`.

### Q3 — DSP regroup into `05a`–`05d` — **DECIDED: no**
Leave flat `05-dsp-waveforms/01-sampling-quant` … `10-octave-builds`. Deepening packs (BER Q-tail, Viterbi↔HMM) can be added later as `11-…`, `12-…` without regrouping.

### Q4 — Empty `networking/` and `waveforms/`
**Proposal:** delete after migrate (empty; nothing to put in `legacy/`).  
**Alt:** keep as empty placeholders under `00-interview-prep/legacy/`.

### Q5 — Outside quant / docs / PROMPTFORMAT
**Proposal:** leave in place; root `README.md` links out; later Phase 3 may absorb `quant/` via a separate migration. `STYLE.md` incorporates `agentics/PROMPTFORMAT.md` rules (copy, don’t delete source).

### Q6 — CMake solutions flag
Packs currently use mixed `USE_SOLUTIONS` / similar. Target wants `-DBUILD_SOLUTIONS=ON`.  
**Proposal:** standardize on `BUILD_SOLUTIONS` during retrofit ( mechanised rename in each `CMakeLists.txt`).

### Q7 — First git commit strategy
**Proposal:** Option **A** — one commit “Import communications interview packs (pre-migration baseline)”, then migration commits with `git mv`.

---

## 3. Proposed move map (old → new)

Assumes Q1–Q3 proposals. Prefix all new paths with `sandboxes/communications/`.

### 3.1 Repo skeleton (create new; no source)

| New path | Action |
|----------|--------|
| `README.md` | Create (phase index; replace role of `modules/README.md`) |
| `STYLE.md` | Create from teaching rules + `PROMPTFORMAT.md` |
| `ROADMAP.md` | Create (phases 0–5 table) |
| `PROGRESS.md` | Generated later by `tools/progress.py` |
| `CMakeLists.txt` | Create root; `add_subdirectory` every pack with `code/` |
| `pyproject.toml` | Create |
| `.pre-commit-config.yaml` | Create (nbstripout, etc.) |
| `.github/workflows/ci.yml` | Create |
| `shared/cpp/`, `shared/py/` | Create empty + README |
| `tools/new_pack.py`, `tools/progress.py` | Create |
| `templates/pack/` | Create |
| `twins/*.md` | Create short stubs (12 pages) |
| `10-foundations/**` | Scaffold only |
| `20-core-modelling/**` | Scaffold only |
| `30-advanced/**` | Scaffold only |
| `40-research/**` | Scaffold only |
| `00-interview-prep/question-bank.md` | Create empty scaffold |
| `00-interview-prep/mock-interviews/` | Create empty dir + README |

### 3.2 Meta files currently under `modules/`

| Old | New | Notes |
|-----|-----|-------|
| `modules/README.md` | `00-interview-prep/README.md` (content merged into root `README.md` + this) | Keep useful tables; mark superseded bits |
| `modules/STUDY_PATH.md` | `00-interview-prep/STUDY_PATH.md` | Keep; aligns with this-week priority |
| `modules/VERIFY.md` | absorbed into generated `PROGRESS.md` VERIFY section | Keep file temporarily as `00-interview-prep/VERIFY.legacy.md` until progress tool works |
| `MIGRATION.md` | stay at repo root | This file |

### 3.3 Module 01 — C++

| Old | New |
|-----|-----|
| `modules/01-cpp/01-toolchain/` | `00-interview-prep/01-cpp-for-protocols/01-toolchain/` |
| `modules/01-cpp/02-types-basics/` | `00-interview-prep/01-cpp-for-protocols/02-types-basics/` |
| `modules/01-cpp/03-lifetime-move/` | `00-interview-prep/01-cpp-for-protocols/03-lifetime-move/` |
| `modules/01-cpp/04-containers-templates/` | `00-interview-prep/01-cpp-for-protocols/04-containers-templates/` |
| `modules/01-cpp/05-ub-layout-bit/` | `00-interview-prep/01-cpp-for-protocols/05-ub-layout-bit/` |
| `modules/01-cpp/06-concurrency-rt/` | `00-interview-prep/01-cpp-for-protocols/06-concurrency-rt/` |
| `modules/01-cpp/07-sm-builds/` | `00-interview-prep/01-cpp-for-protocols/07-sm-builds/` |

### 3.4 Module 02 — Bits

| Old | New |
|-----|-----|
| `modules/02-bits/01-bit-ops/` | `00-interview-prep/02-bits-and-parsing/01-bit-ops/` |
| `modules/02-bits/02-endian-seq/` | `00-interview-prep/02-bits-and-parsing/02-endian-seq/` |
| `modules/02-bits/03-parse-headers/` | `00-interview-prep/02-bits-and-parsing/03-parse-headers/` |
| `modules/02-bits/04-crc-checksum/` | `00-interview-prep/02-bits-and-parsing/04-crc-checksum/` |
| `modules/02-bits/05-block-fec/` | `00-interview-prep/02-bits-and-parsing/05-block-fec/` |
| `modules/02-bits/06-conv-viterbi/` | `00-interview-prep/02-bits-and-parsing/06-conv-viterbi/` |

### 3.5 Module 03 — Networking

| Old | New |
|-----|-----|
| `modules/03-net/01-headers-layering/` | `00-interview-prep/03-networking-protocols/01-headers-layering/` |
| `modules/03-net/02-udp-tcp-qos/` | `00-interview-prep/03-networking-protocols/02-udp-tcp-qos/` |
| `modules/03-net/03-sockets-poll/` | `00-interview-prep/03-networking-protocols/03-sockets-poll/` |
| `modules/03-net/04-rtp-sip/` | `00-interview-prep/03-networking-protocols/04-rtp-sip/` |
| `modules/03-net/05-grpc-security/` | `00-interview-prep/03-networking-protocols/05-grpc-security/` |
| `modules/03-net/06-cellular-encodings/` | `00-interview-prep/03-networking-protocols/06-cellular-encodings/` |

*“03-core” this week* = packs `01`–`04` under this module (Tier-1 net).

### 3.6 Module 04 — Voice

| Old | New |
|-----|-----|
| `modules/04-audio/01-sampling-pcm/` | `00-interview-prep/04-voice-over-packets/01-sampling-pcm/` |
| `modules/04-audio/02-codecs/` | `00-interview-prep/04-voice-over-packets/02-codecs/` |
| `modules/04-audio/03-realtime-audio/` | `00-interview-prep/04-voice-over-packets/03-realtime-audio/` |
| `modules/04-audio/04-builds/` | `00-interview-prep/04-voice-over-packets/04-builds/` |
| *(new)* | `00-interview-prep/04-voice-over-packets/05-jitter-as-quantile/` *(short deepening; scaffold after migrate)* |

### 3.7 Module 05 — DSP (flat layout kept)

| Old | New |
|-----|-----|
| `modules/05-dsp/01-sampling-quant/` | `00-interview-prep/05-dsp-waveforms/01-sampling-quant/` |
| `modules/05-dsp/02-iq-baseband/` | `00-interview-prep/05-dsp-waveforms/02-iq-baseband/` |
| `modules/05-dsp/03-dft-fft/` | `00-interview-prep/05-dsp-waveforms/03-dft-fft/` |
| `modules/05-dsp/04-filters/` | `00-interview-prep/05-dsp-waveforms/04-filters/` |
| `modules/05-dsp/05-modulation/` | `00-interview-prep/05-dsp-waveforms/05-modulation/` |
| `modules/05-dsp/06-pulse-shaping/` | `00-interview-prep/05-dsp-waveforms/06-pulse-shaping/` |
| `modules/05-dsp/07-receiver/` | `00-interview-prep/05-dsp-waveforms/07-receiver/` |
| `modules/05-dsp/08-channels-ber/` | `00-interview-prep/05-dsp-waveforms/08-channels-ber/` |
| `modules/05-dsp/09-hardware-q15/` | `00-interview-prep/05-dsp-waveforms/09-hardware-q15/` |
| `modules/05-dsp/10-octave-builds/` | `00-interview-prep/05-dsp-waveforms/10-octave-builds/` |
| *(new, later)* | `00-interview-prep/05-dsp-waveforms/11-ber-gaussian-tail/` |
| *(new, later)* | `00-interview-prep/05-dsp-waveforms/12-viterbi-as-hmm/` |

### 3.7b Quant sandbox (separate)

| Old | New |
|-----|-----|
| `projects/quant/*` (notebooks, `data/`) | `sandboxes/quant/` (same relative layout; skip `.ipynb_checkpoints`) |

### 3.8 Module 06 — Cellular

| Old | New |
|-----|-----|
| `modules/06-cellular/01-2g-3g/` | `00-interview-prep/06-cellular-2g-6g/01-2g-3g/` |
| `modules/06-cellular/02-4g-lte/` | `00-interview-prep/06-cellular-2g-6g/02-4g-lte/` |
| `modules/06-cellular/03-5g-6g/` | `00-interview-prep/06-cellular-2g-6g/03-5g-6g/` |
| `modules/06-cellular/04-mcptt-iwf/` | `00-interview-prep/06-cellular-2g-6g/04-mcptt-iwf/` |

### 3.9 Module 07 — LMR

| Old | New |
|-----|-----|
| `modules/07-lmr/01-p25-phase1/` | `00-interview-prep/07-land-mobile-radio/01-p25-phase1/` |
| `modules/07-lmr/02-p25-phase2/` | `00-interview-prep/07-land-mobile-radio/02-p25-phase2/` |
| `modules/07-lmr/03-dmr/` | `00-interview-prep/07-land-mobile-radio/03-dmr/` |
| `modules/07-lmr/04-tetra-dpmr-mpt/` | `00-interview-prep/07-land-mobile-radio/04-tetra-dpmr-mpt/` |
| `modules/07-lmr/05-system-iwf/` | `00-interview-prep/07-land-mobile-radio/05-system-iwf/` |
| `modules/07-lmr/06-builds/` | `00-interview-prep/07-land-mobile-radio/06-builds/` |

### 3.10 Module 08 — Embedded

| Old | New |
|-----|-----|
| `modules/08-embedded/01-bare-metal/` | `00-interview-prep/08-embedded-realtime/01-bare-metal/` |
| `modules/08-embedded/02-peripherals-dma/` | `00-interview-prep/08-embedded-realtime/02-peripherals-dma/` |
| `modules/08-embedded/03-freertos/` | `00-interview-prep/08-embedded-realtime/03-freertos/` |
| `modules/08-embedded/04-builds/` | `00-interview-prep/08-embedded-realtime/04-builds/` |

### 3.11 Module 09 — LeetCode

| Old | New |
|-----|-----|
| `modules/leetcode/01-bits/` | `00-interview-prep/09-leetcode/01-bits/` |
| `modules/leetcode/02-parsing/` | `00-interview-prep/09-leetcode/02-parsing/` |
| … through `13-dp/` | `00-interview-prep/09-leetcode/13-dp/` |

### 3.12 Capstone

| Old | New |
|-----|-----|
| `modules/capstone/M1-framing-fec/` | `00-interview-prep/10-capstone-ptt-gateway/M1-framing-fec/` |
| `modules/capstone/M2-rtp-jitter/` | `00-interview-prep/10-capstone-ptt-gateway/M2-rtp-jitter/` |
| `modules/capstone/M3-modem-ber/` | `00-interview-prep/10-capstone-ptt-gateway/M3-modem-ber/` |
| `modules/capstone/M4-grpc-floor/` | `00-interview-prep/10-capstone-ptt-gateway/M4-grpc-floor/` |

### 3.13 Empty dirs / cleanup

| Old | New |
|-----|-----|
| `networking/` | delete (empty) — pending Q4 |
| `waveforms/` | delete (empty) — pending Q4 |
| `modules/` | remove once empty after all `git mv`s |

### 3.14 Phases 1–4 (scaffold only after migrate)

Create pack folders with `pack.yaml` + `README.md` only (no worksheets yet), per your prompt under `10-foundations/`, `20-core-modelling/`, `30-advanced/`, `40-research/`.

### 3.15 Twins (create short pages; fill later)

`twins/{kalman-filter,hmm-viterbi,shannon-capacity-and-kelly,brownian-motion,ornstein-uhlenbeck,random-matrix-theory,pca-subspace-methods,hypothesis-testing-false-alarms,monte-carlo-rare-events,convex-optimisation-allocation,poisson-processes,adaptive-filters-online-regression,low-latency-cpp}.md`

---

## 4. Retrofit per moved pack (after `git mv`, no content rewrite of completed work)

For each of the 64 packs:

1. Add `pack.yaml` (id, title, phase 0, tier, status `not-started`, est_hours, prerequisites, twins, verify list from worksheet).
2. Add short `README.md` (objectives, prereq links, hours, twins).
3. Ensure answers stay only in `answers.md` (already true for these packs).
4. Normalize `code/` layout toward `include/` + `tests/` + `solutions/` (leetcode uses `headers/` today → either rename with `git mv` or document as acceptable alias; **proposal:** `git mv headers → include` where needed).
5. Pair notebooks with jupytext `.py`; strip outputs via pre-commit going forward.
6. Add “Other lens” stub links only where a twin is listed (no bulk rewrite).
7. Root CMake: `add_subdirectory`; label unimplemented stub tests as CTest labels `exercise` so CI can exclude them; solutions gated with `BUILD_SOLUTIONS`.

**Never delete or overwrite** anything in `include/` / attempted notes. If a file doesn’t fit, move to `legacy/` inside that pack and note it here.

---

## 5. This-week study priority (unchanged by folder names)

After migration, study in this order (paths under `00-interview-prep/`):

1. **02** bits-and-parsing  
2. **04** voice-over-packets  
3. **07** land-mobile-radio  
4. **09** leetcode  
5. **01** cpp-for-protocols  
6. **03-core** networking packs 01–04  

Then remaining interview packs. Deepenings (BER Q-function, Viterbi↔HMM, jitter-as-quantile) after those cores, as short new packs — **scaffold only until you ask to fill**, unless you want them filled this week.

Interview date constraint: **nothing in this plan removes or blocks access to existing worksheets**; moves are renames only.

---

## 6. Proposed commit sequence (after approval)

1. `Import sandboxes/communications interview packs (pre-migration baseline)`  
2. `Add learning-repo skeleton (STYLE, ROADMAP, templates, tools, CI stubs)`  
3. `git mv modules/* → 00-interview-prep/… (by module)` — one commit per module if cleaner  
4. `Retrofit pack.yaml + pack README + CMake root`  
5. `Scaffold phases 1–4 + twins`  
6. `Generate PROGRESS.md; write root README index`

---

## 7. Approval checklist

Reply with:

- [ ] Approve move map as written, **or** list path edits  
- [ ] Q1 root: `sandboxes/communications` / other: ________  
- [ ] Q2 keep sub-packs under modules: yes / no  
- [ ] Q3 DSP 05a–d grouping: yes / no  
- [ ] Q4 delete empty `networking`/`waveforms`: yes / no  
- [ ] Q5 leave `quant/` etc. outside: yes / no  
- [ ] Q6 standardize `BUILD_SOLUTIONS`: yes / no  
- [ ] Q7 git strategy A (baseline commit then mv) / B  

**No moves until you approve.**
