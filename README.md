# GravityRush-Reshifted

Gravity Rush Remastered (PS4, 2015) as a native Linux program, through
static recompilation with
[SwARc](https://github.com/JTM-rootstorm/SwARc): the game's x86-64
machine code is lifted to C++ ahead of time and runs on reimplemented
PS4 system libraries (SwARc's Orbis platform module). There is no
emulator in the result, and no game code or data in this repository:
bring your own decrypted copy of the game, dumped from your own console.

It is the first PS4 game on SwARc and the test case the module is built
against; what it needs goes into the module, not into this repository.

## Status

In game. From the title through New Game, the opening movie and the
first dialogue scene into free play in episode 1's city, with sound and
its characters drawn whole, at about 200 fps. The first mission plays
to the first autosave, and Continue loads it. Not yet checked: the
city's sound by ear. [FINDINGS.md](FINDINGS.md) has the detail.

## What you need

- Gravity Rush Remastered, CUSA01130, version 01.00, decrypted from
  your own console: its `app0` and its `param.sfo`.
- Linux x86-64 with a Vulkan driver.
- A SwARc checkout next to this repository's parent (`../../SwARc`), or
  anywhere with `-DSWARC_DIR=...`. Its README lists what it needs to
  build (Clang, CMake 3.25+, Ninja, SDL3, ffmpeg, FreeType, fontconfig,
  glslang).

## Building

```sh
cmake --preset release
cmake --build --preset release
tools/lift.sh "$APP0"
cmake --preset release
cmake --build --preset release
```

The first build makes the lifter and the loader; `tools/lift.sh` then
lifts the game's executable and the libraries it ships into
`generated/` (`generated/lift_report.txt` lists what was left
unlifted), and the second build compiles them into `bin/grr`. Lift
again after updating SwARc's lifter or the game.

## Running

```sh
out/build/release/bin/grr --game-dir "$APP0" --param-sfo "$PARAM_SFO"
```

Save data goes in `$XDG_DATA_HOME/grr` (else `~/.local/share/grr`).
`GRR_TRACE=1` logs every system library call. Anything the module has
not implemented stops the game with a message naming it, rather than
guessing.

## Licence

MIT (see [LICENSE](LICENSE)). The game is not included and is not
covered by it.
