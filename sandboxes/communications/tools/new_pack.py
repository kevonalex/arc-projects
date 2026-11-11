#!/usr/bin/env python3
"""Scaffold a pack from templates/pack into a phase folder."""

from __future__ import annotations

import argparse
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / "templates" / "pack"


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("phase_folder", help="e.g. 10-foundations or 00-interview-prep/02-bits-and-parsing")
    ap.add_argument("nn_name", help="e.g. 01-linear-algebra")
    ap.add_argument("title", help='e.g. "Linear algebra"')
    args = ap.parse_args()

    dest = ROOT / args.phase_folder / args.nn_name
    if dest.exists():
        raise SystemExit(f"already exists: {dest}")

    shutil.copytree(TEMPLATE, dest)

    phase_match = re.match(r"^(\d+)", Path(args.phase_folder).parts[0])
    phase = int(phase_match.group(1)) if phase_match else 0
    # interview-prep is phase 0 even though folder is 00-
    if Path(args.phase_folder).parts[0].startswith("00-"):
        phase = 0

    replacements = {
        "{{title}}": args.title,
        "{{phase}}": str(phase),
        "{{tier}}": "1",
        "{{est_hours}}": "3",
        "NN-topic-name": args.nn_name,
        "Topic title": args.title,
    }

    for path in dest.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() in {".png", ".jpg", ".zip"}:
            continue
        text = path.read_text(encoding="utf-8")
        for old, new in replacements.items():
            text = text.replace(old, new)
        path.write_text(text, encoding="utf-8")

    print(f"created {dest.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
