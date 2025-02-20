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

* [ ] Status bar

* [ ] Portable package with CPack

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


