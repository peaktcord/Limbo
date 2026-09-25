# Third-party notices

Limbo is Copyright (C) 2026 Peakt and is free software licensed under the GNU
General Public License, version 3 or (at your option) any later version. See
`COPYING` for the complete license. The following third-party components
retain their own licenses.

Limbo ships no original game data. Running it requires your own legally
obtained copy of a supported original Oblivion mobile JAR, which the
application verifies before extracting its resources into local storage.

The dependency versions below are pinned in `MODULE.bazel`. Each package built
by `bazel build //:dist` carries these texts in its `licenses/` directory.

## SDL 3.4.14

Simple DirectMedia Layer (SDL) is copyright (C) 1997-2026 Sam Lantinga and is
distributed under the zlib license.

- Source: <https://github.com/libsdl-org/SDL/tree/release-3.4.14>
- License identifier: `Zlib`
- Full terms: `licenses/SDL3.txt`, supplied by `@decomp_sdl3//:license`
  directly from the pinned SDL release archive.

## zlib 1.3.1

zlib is copyright (C) 1995-2024 Jean-loup Gailly and Mark Adler, distributed
under the zlib license.

- Source: <https://github.com/madler/zlib>
- License identifier: `Zlib`
- Full terms: `licenses/zlib.txt`, supplied by `@zlib//:LICENSE`.

## stb_image and stb_image_write

This project uses `stb_image.h` and `stb_image_write.h` from
<https://github.com/nothings/stb>. The upstream files are offered under a
choice of public-domain terms or the MIT license. This project distributes
them under the MIT option.

- License identifier: `MIT`
- Full terms: `licenses/stb.txt`
- The original license notices are also retained verbatim in the vendored
  headers under `third_party/stb/`.

## Emscripten SDK 6.0.8

The browser build is compiled with the pinned Emscripten SDK, which links
LLVM `libc++`, `compiler-rt`, and musl into the WebAssembly output. These
notices apply to the web package only; the native package omits them.

- Source: <https://github.com/emscripten-core/emsdk>
- License identifiers: `MIT`, `Apache-2.0 WITH LLVM-exception`
- Full terms: `licenses/Emscripten.txt`, `licenses/LLVM-libcxx.txt`,
  `licenses/LLVM-compiler-rt.txt`, and `licenses/musl.txt`, copied from the
  corresponding files in the pinned SDK.
