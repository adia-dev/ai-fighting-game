################################################################################
# Platform Detection and Safety Settings
################################################################################

# Determine operating system and set platform-specific variables safely
UNAME_S := $(shell uname -s)

# Determine optimal number of parallel jobs (num_cores + 1 is often optimal)
ifeq ($(UNAME_S),Darwin)
    NUM_CORES := $(shell sysctl -n hw.ncpu)
    PARALLEL_JOBS := $(shell echo $(( $(NUM_CORES) + 1 )))
    OPEN_CMD := open
    # macOS specific - use brew paths
    BREW_PREFIX := $(shell brew --prefix)
else
    NUM_CORES := $(shell nproc 2>/dev/null || echo 1)
    OPEN_CMD := xdg-open
endif

# Sanitizer configuration - can be disabled with make ENABLE_ASAN=0
ENABLE_ASAN ?= 0

# Command line arguments for run target
ARGS ?=

################################################################################
# Project Structure - All paths relative to project root
################################################################################

# Ensure ROOT_DIR is absolute and ends with a slash
ROOT_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
ifeq ($(ROOT_DIR),)
    $(error Could not determine project root directory)
endif
ROOT_DIR := $(ROOT_DIR)/

# ImGui directory setup
IMGUI_DIR := $(ROOT_DIR)vendor/imgui

# Project metadata
PROJECT_NAME := fighting-game
VERSION := 1.0.0
DESCRIPTION := "Fighting Game Project"

# Directory structure - all relative to ROOT_DIR
BUILD_DIR := $(ROOT_DIR)build
BUILD_TYPE ?= Debug
BUILD_DIR_FULL := $(BUILD_DIR)/$(BUILD_TYPE)
OBJ_DIR := $(BUILD_DIR_FULL)/obj
EXE_DIR := $(BUILD_DIR_FULL)/bin
LIB_DIR := $(BUILD_DIR_FULL)/lib
SOURCES_DIR := $(ROOT_DIR)src
INCLUDE_DIR := $(ROOT_DIR)include
TEST_DIR := $(ROOT_DIR)tests
RESOURCE_DIR := $(ROOT_DIR)assets
LOG_DIR := $(ROOT_DIR)logs
DOC_DIR := $(ROOT_DIR)docs

# Final executable name
EXE := $(PROJECT_NAME)
ifeq ($(OS),Windows_NT)
    EXE := $(EXE).exe
endif

# Timestamp for build logging
TIMESTAMP := $(shell date '+%Y%m%d_%H%M%S')
BUILD_LOG := $(LOG_DIR)/build_$(TIMESTAMP).log

################################################################################
# Compiler Configuration
################################################################################

# Use ccache if available with optimized settings
CCACHE := $(shell command -v ccache 2> /dev/null)
ifdef CCACHE
    CXX := ccache g++
    export CCACHE_DIR := $(BUILD_DIR)/.ccache
    export CCACHE_COMPRESS := 1
    export CCACHE_COMPRESSLEVEL := 6
    export CCACHE_MAXSIZE := 10G
    export CCACHE_SLOPPINESS := time_macros,include_file_mtime,file_macro
    export CCACHE_LIMIT_MULTIPLE := 0.95
    export CCACHE_LOGFILE := $(LOG_DIR)/ccache.log
    export CCACHE_BASEDIR := $(ROOT_DIR)
else
    CXX := g++
endif

# Find all source files safely
SOURCES := $(shell find "$(SOURCES_DIR)" -type f -name '*.cpp' 2>/dev/null)

# ImGui sources with proper paths
IMGUI_SOURCES := $(IMGUI_DIR)/imgui.cpp \
                 $(IMGUI_DIR)/imgui_demo.cpp \
                 $(IMGUI_DIR)/imgui_draw.cpp \
                 $(IMGUI_DIR)/imgui_tables.cpp \
                 $(IMGUI_DIR)/imgui_widgets.cpp \
                 $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
                 $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer2.cpp

# Combine sources
ALL_SOURCES := $(SOURCES) $(IMGUI_SOURCES)

