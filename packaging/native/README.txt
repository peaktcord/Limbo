Oblivion native build
=====================

Launch limbo_sdl.exe directly, or use run.cmd. On first launch, drop the
original Oblivion mobile JAR onto the game's prompt screen. The game verifies
it and stores the extracted resources in %LOCALAPPDATA%\Limbo\oblivion
for later launches. No original game data is included in this package.

Command line:

  limbo_sdl.exe [game.jar] [--scale N] [--cache DIR] [--saves DIR]
                   [--portable] [--install-only]

Saves and the session log are in %LOCALAPPDATA%\Limbo\saves\rms.
--portable, or portable.txt beside the executable, uses data\ beside the
executable for resources and saves. --saves DIR overrides the save/log folder.
Delete the extracted oblivion directory to require the original JAR again.

Diagnostics go to limbo.log in the save folder. The previous launch's log
is kept as limbo.log.prev. If the game fails, retain both files; they
include the build identity, recent operations, and available crash context.
The game does not write routine diagnostics to a console.

See CONTROLS.txt for input, and COPYING plus THIRD_PARTY_NOTICES.md and
licenses/ for distribution terms.
