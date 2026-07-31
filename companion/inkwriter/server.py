"""A tiny draft sync server: accepts HTTP PUT into a local folder.

The device uploads conflict-safe snapshot names (`draft-001-20260731T101500.md`),
so the server only ever creates files; it never overwrites and never merges.
"""

from __future__ import annotations

import re
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

SAFE_NAME = re.compile(r"^[A-Za-z0-9._-]+\.md$")


def make_handler(root: Path):
    class Handler(BaseHTTPRequestHandler):
        def do_PUT(self):  # noqa: N802 (http.server naming)
            name = Path(self.path).name
            if not SAFE_NAME.match(name):
                self.send_error(400, "bad draft name")
                return
            target = root / name
            if target.exists():
                # Snapshot names never repeat; a repeat means a client retry.
                self.send_response(200)
                self.end_headers()
                return
            length = int(self.headers.get("Content-Length", 0))
            if length <= 0 or length > 4_000_000:
                self.send_error(411, "missing or oversized body")
                return
            body = self.rfile.read(length)
            root.mkdir(parents=True, exist_ok=True)
            target.write_bytes(body)
            self.send_response(201)
            self.end_headers()

        def log_message(self, fmt, *args):  # quiet by default
            pass

    return Handler


def serve(root: Path, host: str = "0.0.0.0", port: int = 8377) -> ThreadingHTTPServer:
    server = ThreadingHTTPServer((host, port), make_handler(root))
    return server
