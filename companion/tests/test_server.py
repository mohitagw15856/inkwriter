import threading
import urllib.request

from inkwriter import server


def _put(url, data):
    req = urllib.request.Request(url, data=data, method="PUT")
    try:
        with urllib.request.urlopen(req, timeout=5) as resp:
            return resp.status
    except urllib.error.HTTPError as e:
        return e.code


def test_put_creates_and_never_overwrites(tmp_path):
    srv = server.serve(tmp_path, host="127.0.0.1", port=0)
    port = srv.server_address[1]
    t = threading.Thread(target=srv.serve_forever, daemon=True)
    t.start()
    try:
        url = f"http://127.0.0.1:{port}/draft-001-20260731T101500.md"
        assert _put(url, b"first words") == 201
        assert (tmp_path / "draft-001-20260731T101500.md").read_bytes() == b"first words"
        # Retry of the same snapshot: acknowledged, not overwritten.
        assert _put(url, b"changed") == 200
        assert (tmp_path / "draft-001-20260731T101500.md").read_bytes() == b"first words"
        # Traversal cannot escape the root: the name is flattened to its
        # basename, so the file lands inside root, never beside it.
        assert _put(f"http://127.0.0.1:{port}/%2e%2e/evil.md", b"x") in (201, 400)
        assert not (tmp_path.parent / "evil.md").exists()
        # Names without the .md extension are rejected outright.
        assert _put(f"http://127.0.0.1:{port}/no-extension", b"x") == 400
    finally:
        srv.shutdown()
        srv.server_close()