# Object and dependency files with safe paths
OBJS := $(ALL_SOURCES:$(ROOT_DIR)%=$(OBJ_DIR)/%)
OBJS := $(OBJS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

# Base compiler flags
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror=return-type \
            -Wno-unused-parameter -pthread \
            -I"$(INCLUDE_DIR)" \
            -I"$(IMGUI_DIR)" \
            -I"$(IMGUI_DIR)/backends" \
            -I"$(SOURCES_DIR)"

# Architecture-specific flags for Apple Silicon
ifeq ($(UNAME_S),Darwin)
    ARCH := $(shell uname -m)
    ifeq ($(ARCH),arm64)
        CXXFLAGS += -arch arm64
        LDFLAGS += -arch arm64
    endif

    CXXFLAGS += -I$(BREW_PREFIX)/include
    FRAMEWORK_PATHS := -F/Library/Frameworks -F$(BREW_PREFIX)/Frameworks
    LIBRARY_PATHS := -L$(BREW_PREFIX)/lib

    # Check if required packages are available
    ifneq ($(shell pkg-config --exists sdl2 sdl2_ttf sdl2_image || echo 'no'),)
        $(error Missing required packages. Please run: brew install sdl2 sdl2_ttf sdl2_image)
    endif

    CXXFLAGS += $(shell pkg-config --cflags sdl2 sdl2_ttf sdl2_image)
    LIBS := $(FRAMEWORK_PATHS) \
            -framework OpenGL \
            -framework Cocoa \
            -framework IOKit \
            -framework CoreVideo \
            $(LIBRARY_PATHS) \
            $(shell pkg-config --libs sdl2 sdl2_ttf sdl2_image)
endif

# Build type specific flags with sanitizer support
ifeq ($(BUILD_TYPE),Debug)
    CXXFLAGS += -g -O0 -DDEBUG
    ifeq ($(ENABLE_ASAN),1)
        SANITIZE_FLAGS := -fsanitize=address -fno-omit-frame-pointer
        CXXFLAGS += $(SANITIZE_FLAGS)
        LDFLAGS += $(SANITIZE_FLAGS)
        $(info AddressSanitizer enabled)
    endif
else
    CXXFLAGS += -O3 -DNDEBUG -flto
    LDFLAGS += -flto
endif

# Resource definitions
CXXFLAGS += -DRESOURCE_DIR=\"$(RESOURCE_DIR)\" \
            -DLOG_DIR=\"$(LOG_DIR)\" \
            -DBUILD_TIMESTAMP=\"$(TIMESTAMP)\"

# Add dependency generation
CXXFLAGS += -MMD -MP

################################################################################
# Output Formatting
################################################################################

# Use printf safely
PRINTF := /usr/bin/printf

# Terminal colors if supported
ifneq ($(TERM),)
    BLUE := $(shell tput setaf 4)
    GREEN := $(shell tput setaf 2)
    YELLOW := $(shell tput setaf 3)
    RED := $(shell tput setaf 1)
    BOLD := $(shell tput bold)
    RESET := $(shell tput sgr0)
endif

################################################################################
# Build Targets
################################################################################

.PHONY: all clean clean-all install uninstall test docs coverage format lint analyze help setup-imgui

# Default target
all: check-env log print-info $(EXE_DIR)/$(EXE)
	@$(PRINTF) "$(GREEN)Build complete for $(UNAME_S) ($(NUM_CORES) cores)$(RESET)\n"
	@$(PRINTF) "$(BLUE)Build log saved to: $(BUILD_LOG)$(RESET)\n"

# ImGui setup target
setup-imgui:
	@mkdir -p $(ROOT_DIR)vendor
	@if [ ! -d "$(IMGUI_DIR)" ]; then \
		git clone https://github.com/ocornut/imgui.git $(IMGUI_DIR); \
		$(PRINTF) "$(GREEN)ImGui cloned successfully to $(IMGUI_DIR)$(RESET)\n"; \
	else \
		$(PRINTF) "$(YELLOW)ImGui directory already exists$(RESET)\n"; \
	fi

# Environment check
check-env:
	@$(PRINTF) "$(BLUE)Checking build environment...$(RESET)\n"
	@if [ ! -d "$(SOURCES_DIR)" ]; then \
		$(PRINTF) "$(RED)Error: Source directory not found$(RESET)\n"; \
		exit 1; \
	fi

# Build information
print-info:
	@$(PRINTF) "$(BLUE)Build Configuration:$(RESET)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  Project: $(PROJECT_NAME) v$(VERSION)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  OS: $(UNAME_S)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  Compiler: $(CXX)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  Build Type: $(BUILD_TYPE)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  Number of CPU cores: $(NUM_CORES)\n" | tee -a "$(BUILD_LOG)"
	@$(PRINTF) "  ASan Enabled: $(ENABLE_ASAN)\n" | tee -a "$(BUILD_LOG)"

# Compilation rule with safety checks
$(OBJ_DIR)/%.o: $(ROOT_DIR)%.cpp
	@mkdir -p $(dir $@)
	@$(PRINTF) "$(YELLOW)Compiling: $<$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -c -o "$@" "$<" 2>&1 | tee -a "$(BUILD_LOG)"; \
	exit_code=$${PIPESTATUS[0]}; \
	if [ $$exit_code -ne 0 ]; then \
		$(PRINTF) "$(RED)Error compiling $< - See $(BUILD_LOG) for details$(RESET)\n"; \
		exit $$exit_code; \
	fi

# Linking rule with safety checks
$(EXE_DIR)/$(EXE): $(OBJS)
	@mkdir -p "$(EXE_DIR)"
	@$(PRINTF) "$(BLUE)Linking: $@$(RESET)\n"
	@$(CXX) -o "$@" $(OBJS) $(LIBS) $(LDFLAGS) 2>&1 | tee -a "$(BUILD_LOG)"; \
	exit_code=$${PIPESTATUS[0]}; \
	if [ $$exit_code -ne 0 ]; then \
		$(PRINTF) "$(RED)Error linking $@ - See $(BUILD_LOG) for details$(RESET)\n"; \
		exit $$exit_code; \
	fi

# Directory creation
log:
	@mkdir -p "$(LOG_DIR)"
	@mkdir -p "$(BUILD_DIR)/.ccache"

################################################################################
# Development Targets
################################################################################

# Run target that builds and executes the project
run: $(EXE_DIR)/$(EXE)
	@$(PRINTF) "$(BLUE)Running $(PROJECT_NAME)...$(RESET)\n"
	@DYLD_DISABLE_CRASH_REPORTER=1 \
	ASAN_OPTIONS="detect_stack_use_after_return=1:strict_init_order=1:strict_memcmp=1" \
	ASAN_SYMBOLIZER_PATH="$(shell which llvm-symbolizer)" \
	"$(EXE_DIR)/$(EXE)" $(ARGS)

################################################################################
# Cleaning Targets
################################################################################

# Safe clean target that only removes build artifacts
clean:
	@$(PRINTF) "$(BLUE)Cleaning build artifacts...$(RESET)\n"
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf "$(BUILD_DIR)"; \
	fi
	@if [ -d "$(EXE_DIR)" ]; then \
		rm -rf "$(EXE_DIR)"; \
	fi
	@find . -type f \( -name "*.o" -o -name "*.d" \) -delete
	@$(PRINTF) "$(GREEN)Clean complete$(RESET)\n"

# Deep clean that removes all generated files
clean-all: clean
	@$(PRINTF) "$(BLUE)Performing deep clean...$(RESET)\n"
	@if [ -d "$(LOG_DIR)" ]; then \
		rm -rf "$(LOG_DIR)"; \
	fi
	@if [ -d "$(DOC_DIR)" ]; then \
		rm -rf "$(DOC_DIR)"; \
	fi
	@find . -type f \( -name "*~" -o -name "*.tmp" -o -name ".DS_Store" -o -name "*.bak" \) -delete
	@$(PRINTF) "$(GREEN)Deep clean complete$(RESET)\n"

################################################################################
# Help Target
################################################################################

help:
	@$(PRINTF) "$(BOLD)$(PROJECT_NAME) Build System Help$(RESET)\n\n"
	@$(PRINTF) "$(BLUE)Setup:$(RESET)\n"
	@$(PRINTF) "  make setup-imgui  - Setup ImGui library\n"
	@$(PRINTF) "$(BLUE)Build Targets:$(RESET)\n"
	@$(PRINTF) "  make              - Build the project\n"
	@$(PRINTF) "  make run          - Build and run the project\n"
	@$(PRINTF) "\n$(BLUE)Cleaning Targets:$(RESET)\n"
	@$(PRINTF) "  make clean        - Remove build artifacts\n"
	@$(PRINTF) "  make clean-all    - Remove all generated files\n"
