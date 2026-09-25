Oblivion browser build
======================

This directory is a self-contained static web build. It must be served over
HTTP; browsers will not reliably load its WebAssembly from a file: URL.

For a local preview with Python 3:

  python serve_web.py

Then open http://127.0.0.1:8000/. The server binds only to loopback.

For deployment, upload index.html, limbo_web_cc.js, and
limbo_web_cc.wasm together without renaming them. Configure the server to
send .wasm as application/wasm.

The page asks for a supported original Oblivion mobile JAR on first use. It
verifies the JAR, stores only extracted resources in browser-local IndexedDB,
and reuses those resources on later visits. The selected JAR is not uploaded
by this build. Clearing site data removes the extracted resources and saves.

The session log is stored with saves at /limbo/saves/rms/limbo.log
and synchronized to IndexedDB. The previous session is limbo.log.prev.
Use Save log on the page to download the current log when reporting a problem.

No original game data is included. See COPYING and THIRD_PARTY_NOTICES.md
plus licenses/ for distribution terms.
