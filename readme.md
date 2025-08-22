# godot-gis-starter (Conan edition)

Conan-based setup for a Godot 4.3/4.4 GDExtension that links to GDAL/OGR/PROJ.

## Quick start (Conan 2.x)

```bash
# 1) Add godot-cpp submodule (match your editor version)
git submodule add https://github.com/godotengine/godot-cpp.git extern/godot-cpp
git submodule update --init --recursive
# checkout branch 4.4 or 4.3 to match your Godot editor

# 2) Detect a default profile
conan profile detect --force

# 3) Install deps (RelWithDebInfo example)
conan install . --output-folder=build --build=missing -s build_type=RelWithDebInfo

# 4) Configure CMake with Conan toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake

# 5) Build
cmake --build build --config RelWithDebInfo
```

> The Conan generators place package config files in `build/generators`. The CMakeLists already adds this folder to `CMAKE_PREFIX_PATH` so `find_package(gdal CONFIG REQUIRED)` resolves and exposes `gdal::gdal`.

## Notes
- `conanfile.txt` pins `gdal/3.*` and `proj/9.*`. Adjust versions if you need a specific one.
- GDAL typically pulls PROJ as a dependency, but adding `proj` explicitly makes it available as `proj::proj`.
- If you prefer dynamic GDAL (`shared=True`), flip the options.
- On Windows/MSVC, the default profile uses the MD runtime (dynamic). Change via:
  ```bash
  conan profile update 'conf.tools.cmake.cmaketoolchain:msvc_runtime_type=static' default
  ```
  or pass it on the command line.

## Godot integration
- After build, copy the produced library from `build/bin/` to `demo/bin/` and open `demo/` in the editor.
- The `godot-gis.gdextension` file expects platform-specific names; adjust as needed.
