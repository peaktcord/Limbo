# Vendored third-party sources

Single-file libraries checked in verbatim. Record provenance here when
adding one.

| File | Version | Source | SHA-256 |
|---|---|---|---|
| `stb_image_write.h` | v1.16 | [nothings/stb](https://github.com/nothings/stb) | `cbd5f0ad7a9cf4468affb36354a1d2338034f2c12473cf1a8e32053cb6914a05` |
| `stb_image.h` | v2.30 | [nothings/stb @ f58f558](https://github.com/nothings/stb/blob/f58f558c120e9b32c217290b80bad1a0729fbb2c/stb_image.h) | `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3` |

`stb_image.h` decodes the game's PNG art for the software rasterizer;
`stb_image_write.h` saves captured frames. Both stb headers are public domain
(MIT alternative).

Both files were copied from the Raingrasp port of the same publisher's
earlier title; the hashes above are of the copies in this tree.
