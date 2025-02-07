# ShaderRinth

A Shadertoy-like GLSL Editor written in C++

## Design philosophy

This program should fulfill the purpose of quickly
generating various procedural textures per written
shader code.

For instance, one could write code that procedurally
generates some wood pattern and the program should
take care of its outputs in normals, diffuse, etc.

For that workflow to be applicable, the software
should be relatively easy to use and offer various
presets so that the user can iterate on their ideas
faster.

## Goal posts

The program is currently *viable* for testing
purposes.

---

### VERSION 0.0.1 (Prototype)

* Integrate more powerful code/text editor [done]

* Create framework for shaders/materials/configurable uniforms etc. [done]

* Ability to save/load renders and shader projects [done]

* Node based editor for shader passes / uniforms / textures [done]

* Load textures & configure as uniforms (needs image handling) [done]

* Making renders to picture (e.g. Render Widget) [done -> file option]

* In-app file browser/file outliner (enhanced asset management)

### VERSION 0.1.0 (Miniminal Viable Product)

* Enhanced error highlighting

* Help page?

* Properties widget: Selection system? control parameters

* Outliner widget: show execution flow if multiple graphs are present

* Multiple graphs and render to texture if needed

* Undo history for common actions

* UI improvements [wip]
  * Custom theme [done]
  * Advanced windowing e.g. Splitters, Widgets etc. 

* Finishing touches

* ...


