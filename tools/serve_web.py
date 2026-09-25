from __future__ import annotations

import argparse
import functools
import http.server
from pathlib import Path


class NoCacheHandler(http.server.SimpleHTTPRequestHandler):

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store, must-revalidate")
        super().end_headers()

    def send_header(self, keyword: str, value: str) -> None:
        if keyword.lower() == "last-modified":
            return
        super().send_header(keyword, value)

    def send_head(self):
        del self.headers["If-Modified-Since"]
        del self.headers["If-None-Match"]
        return super().send_head()


def main() -> int:
    script_directory = Path(__file__).resolve().parent
    packaged_page = script_directory / "index.html"
    repository = script_directory.parent
    default_directory = (
        script_directory
        if packaged_page.is_file()
        else repository / "bazel-bin/limbo_web"
    )
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument(
        "--directory",
        type=Path,
        default=default_directory,
    )
    args = parser.parse_args()
    root = args.directory.resolve()
    page = root / "index.html"
    if not page.is_file():
        parser.error(
            f"{page} does not exist; run bazelisk build //:limbo_web first "
            "or pass --directory"
        )

    handler = functools.partial(NoCacheHandler, directory=str(root))
    server = http.server.ThreadingHTTPServer(("127.0.0.1", args.port), handler)
    print(f"Serving {page.name} at http://127.0.0.1:{args.port}/{page.name}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
