"""Combines received drafts into a per-day writing log."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path

# draft-001-20260731T101500.md -> ("draft-001", "2026-07-31")
SNAPSHOT = re.compile(r"^(?P<base>.+)-(?P<date>\d{8})T\d{6}\.md$")


def group_by_day(folder: Path) -> dict[str, list[Path]]:
    days: dict[str, list[Path]] = defaultdict(list)
    for path in sorted(folder.glob("*.md")):
        m = SNAPSHOT.match(path.name)
        if not m:
            continue
        d = m.group("date")
        days[f"{d[0:4]}-{d[4:6]}-{d[6:8]}"].append(path)
    return dict(days)


def latest_per_base(paths: list[Path]) -> list[Path]:
    """Snapshots sort lexically; the last one per base draft wins."""
    by_base: dict[str, Path] = {}
    for path in sorted(paths):
        m = SNAPSHOT.match(path.name)
        if m:
            by_base[m.group("base")] = path
    return [by_base[k] for k in sorted(by_base)]


def export_daily_log(folder: Path, out_dir: Path) -> list[Path]:
    """Writes one `writing-log-YYYY-MM-DD.md` per day of snapshots."""
    out_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []
    for day, paths in sorted(group_by_day(folder).items()):
        parts = [f"# Writing log {day}", ""]
        total_words = 0
        for path in latest_per_base(paths):
            text = path.read_text(encoding="utf-8")
            words = len(text.split())
            total_words += words
            base = SNAPSHOT.match(path.name).group("base")
            parts += [f"## {base} ({words} words)", "", text.rstrip(), ""]
        parts.insert(1, f"\nTotal: {total_words} words across {len(latest_per_base(paths))} drafts\n")
        out = out_dir / f"writing-log-{day}.md"
        out.write_text("\n".join(parts), encoding="utf-8")
        written.append(out)
    return written
