#include "GuiContext.hpp"
#include "Core/Logger.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_internal.h"
#include <SDL.h>
#include <filesystem>
#include <stdio.h>

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

GuiContext::~GuiContext() {
  if (m_initialized) {
    cleanup();
  }
}

void GuiContext::init(SDL_Window *window, SDL_Renderer *renderer,
                      Config &config) {
  m_window = window;
  m_renderer = renderer;
  m_config = config;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();

  // Set the ini filename before anything else
  if (!config.iniFilename.empty()) {
    static std::string iniPath =
        config.iniFilename; // Keep string in static storage
    io.IniFilename = iniPath.c_str();
  } else {
    io.IniFilename = nullptr; // Disable .ini file
  }

  // Enable keyboard nav
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef __EMSCRIPTEN__
  // Force 1.0 scale for Emscripten
  // io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

  // Disable unsupported features
  // config.flags &= ~ImGuiConfigFlags_ViewportsEnable;
  // config.flags &= ~ImGuiConfigFlags_DockingEnable;
#else
#endif
  io.ConfigFlags |= config.flags;

  // Initialize backends
  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  // Setup style
  setupStyle();

  // Set default layout if no .ini exists
  if (!std::filesystem::exists(config.iniFilename)) {
    Logger::warn("Could not find window layout file at '%s'",
                 config.iniFilename.c_str());
    setupDefaultLayout();
  } else {
    Logger::info("Loading layout file at '%s'", config.iniFilename.c_str());
  }
  m_initialized = true;
}

void GuiContext::cleanup() {
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

void GuiContext::beginFrame() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void GuiContext::endFrame() {
  ImGuiIO &io = ImGui::GetIO();

  // Complete the Dear ImGui frame
  ImGui::Render();

  // Get the draw data
  ImDrawData *draw_data = ImGui::GetDrawData();

  // Render Dear ImGui into our texture
  ImGui_ImplSDLRenderer2_RenderDrawData(draw_data, m_renderer);

  // Present the frame
  SDL_RenderPresent(m_renderer);

#ifndef __EMSCRIPTEN__
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
#endif
}

bool GuiContext::processEvent(const SDL_Event &event) {
  if (!m_initialized)
    return false;

  SDL_Event scaled_event = event;

  // Scale mouse input coordinates based on DPI
  switch (event.type) {
  case SDL_MOUSEMOTION:
    scaled_event.motion.x = static_cast<int>(event.motion.x * m_dpiScale);
    scaled_event.motion.y = static_cast<int>(event.motion.y * m_dpiScale);
    break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    scaled_event.button.x = static_cast<int>(event.button.x * m_dpiScale);
    scaled_event.button.y = static_cast<int>(event.button.y * m_dpiScale);
    break;
  case SDL_MOUSEWHEEL:
    // Mouse wheel doesn't need scaling
    break;
  }

  return ImGui_ImplSDL2_ProcessEvent(&scaled_event);
}

void GuiContext::createGameViewport(int width, int height) {
  // Scale the viewport dimensions based on DPI
  int scaled_width = static_cast<int>(width * m_dpiScale);
  int scaled_height = static_cast<int>(height * m_dpiScale);

  if (m_gameViewportTexture) {
    if (m_gameViewportWidth == scaled_width &&
        m_gameViewportHeight == scaled_height) {
      return; // No need to recreate if dimensions haven't changed
    }
    SDL_DestroyTexture(m_gameViewportTexture);
  }

  m_gameViewportTexture =
      SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_TARGET, scaled_width, scaled_height);

  m_gameViewportWidth = scaled_width;
  m_gameViewportHeight = scaled_height;
}

void GuiContext::updateDpiScale() {
#ifndef __EMSCRIPTEN__
  int window_width, window_height;
  int framebuffer_width, framebuffer_height;
  SDL_GetWindowSize(m_window, &window_width, &window_height);
  SDL_GL_GetDrawableSize(m_window, &framebuffer_width, &framebuffer_height);
  m_dpiScale = static_cast<float>(framebuffer_width) / window_width;
#else
  m_dpiScale = 1.0f;
#endif
}

SDL_Texture *GuiContext::getGameViewportTexture() {
  return m_gameViewportTexture;
}

void GuiContext::beginGameViewportRender() {
  SDL_SetRenderTarget(m_renderer, m_gameViewportTexture);
  SDL_RenderClear(m_renderer);
}

void GuiContext::endGameViewportRender() {
  SDL_SetRenderTarget(m_renderer, nullptr);
}

void GuiContext::setupStyle() {
  ImGuiStyle &style = ImGui::GetStyle();

  // Colors
  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.51f, 0.51f, 0.51f, 0.31f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);

  // Styling
  style.WindowRounding = 4.0f;
  style.FrameRounding = 2.0f;
  style.PopupRounding = 2.0f;
  style.ScrollbarRounding = 2.0f;
  style.FramePadding = ImVec2(4, 3);
  style.ItemSpacing = ImVec2(8, 4);
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
}

void GuiContext::setupDefaultLayout() {
  // Wait for first frame to be rendered
  beginFrame();

  // Set up default docking layout
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::DockBuilderRemoveNode(ImGui::GetID("MyDockSpace"));
  ImGui::DockBuilderAddNode(ImGui::GetID("MyDockSpace"),
                            ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(ImGui::GetID("MyDockSpace"), viewport->Size);

  // Split the docking space
  ImGuiID dockMainId = ImGui::GetID("MyDockSpace");
  ImGuiID dockRightId = ImGui::DockBuilderSplitNode(
      dockMainId, ImGuiDir_Right, 0.25f, nullptr, &dockMainId);
  ImGuiID dockRightDownId = ImGui::DockBuilderSplitNode(
      dockRightId, ImGuiDir_Down, 0.6f, nullptr, &dockRightId);

  // Dock windows
  ImGui::DockBuilderDockWindow("Game View", dockMainId);
  ImGui::DockBuilderDockWindow("AI Control & Debug", dockRightId);
  ImGui::DockBuilderDockWindow("Performance", dockRightDownId);

  ImGui::DockBuilderFinish(dockMainId);

  endFrame();
}
