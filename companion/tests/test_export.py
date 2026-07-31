from pathlib import Path

from inkwriter import export
from inkwriter.cli import main


def _snap(folder: Path, base, stamp, text):
    (folder / f"{base}-{stamp}.md").write_text(text)


def test_export_groups_days_and_takes_latest(tmp_path):
    drafts = tmp_path / "drafts"
    drafts.mkdir()
    _snap(drafts, "draft-001", "20260731T090000", "early words here")
    _snap(drafts, "draft-001", "20260731T180000", "final words here now")
    _snap(drafts, "draft-002", "20260731T120000", "second draft")
    _snap(drafts, "draft-001", "20260730T120000", "yesterday text")
    (drafts / "notes.md").write_text("not a snapshot")

    out = tmp_path / "logs"
    written = export.export_daily_log(drafts, out)
    names = [p.name for p in written]
    assert names == ["writing-log-2026-07-30.md", "writing-log-2026-07-31.md"]
    log31 = (out / "writing-log-2026-07-31.md").read_text()
    assert "final words here now" in log31
    assert "early words" not in log31          # superseded snapshot dropped
    assert "second draft" in log31
    assert "Total: 6 words across 2 drafts" in log31


def test_cli_export(tmp_path, capsys):
    drafts = tmp_path / "d"
    drafts.mkdir()
    _snap(drafts, "a", "20260731T000001", "one two")
    rc = main(["export", "--root", str(drafts), "--out", str(tmp_path / "o")])
    assert rc == 0
    assert "writing-log-2026-07-31.md" in capsys.readouterr().out
