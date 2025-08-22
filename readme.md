# GodEarth 🌍

**GodEarth** est un moteur et un outil **SIG (Système d’Information Géographique)** basé sur [Godot Engine](https://godotengine.org/).  
L’objectif est de fournir un module **GDExtension** qui permet d’intégrer des fonctionnalités GIS avancées directement dans Godot, en s’appuyant notamment sur la bibliothèque [GDAL](https://gdal.org/).

---

## ⚙️ Pré-requis

Avant de commencer, assurez-vous d’avoir installé :

- **Python** ≥ 3.9 (nécessaire pour Conan et SCons)  
- **Conan** ≥ 2.x  
- **SCons** ≥ 4.x  
- **CMake** ≥ 3.24  
- **MSVC** (Visual Studio 2022 recommandé)  
- **Git** (pour les submodules)

---

## 📂 Setup du projet

Cloner le projet avec ses sous-modules :

```bash
git clone https://github.com/<ton-user>/godearth.git
cd godearth
git submodule update --init --recursive


# 🔨 Procédure de build

```
# Clean
rm -rf build

# Generation de extension_api.json
godot.exe --headless --dump-extension-api build/extension_api.json
```

👉 Ce fichier extension_api.json doit être copié dans extern/godot-cpp/ pour remplacer celui existant.

## Génération des bindings C++

```
pushd extern/godot-cpp
scons platform=windows target=template_debug   generate_bindings=yes -j16
scons platform=windows target=template_release generate_bindings=yes -j16
popd
```

## Installation des dépendances, config et build

```
conan install . --output-folder=build --build=missing ^
  -s build_type=RelWithDebInfo ^
  -s:h compiler.cppstd=17 ^
  -s:b compiler.cppstd=17

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build --config RelWithDebInfo
```



