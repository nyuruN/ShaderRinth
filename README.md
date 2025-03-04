# ShaderRinth

A Shadertoy-like GLSL Editor written in C++

## About the project

ShaderRinth is designed to be a quick a dirty
playground to experiment on your shader ideas!

With ShaderRinth you should be able to generate
textures with your renders, create interactive
demos, and more.

For that workflow to be applicable, the software
should be relatively easy to use and offer various
presets so that the user can iterate on their ideas
faster.

## Known Issues

* Adding a second viewport will only output the latter
and may appear to be stretched

* Many Zep Vim keybinds are not mapped nor integrated
into ShaderRinth and may cause a crash in the worst case
  * :q will crash the program

## Building the project

This project depends on Git submodules and CMake
so make sure these two are installed and available.

### For Windows (MinGW)

Make sure you have MinGW + PkgConfig installed on your system.

```console
git clone --recurse-submodules https://github.com/nyuruN/ShaderRinth.git
cd ShaderRinth && mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=/path/to/mingw-g++.exe -DVCPKG_TARGET_TRIPLET=x64-mingw-static ..
cmake --build .
```

### For Linux

You can use any build system generator of your choice.

```console
git clone --recurse-submodules https://github.com/nyuruN/ShaderRinth.git
cd ShaderRinth && mkdir build && cd build
cmake -G Ninja ..
cmake --build .
```

## Goal posts

The program is currently *viable* for testing
purposes.

---

### VERSION 0.0.1 (Prototype)

* [x] Integrate more powerful code/text editor

* [x] Create framework for shaders/materials/configurable uniforms etc.

* [x] Ability to save/load renders and shader projects

* [x] Node based editor for shader passes / uniforms / textures

* [x] Load textures & configure as uniforms (needs image handling)

* [x] Making renders to picture (e.g. Render Widget)

* [x] Copy and paste in NodeEditorWidget

* [x] Enhanced asset management (Outliner?)

* [x] Runtime Factories for polymorphic nodes and widgets (no if-chains)

* [x] Event system?

* [x] Status bar

* [x] Portable package with CPack

* [ ] Finishing touches (testing, cleanup, cross-compiles)

### VERSION 0.1.0 (Miniminal Viable Product)

* [x] Serialize project data in toml file

* [ ] Enhanced error highlighting

* [ ] Help page?

* [ ] Inspector widget: Selection system? control parameters

* [ ] Multiple graphs and render to texture if needed

* [ ] Undo history for common actions

* [ ] UI improvements

  * [x] Custom theme

  * [ ] Advanced windowing e.g. Splitters, Widgets etc. 

* ***To be continued***


