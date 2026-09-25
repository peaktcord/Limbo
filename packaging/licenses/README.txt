Licenses
========

COPYING is the GNU General Public License, version 3, which covers Limbo.
THIRD_PARTY_NOTICES.md summarizes the components below and their licenses.

The dependency versions are pinned in MODULE.bazel.

- SDL3.txt is supplied by @decomp_sdl3//:license (SDL 3.4.14).
- zlib.txt is supplied by @zlib//:LICENSE (zlib 1.3.1).
- stb.txt is the license carried by third_party/stb/stb_image.h and
  third_party/stb/stb_image_write.h.
- Emscripten.txt, LLVM-libcxx.txt, LLVM-compiler-rt.txt, and musl.txt are copied
  from the corresponding license files in the pinned Emscripten SDK 6.0.8.

The LLVM and musl notices are relevant to the WebAssembly runtime linked into
the browser build. The native package intentionally omits those web-only
notices.
