# Emscripten Bazel compatibility patch

`MODULE.bazel` pins the Bazel integration from Emscripten SDK 6.0.8 to the
official release archive with integrity
`sha256-O7bJn7B0BfTPBUdDwaINZax8G3CgRTEhY3VCa5fJMwo=`. The local patch keeps
that toolchain hermetic on Windows while adding the SDL3 WASM port:

- create the secondary cache's pkg-config directory before Emscripten's
  `embuilder` source-builds SDL;
- make Clang compile against the declared secondary sysroot, where port headers
  are installed;
- pin Emscripten's SDL port to release 3.4.14, matching `rules_decomp` and the
  Raingrasp native dependency. Its official source ZIP has SHA-512
  `02da1bdd168ca79e9718d7e0244ee1ad05b413363a56259385e2ef02f8f54a41d836d3cb995a1018123bb78ad7bf7ee39c7fff193ab5a369ed18d9b3d58a13bc`.

To rebase, update the exact Emscripten release and archive integrity in
`MODULE.bazel`, refresh the patch against that release's `bazel/` directory,
then update the nested SDL version and SHA-512 only after verifying the
official SDL release archive. Keep the cache target list minimal by adding only
libraries named by an actual link failure.

Verify the result from the repository root:

```powershell
bazelisk mod tidy
bazelisk build //:limbo_wasm_probe
node bazel-bin/limbo_wasm_probe/limbo_wasm_probe_cc.js
bazelisk test //:test_fast //:test_private_jars --nocache_test_results
```
