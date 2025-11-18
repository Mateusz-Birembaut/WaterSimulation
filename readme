# WaterSimulation

Prerequisites
-------------

Before building and running the project, make sure you have:

- Git
- CMake (>= 3.10)
- A modern C++ compiler (g++ or clang)
- SDL2 development headers (see Dependencies section)

Cloning the repository
----------------------

Clone the repository and initialize its submodules (Magnum, Corrade):

```bash
git clone --recurse-submodules <repo-url>
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

Submodules
----------

Third-party libraries are placed in `external/` as Git submodules (Magnum,
Corrade). These sources are required to build the project.

Dependencies — SDL2
-------------------

Magnum uses `Sdl2Application`. Install the SDL2 development headers for your
platform:

Ubuntu / Debian:

```bash
sudo apt update
sudo apt install libsdl2-dev
```

Fedora / RHEL (dnf):

```bash
sudo dnf install SDL2-devel
```

Arch / Manjaro:

```bash
sudo pacman -Syu sdl2
```


Build & Run
-----------

After installing dependencies and initializing submodules, build and run
the application:

```bash
# Configure build (Debug)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build the WaterSimulation target
cmake --build build --target WaterSimulation -- -j$(nproc)

# Run the executable
./build/Debug/bin/WaterSimulation
```

For a Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target WaterSimulation -- -j$(nproc)
```

VS Code — Tasks & Debugging
--------------------------

The repository includes VS Code tasks and launch configurations in `.vscode/`:

- `tasks.json`: configure, build, run
- `launch.json`: debug configurations (gdb and lldb)

Useful shortcuts:

- `F5` — start the debugger (pre‑build is configured via `preLaunchTask`)
- `Ctrl+F5` / `⌘+F5` — start without debugging

Useful commands
---------------

- Clone with submodules:

```bash
git clone --recurse-submodules <repo-url>
```

- Initialize submodules if you cloned without them:

```bash
git submodule update --init --recursive
```

- Update submodules to the referenced commits (or follow remote):

```bash
git submodule update --remote --merge
```

Troubleshooting
---------------

- "SDL2 not found" — install `libsdl2-dev` (or the equivalent for your OS)
  and re-run `cmake`.
- `external/` is empty or files are missing -> run `git submodule update --init --recursive`.
- Executable not found after building -> check `build/Debug/bin` or `build/Release/bin`.

