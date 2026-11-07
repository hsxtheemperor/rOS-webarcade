#!/usr/bin/env python3
import argparse
import hashlib
import json
import threading
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit


class BinWatcher:
    def __init__(self, directory: Path, interval: float = 0.2, settle: float = 0.4):
        self.directory = directory
        self.interval = interval
        self.settle = settle
        self._lock = threading.Lock()
        self._version = self._signature()
        self._candidate = None
        self._candidate_since = 0.0
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._thread.start()

    def version(self) -> str:
        with self._lock:
            return self._version

    def _signature(self) -> str:
        digest = hashlib.sha256()
        if not self.directory.exists():
            return digest.hexdigest()

        for path in sorted(item for item in self.directory.rglob("*") if item.is_file()):
            relative = path.relative_to(self.directory).as_posix().encode("utf-8")
            stat = path.stat()
            digest.update(len(relative).to_bytes(4, "little"))
            digest.update(relative)
            digest.update(stat.st_size.to_bytes(8, "little"))
            digest.update(stat.st_mtime_ns.to_bytes(8, "little", signed=True))
            digest.update(stat.st_ino.to_bytes(8, "little"))
        return digest.hexdigest()

    def _run(self) -> None:
        while True:
            time.sleep(self.interval)
            try:
                current = self._signature()
            except OSError as error:
                print(f"[rOS dev] could not scan vdisk bin: {error}", flush=True)
                continue
            now = time.monotonic()
            if current == self.version():
                self._candidate = None
                continue
            if current != self._candidate:
                self._candidate = current
                self._candidate_since = now
                continue
            if now - self._candidate_since < self.settle:
                continue

            with self._lock:
                previous = self._version
                self._version = current
            self._candidate = None
            if current != previous:
                print("[rOS dev] vdisk/vdisk01/bin changed; browser will reload", flush=True)


class DevRequestHandler(SimpleHTTPRequestHandler):
    server_version = "rOSDev/1.0"

    def log_message(self, format: str, *args: object) -> None:
        if urlsplit(self.path).path == "/__dev/vdisk-bin-version":
            return
        super().log_message(format, *args)

    @property
    def watcher(self) -> BinWatcher:
        return self.server.watcher

    def do_GET(self) -> None:
        path = urlsplit(self.path).path
        if path == "/__dev/vdisk-bin-version":
            payload = json.dumps({"version": self.watcher.version()}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if path == "/service-worker.js":
            service_worker = (Path(self.directory) / "service-worker.js").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "application/javascript; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(service_worker)))
            self.end_headers()
            self.wfile.write(service_worker)
            return

        if path in ("/", "/index.html"):
            index = (Path(self.directory) / "index.html").read_bytes()
            marker = b"</head>"
            script = b"""
<script>
window.__ROS_DEV__ = true;
(() => {
  let version;
  async function checkVdiskBin() {
    try {
      const response = await fetch("/__dev/vdisk-bin-version", { cache: "no-store" });
      if (!response.ok) throw new Error(`watcher returned HTTP ${response.status}`);
      const current = (await response.json()).version;
      if (version === undefined) version = current;
      else if (current !== version) {
        console.info("[rOS dev] vdisk bin changed; reloading");
        location.reload();
        return;
      }
    } catch (error) {
      console.warn("[rOS dev] vdisk watcher check failed", error);
    }
    window.setTimeout(checkVdiskBin, 500);
  }
  checkVdiskBin();
})();
</script>
"""
            if marker not in index:
                self.send_error(500, "index.html is missing </head>")
                return
            index = index.replace(marker, script + marker, 1)
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(index)))
            self.end_headers()
            self.wfile.write(index)
            return

        super().do_GET()


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve rOS with vdisk Wasm live reload.")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    root = args.root.resolve()
    bin_directory = root / "vdisk" / "vdisk01" / "bin"
    watcher = BinWatcher(bin_directory)
    watcher.start()
    handler = lambda *handler_args, **handler_kwargs: DevRequestHandler(
        *handler_args, directory=str(root), **handler_kwargs
    )
    server = ThreadingHTTPServer((args.host, args.port), handler)
    server.watcher = watcher
    print(f"[rOS dev] serving {root} at http://{args.host}:{args.port}/", flush=True)
    print(f"[rOS dev] watching {bin_directory} for changes", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[rOS dev] stopping", flush=True)
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
