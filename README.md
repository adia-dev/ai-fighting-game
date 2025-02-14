# Fighting Game Project

<p align="center">
  <img src="https://github.com/user-attachments/assets/bd26ae13-45ae-463e-a161-8e0a934eba90" alt="Game Screenshot">
</p>

A 2D fighting game with neural network-powered AI opponents, built with SDL2 and ImGui.

## Features

- Real-time 2D combat system with combos, blocking, and special moves
- Neural network AI opponents that learn and adapt
- Debug visualization tools for AI decision making
- Customizable character animations and hitboxes
- Training mode for AI agents
- Cross-platform support (Windows, macOS, Linux, Web)

<p align="center">
  <video src="https://github.com/user-attachments/assets/d1e813ca-8c45-499a-a680-d0eafebd7052" alt="AI Debug Visualization">
</p>


## Prerequisites

### Desktop Build
- CMake 3.15+
- C++17 compatible compiler
- SDL2 development libraries
- SDL2_image development libraries
- SDL2_ttf development libraries

### Web Build
- Emscripten SDK
- Python 3.x (for local development server)

## Building

### Desktop Build

```bash
# Clone the repository
git clone https://github.com/yourusername/fighting-game.git
cd fighting-game

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run the game
./fighting-game
```

### Web Build

```bash
# Make sure you have sourced the Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Build using Emscripten makefile
make -f Makefile.emscripten

# Start local development server
make -f Makefile.emscripten serve
```

Then open `http://localhost:8000` in your web browser.

## Controls

- **Left/Right Arrow**: Move
- **Space**: Jump
- **A**: Attack
- **B**: Block
- **D**: Dash
- **Tab**: Toggle Debug UI
- **Escape**: Close Debug UI

## Development

### Adding New Features

1. Character Animations:
   - Add animation frames to `assets/animations/`
   - Define hitboxes in animation JSON files
   - Register animations in `PiksyAnimationLoader`

2. AI Behaviors:
   - Extend the state space in `State.hpp`
   - Add new action types in `ActionType`
   - Modify reward calculations in `RLAgent`

## Configuration

The game can be configured through:
- Command line arguments
- Config files in `assets/config/`
- In-game debug UI

Key configuration options include:
- Physics parameters
- AI learning rates
- Combat mechanics
- Graphics settings


## Acknowledgments

- SDL2 development team
- Dear ImGui library
- [Piksy](https://github.com/yourusername/piksy) for animation tooling
