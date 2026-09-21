# 🍺 Beer Bottle Liquid Simulation (3D)

A real-time 3D beer bottle simulation written in C++ and OpenGL, inspired by the new GTA6 trailer. Renders a textured
beer bottle alongside a genuine 3D liquid cylinder — complete with a tapered neck, a
wavy top surface, Lambert lighting, and rising bubbles that drift inside the liquid.

## Features

- **Textured bottle** loaded from a PNG using `stb_image`
- **True 3D liquid mesh** — a tapered cylinder generated procedurally from a stack × sector grid
- **Animated surface wave** on the top of the liquid using time-based sine offsets
- **Lambert diffuse lighting** on the liquid via per-vertex normals
- **3D bubble particles** rising through the cylinder, with perspective scaling
- **Rotating scene** — the liquid and bubbles spin around the Y axis, the bottle stays static
- **Proper transparency ordering** for correct alpha blending of the liquid over the bubbles
- **Perspective camera** via GLM (`model`, `view`, `projection` matrices)

## Requirements

- **C++ compiler** (g++ or clang++)
- **OpenGL 3.3** capable GPU / driver
- **GLEW** — OpenGL extension loader
- **GLFW** — windowing and input
- **GLM** — matrix math (`perspective`, `rotate`, `translate`, etc.)
- **stb_image** — single-header image loader (system package or vendored)

### Installing dependencies

**Debian / Ubuntu:**
bash
sudo apt install build-essential libglew-dev libglfw3-dev libglm-dev libstb-dev

**Build**
g++ beer_bottle_simulation.cpp -o beer_bottle_simulation -lGL -lglfw -lGLEW'

**Run**

./beer_bottle_simulation
