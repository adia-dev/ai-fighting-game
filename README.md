# Wasm Ball Z

## Overview

Wasm Ball Z is a 2D fighting game developed in C++ using SDL2. The project includes character management, sprite animation, basic AI using Q-learning, resource management, and game state handling.

## Dependencies

- C++17 (or higher)
- SDL2
- SDL2_ttf
- SDL2_image
- TinyXML2

If building for the web with Emscripten, install the Emscripten SDK.

## Building

### Using CMake (Recommended)

1. Clone the repository.
2. Create and enter a build directory:
   ```bash
   mkdir build
   cd build
   ```
3. Generate the build system:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   make
   ```

### Manual Compilation (Linux Example)

Compile the source files with a command similar to the following. Adjust include paths and library locations as needed:

```bash
g++ -std=c++17 \
  -I/path/to/SDL2/include \
  -I/path/to/SDL2_ttf/include \
  -I/path/to/SDL2_image/include \
  -I/path/to/tinyxml2 \
  -o WasmBallZ \
  src/main.cpp src/application/application.cpp src/entities/character/character.cpp src/entities/character/ai_character.cpp \
  src/managers/game_manager/game_manager.cpp src/managers/input_manager/input_manager.cpp \
  src/map/map.cpp src/sprite/sprite.cpp src/sprite/animator/animator.cpp \
  src/window/window.cpp src/entities/agent/QLearningAgent.cpp \
  -L/path/to/SDL2/lib -lSDL2 -lSDL2_ttf -lSDL2_image -ltinyxml2
```

Replace `/path/to/...` with the correct paths on your system.

## Running

After building, execute the application from the command line:

```bash
./WasmBallZ
```

## Project Structure

- **src/**  
  Contains the source code.
  - **application/**: Application initialization and main loop.
  - **entities/**: Character classes, AI logic, and other game entities.
  - **managers/**: Resource management, input handling, and game management.
  - **map/**: Map loading and rendering.
  - **sprite/**: Sprite rendering and animation handling.
  - **state/**: Game state definitions and management.
  - **window/**: Window creation and renderer setup.
  - **utils/**: Utility functions and resource path definitions.
- **resources/**  
  Contains textures, fonts, and animation XML files.
