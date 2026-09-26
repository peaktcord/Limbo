# Limbo

A C++ recreation of the *The Elder Scrolls IV: Oblivion* feature phone game. It aims for an authentic experience by default, but with some optional modernizations. This fixes any bugs I noticed in the original.

This does not include any game assets. You will need to have your own copy of the original mobile game `.jar` files.

It's also available as a web version [here](https://peaktcord.github.io/Limbo/). A `.jar` file is still required.

## Modernizations

This uses controls designed for a keyboard, not a phone pad.

There is also a widescreen mode and several other adjustments that can be enabled in the Port Options menu, such as eight way movement and screen-relative movement. These are disabled by default to favor an authentic experience.

Help text is not updated with the more modern controls.

## Running the game

The game will prompt you for jar files on boot. Drag them onto the window to use them. You only need to do this once per game.

This launches directly into the last game you played. You can switch games with 'Game Select' on the menu.

Save data and jar unpacks are written to `%LOCALAPPDATA%\Limbo\`.

From the repository, build and launch the game with:

```
bazelisk run //:limbo_sdl
```

## Building

This project uses Bazel to build and test. This is untested on anything other than Windows.

It uses MSVC as the compiler.

```
bazelisk build //:dist
bazelisk test //:test_fast
bazelisk test //:test_portable
```

Build for release with the following commands. This produces `limbo-windows.zip` and `limbo-web.zip`.

```
bazelisk build -c opt //:dist
powershell -Command "Compress-Archive -Path bazel-bin/limbo_windows_package/* -DestinationPath limbo-windows.zip -Force"
powershell -Command "Compress-Archive -Path bazel-bin/limbo_web_package/* -DestinationPath limbo-web.zip -Force"
```

### Running playthrough tests

To run the more extensive tests, you need the jar files. Place `tes-oblivion-mobile-1.0.10.jar` in `artifacts/private/jars/`, then:

```
bazelisk test //:test_private_jars
```

## Disclaimer

Limbo is an unofficial, fan-made project. It is not affiliated with,
authorized by, endorsed by, or in any way associated with Bethesda Softworks
LLC, ZeniMax Media Inc., Vir2L Studios, or any of their subsidiaries or
affiliates. *The Elder Scrolls*, *Oblivion*, and all related
names and marks are trademarks of their respective owners, used here only to
identify the games this project recreates.

This project ships no game assets. Running it requires your own legally
obtained copies of the original `.jar` files.

## License

Copyright (C) 2026 Peakt.

Limbo is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. See [`COPYING`](COPYING) for the complete license and
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the separately licensed
third-party components.
