# Patella-Plus

Patella-Plus is [Patella-Modern](https://github.com/Coldensjo/Patella-Modern), a modernized source reconstruction of the
Tibia 7.72 client, with added features that differ from the original. Patella-Modern stays the faithful version; here
features are added one at a time. Patella-Plus has no automated tests, because features are tested by hand.

- **Targets:** Windows x64 and Linux x64.
- **Language and compilers:** C++23 (C++26 features only behind `src/compat/cxx26.h`); MSVC on Windows, GCC 16 on Linux.
- **Rendering:** Vulkan 1.3 on both platforms. A CPU Vulkan driver is the fallback.
- **Window, input and clipboard:** SDL3.

## No CipSoft files included

This repository contains **no CipSoft files**: no `Tibia.exe`, `Tibia.dat`, `Tibia.spr`, `Tibia.pic` or
`Tibia.cfg`, no data derived from them and no game server. Nothing built from it (packages, releases) contains them
either. To build or run the client, you need **your own copy of the Tibia 7.72 client files**. The build expects them
in the repository root, where they are gitignored.

## Building

The build is CMake with Ninja, one workflow preset per platform. On Windows it needs Visual Studio 2026 with the C++
workload, Python 3.13 on PATH, and `Tibia.exe` in the root (the resource pictures are extracted from it). The MSVC
presets need the compiler set up (`vcvarsall.bat`); `tools/build.py` does that and runs the workflow from any shell (a
developer prompt is not needed). It uses CMake 4.4.5 from `C:\Servers\Software\cmake`, or from the folder named by
`PATELLA_CMAKE_DIR`; `python -I tools/build.py --install-cmake <folder>` installs that version.

```
python -I tools/build.py windows-x64           # configure, build Release
python -I tools/build.py windows-x64-debug     # the same for Debug
python -I tools/build.py linux-x64-gcc         # the Linux client with GCC 16, Release
python -I tools/build.py linux-x64-gcc-debug   # the same for Debug
```

`python -I tools/build.py --env <preset> <command>` runs any other command (`cmake --build`) with that preset's compiler
set up.

The 64-bit client is `build\windows-x64\<Configuration>\Patella.exe` with its PDB. Run the client from a folder that
holds copies of the data files (for example `build\run`), never from the root, because the client writes its settings
(`Patella.ini`) and the automap next to the data. The client finds its data files in the working folder, else next to
`Patella.exe`, else in the folder of `--data <folder>` (which comes first) or of `DataFolder=<folder>` in a
`Patella.ini`; `Patella.ini` stays next to the data.

The Linux client is `build/linux-x64-gcc/<Configuration>/Patella`. On Linux, `cmake --workflow --preset linux-x64-gcc`
builds it, with GCC 16, CMake 3.30+, Ninja, Python 3 and SDL3's X11 and Wayland build dependencies (the Dockerfile below
lists them). On Windows, `tools/build.py` runs the Linux presets in a Docker image with
GCC 16.2, CMake 4.4.5 and Ninja 1.13.2, which you build once from
[docker/linux-x64/Dockerfile](docker/linux-x64/Dockerfile):

```
docker build -t patella-modern/linux-x64:gcc16.2-clang23.1-libstdcxx16-cmake4.4.5-ninja1.13.2-sdl3-mesa25.0.7 docker/linux-x64
```

The presets `package-windows-x64` (a zip) and `package-linux-x64` (a tarball and an AppImage) make the packages. They
build with neutral placeholder pictures only, so they need a folder without `Tibia.exe`. `python -I tools/package_check.py
<package>` proves a package holds no CipSoft file.

CI (GitHub Actions): `.github/workflows/hosted.yml` compiles the Windows and Linux clients with the placeholder pictures
on every push and pull request, without any CipSoft file.

## Building for your own server

If you build the client for your own server, you fix these values when you compile: the web links, the caption of Info's
website button, the four login servers and their port, the RSA modulus, the test server, the site named in the connection
errors and the first hint, and the bug report address. They are in one header, `src/BuildConfigDefaults.h`, with today's
values as its defaults.

1. Copy `src/BuildConfigDefaults.h` somewhere outside the repository, and change the values in your copy. Pulling updates
   then never conflicts with them.
2. Configure a build folder with your copy, and build it:

```
python -I tools/build.py --env windows-x64 cmake --preset windows-x64 -B build/own -DPATELLA_BUILD_CONFIG=C:/your/BuildConfig.h
python -I tools/build.py --env windows-x64 cmake --build build/own --config Release
```

A bad value stops the compile with a message that says what is wrong. Links must start with `http://` or `https://`, have
something after that, and hold no space or control character, because opening a link hands it to the operating system,
which would otherwise start a program named there. Host names are at most 99 characters, ports 1 to 65535, the modulus 306
to 309 decimal digits, the caption at most 49 characters (the button is 86 pixels wide, so keep it to about 17), and every
value is ASCII. There are always four login servers: if you have fewer, repeat one.

The values are fixed in the program: `Patella.ini` and the environment do not change them, except that `ServerAddress`,
`ServerPort` and `RsaModulus` in `Patella.ini` replace the login server and its key unless the header's
`ServerAddressOverride`, `ServerPortOverride` and `RsaModulusOverride` are set. They are plain text in the program, so
someone with a hex editor could still change them; only your server can enforce anything.

## License

MIT, see [LICENSE](LICENSE); [NOTICE](NOTICE) gives its scope. It covers this repository's own work; `src/Extern` keeps its own licenses, and
Tibia and its files belong to CipSoft GmbH and are not licensed here.
