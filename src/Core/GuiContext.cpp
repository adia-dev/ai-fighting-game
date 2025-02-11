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

  if (!config.iniFilename.empty()) {
    static std::string iniPath = config.iniFilename;
    io.IniFilename = iniPath.c_str();
  } else {
    io.IniFilename = nullptr;
  }

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef __EMSCRIPTEN__

#else
#endif
  io.ConfigFlags |= config.flags;

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  setupStyle();

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

  ImGui::Render();

  ImDrawData *draw_data = ImGui::GetDrawData();

  ImGui_ImplSDLRenderer2_RenderDrawData(draw_data, m_renderer);

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

    break;
  }

  return ImGui_ImplSDL2_ProcessEvent(&scaled_event);
}

void GuiContext::createGameViewport(int width, int height) {

  int scaled_width = static_cast<int>(width * m_dpiScale);
  int scaled_height = static_cast<int>(height * m_dpiScale);

  if (m_gameViewportTexture) {
    if (m_gameViewportWidth == scaled_width &&
        m_gameViewportHeight == scaled_height) {
      return;
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

  ImVec4 *colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_TabSelected] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_TabDimmed] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.34f, 0.31f, 1.00f);
  colors[ImGuiCol_TabDimmedSelectedOverline] =
      ImVec4(0.35f, 0.62f, 0.40f, 1.00f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.41f, 0.95f, 0.48f, 0.70f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
  colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.50f);

  style.FrameBorderSize = 0.0f;
  style.WindowBorderSize = 0.0f;
  style.PopupBorderSize = 0.0f;
  style.TabBorderSize = 0.0f;
  style.ChildBorderSize = 0.0f;
  style.SeparatorTextBorderSize = 0.0f;

  style.FrameRounding = 4.0f;
  style.ChildRounding = 4.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 2.0f;
  style.TabRounding = 4.0f;

  style.GrabMinSize = 15.0f;
  style.ScrollbarSize = 12.0f;

  style.WindowMenuButtonPosition = ImGuiDir_None;

  style.IndentSpacing = 15.0f;

  style.DockingSeparatorSize = 0.0f;

  style.WindowPadding = ImVec2(0.0f, 0.0f);
  style.FramePadding = ImVec2(3.0f, 3.0f);
  style.ItemSpacing = ImVec2(3.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(0.0f, 0.0f);
}

void GuiContext::setupDefaultLayout() {

  beginFrame();

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::DockBuilderRemoveNode(ImGui::GetID("MyDockSpace"));
  ImGui::DockBuilderAddNode(ImGui::GetID("MyDockSpace"),
                            ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(ImGui::GetID("MyDockSpace"), viewport->Size);

  ImGuiID dockMainId = ImGui::GetID("MyDockSpace");
  ImGuiID dockRightId = ImGui::DockBuilderSplitNode(
      dockMainId, ImGuiDir_Right, 0.25f, nullptr, &dockMainId);
  ImGuiID dockRightDownId = ImGui::DockBuilderSplitNode(
      dockRightId, ImGuiDir_Down, 0.6f, nullptr, &dockRightId);

  ImGui::DockBuilderDockWindow("Game View", dockMainId);
  ImGui::DockBuilderDockWindow("AI Control & Debug", dockRightId);
  ImGui::DockBuilderDockWindow("Performance", dockRightDownId);

  ImGui::DockBuilderFinish(dockMainId);

  endFrame();
}
