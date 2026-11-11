# Communications learning repo

Interview prep for Etherstack (Phase 0) plus a multi-year modelling roadmap (Phases 1–5).

**Interviews ~17 Nov 2026 — study `00-interview-prep/` first.** Long-term folders are scaffolds only.

| Doc | Purpose |
|-----|---------|
| [STYLE.md](STYLE.md) | First-principles teaching rules |
| [ROADMAP.md](ROADMAP.md) | Phases 0–5 and “done when” |
| [PROGRESS.md](PROGRESS.md) | Generated pack table (do not hand-edit) |
| [MIGRATION.md](MIGRATION.md) | Move map from the old `modules/` layout |
| [twins/](twins/) | Same maths, comms ↔ quant lenses |
| [../quant/](../quant/) | Separate quant notebooks/data sandbox |

## Phase map

| Phase | Folder | Status |
|------:|--------|--------|
| 0 | [00-interview-prep/](00-interview-prep/) | **Filled** — 64 packs + deepenings scaffolds |
| 1 | [10-foundations/](10-foundations/) | Scaffold |
| 2 | [20-core-modelling/](20-core-modelling/) | Scaffold |
| 3 | [30-advanced/](30-advanced/) | Scaffold |
| 4–5 | [40-research/](40-research/) | Scaffold |

Also retained for later work: [networking/](networking/), [waveforms/](waveforms/).

## This week (interview priority)

Under `00-interview-prep/`: **02 → 04 → 07 → 09 → 01 → 03-core (net 01–04)**, then the rest.  
Detail: [00-interview-prep/STUDY_PATH.md](00-interview-prep/STUDY_PATH.md).

## How a pack works

Each topic folder has `pack.yaml`, `README.md`, `worksheet` / answers / cheatsheet, and `code/` (stubs vs `solutions/`).

```bash
# one pack
cmake -S 00-interview-prep/02-bits-and-parsing/01-bit-ops/code -B build-pack
cmake --build build-pack && ctest --test-dir build-pack --output-on-failure

# reference solutions (spoiler)
cmake -S …/code -B build-pack -DUSE_SOLUTIONS=ON
```

Root build (all packs that register CMake):

```bash
cmake -B build -DUSE_SOLUTIONS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Python env: `pip install -e .` from this directory (`pyproject.toml`).

Scaffold a new pack: `python tools/new_pack.py 10-foundations 06-new-topic "Title"`  
Refresh progress: `python tools/progress.py --write`

## Legal

Synthetic signals and published test vectors only. No live public-safety radio interception (UK Wireless Telegraphy Act 2006).
