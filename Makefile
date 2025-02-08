# Compiler settings
COMPILER := g++
ROOT_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
RESOURCE_DIR := $(ROOT_DIR)assets

# SDL flags for both native and wasm builds
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 sdl2_image sdl2_ttf)
SDL_LIBS := $(shell pkg-config --libs sdl2 sdl2_image sdl2_ttf)
LDFLAGS := $(SDL_LIBS)

# Optimization and memory options for emcc
EMCC_OPTIMIZATION_LEVEL := 3
EMCC_TOTAL_MEMORY := 256MB
EMCC_INITIAL_MEMORY := 256MB
EMCC_ALLOW_MEMORY_GROWTH := 1

# Compilation flags
CFLAGS := --std=c++17 -g -Wall $(SDL_CFLAGS) -I$(ROOT_DIR)src -DRESOURCE_DIR=\"$(RESOURCE_DIR)\"

# Emscripten-specific flags for WebAssembly builds
EMCCFLAGS := -sUSE_SDL=2 \
             -sUSE_SDL_IMAGE=2 \
             -sUSE_SDL_TTF=2 \
             --emrun \
             -lembind \
             -O$(EMCC_OPTIMIZATION_LEVEL) \
             -DRESOURCE_DIR=\"/assets\" \
             --preload-file $(RESOURCE_DIR)@/assets \
             -sSDL2_IMAGE_FORMATS='["png", "jpg"]' \
             -sINITIAL_MEMORY=$(EMCC_INITIAL_MEMORY) \
             -sTOTAL_MEMORY=$(EMCC_TOTAL_MEMORY) \
             -sALLOW_MEMORY_GROWTH=$(EMCC_ALLOW_MEMORY_GROWTH)

# Directories
SRC_DIR := $(ROOT_DIR)src
OBJ_DIR := $(ROOT_DIR)obj
BIN_DIR := $(ROOT_DIR)bin
DIST_DIR := $(ROOT_DIR)dist

# Output binary names
BIN := app
DIST := app
DIST_EXTENSION := html

# Source and object files (ignore files starting with .null-ls)
SRC_FILES := $(shell find $(SRC_DIR) -type f -name '*.cpp' -not -name '.null-ls*')
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Code formatting style
CLANG_FORMAT_STYLE := LLVM

# Phony targets
.PHONY: all clean bear format run wasm_run

# Default target to build everything
all: format app wasm

# Compile .cpp files to .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(COMPILER) $(CFLAGS) -c $< -o $@

# Native build target
app: $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(COMPILER) $(CFLAGS) -o $(BIN_DIR)/$(BIN) $(OBJ_FILES) $(LDFLAGS)

# Run native application
run: app
	$(BIN_DIR)/$(BIN)

# WebAssembly build with preloaded resources
wasm: COMPILER := emcc
wasm: $(SRC_FILES)
	@mkdir -p $(DIST_DIR)
	$(COMPILER) $(EMCCFLAGS) -I$(ROOT_DIR)src $(SRC_FILES) -o $(DIST_DIR)/$(DIST).$(DIST_EXTENSION)

# Run WebAssembly application
wasm_run: wasm
	emrun $(DIST_DIR)/$(DIST).html

# Format code with clang-format
format:
	clang-format -i -style=$(CLANG_FORMAT_STYLE) src/**/*.cpp src/**/*.hpp

# Generate compilation database for tools like bear
bear: clean
	bear -- make all

# Clean build directories
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DIST_DIR)
