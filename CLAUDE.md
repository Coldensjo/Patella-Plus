# Patella-Plus

This repository is [Patella-Modern](https://github.com/Coldensjo/Patella-Modern), the faithful modernized reconstruction
of the Tibia 7.72 client, with added features that differ from the original, one at a time. It has no automated tests and
no documents besides README.md and this file. Sources are in `src/`, the build in `cmake/`, `CMakePresets.json` and `tools/`.

## Building

From any shell. `tools/build.py` sets up MSVC, runs the pinned CMake 4.4.5 and fails on any compiler or linker warning.

```
python -I tools/build.py windows-x64           # Release: build\windows-x64\Release\Patella.exe
python -I tools/build.py windows-x64-debug     # Debug
python -I tools/build.py linux-x64-gcc         # Release, in the Docker image of docker/linux-x64
python -I tools/build.py linux-x64-gcc-debug   # Debug
python -I tools/build.py package-windows-x64   # the zip, only in a folder without Tibia.exe
python -I tools/build.py package-linux-x64     # the tarball and AppImage, only in a folder without Tibia.exe
python -I tools/build.py --env <preset> <command>   # any other command in that preset's environment
```

`python -I tools/repro_check.py [<commit>]` proves this checkout's Release client is that commit's, byte for byte.
`python -I tools/source_check.py comments|names|rename|format` proves a change is only comments or only renames, renames,
and checks the style of `.clang-format`. A worktree has no `Tibia.exe`: configure it with `-DPATELLA_TIBIA_EXE=<path>`.

The normal builds read the local `Tibia.exe` for the pictures. `-DPATELLA_PLACEHOLDER_RESOURCES=ON` builds neutral ones and
is refused where a `Tibia.exe` exists. `python -I tools/package_check.py <package>` proves a package holds no CipSoft file.

## Where values live

- `src/BuildConfigDefaults.h`: what the builder fixes at compile time (web links, login servers and port, RSA modulus,
  test server, the site named in texts, the bug report address). A developer's own copy is `-DPATELLA_BUILD_CONFIG=<file>`,
  and the `static_assert`s of `src/BuildConfig.h` check it.
- `Patella.ini` (`src/settings/`): what players may change.

## Rules

- CipSoft files (Tibia.exe, Tibia.dat, Tibia.spr, Tibia.pic, Tibia.cfg and anything derived from them) are never committed, pushed or uploaded.
- No client is run on the owner's desktop by an agent.
- Stage files by exact name.
- Indent with tabs, except where a format or `.editorconfig` requires spaces.
- No tests or docs are added.
- A feature's agent proves it compiles, then the owner tries it and says when it merges.
