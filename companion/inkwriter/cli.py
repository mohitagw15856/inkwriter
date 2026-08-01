"""Command line interface: `inkwriter serve` and `inkwriter export`."""

from __future__ import annotations

import argparse
from pathlib import Path

from . import export as export_mod
from . import server as server_mod


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="inkwriter",
                                     description="InkWriter draft tools")
    sub = parser.add_subparsers(dest="command", required=True)

    s = sub.add_parser("serve", help="receive drafts over HTTP PUT")
    s.add_argument("--root", type=Path, default=Path("drafts"))
    s.add_argument("--host", default="0.0.0.0")
    s.add_argument("--port", type=int, default=8377)

    e = sub.add_parser("export", help="combine snapshots into daily logs")
    e.add_argument("--root", type=Path, default=Path("drafts"))
    e.add_argument("--out", type=Path, default=Path("logs"))

    args = parser.parse_args(argv)

    if args.command == "serve":
        srv = server_mod.serve(args.root, args.host, args.port)
        print(f"inkwriter: receiving drafts into {args.root} on "
              f"http://{args.host}:{args.port} (Ctrl+C stops)")
        try:
            srv.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            srv.server_close()
        return 0

    if args.command == "export":
        written = export_mod.export_daily_log(args.root, args.out)
        for path in written:
            print(path)
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
