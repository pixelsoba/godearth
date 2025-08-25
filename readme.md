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
```

# 🔨 Procédure de build

```bash
# Clean
rm -rf build

# Generation de extension_api.json
godot.exe --headless --dump-extension-api build/extension_api.json
```

👉 Ce fichier extension_api.json doit être copié dans extern/godot-cpp/ pour remplacer celui existant.

## Génération des bindings C++

```bash
cd extern/godot-cpp
scons platform=windows target=template_debug   generate_bindings=yes -j16
scons platform=windows target=template_release generate_bindings=yes -j16
cd ../..
```

## Installation des dépendances, config et build

```bash
conan install . --output-folder=build --build=missing -s build_type=RelWithDebInfo -s:h compiler.cppstd=17 -s:b compiler.cppstd=17

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="build/conan_toolchain.cmake"
cmake --build build --config RelWithDebInfo
```

## 🔁 Ajouter de la documentation aux bindings C++

Pour que les classes/méthodes/propriétés exposées par la GDExtension aient une documentation visible dans l'éditeur Godot, il faut patcher le fichier `extension_api.json` avant de regénérer les bindings `godot-cpp`.

Procédure recommandée :

1. Générer l'API actuelle (si nécessaire) :

```powershell
# depuis la racine du projet
godot.exe --headless --dump-extension-api build/extension_api.json
```

2. Fusionner ton patch JSON (dans `doc/extension_api_patches/`) avec `build/extension_api.json` et écrire le résultat dans `extern/godot-cpp/gdextension/extension_api.json`.

Exemple simple (PowerShell) :

```powershell
# remplacer 'ellipsoid_doc.json' par le patch souhaité
$base = Get-Content build/extension_api.json -Raw | ConvertFrom-Json
$patch = Get-Content doc/extension_api_patches/ellipsoid_doc.json -Raw | ConvertFrom-Json
# Fusion basique (remplace ou ajoute la classe Ellipsoid)
$base.classes.Ellipsoid = $patch.classes.Ellipsoid
$base | ConvertTo-Json -Depth 10 | Set-Content extern/godot-cpp/gdextension/extension_api.json
```

3. Regénérer les bindings `godot-cpp` :

```powershell
cd extern/godot-cpp
scons platform=windows target=template_debug generate_bindings=yes -j8
```

4. Rebuild la GDExtension avec CMake.

Note : la fusion JSON peut être plus fine (merging profond) si tu veux préserver d'autres descriptions. Le dossier `doc/extension_api_patches` contient des exemples de patchs (actuellement `ellipsoid_doc.json`).



