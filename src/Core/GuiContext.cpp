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
  // ImVec4* colors = style.Colors;

  // // Base Colors
  // ImVec4 bgColor = ImVec4(0.10f, 0.105f, 0.11f, 1.00f);
  // ImVec4 lightBgColor = ImVec4(0.15f, 0.16f, 0.17f, 1.00f);
  // ImVec4 panelColor = ImVec4(0.17f, 0.18f, 0.19f, 1.00f);
  // ImVec4 panelHoverColor = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
  // ImVec4 panelActiveColor = ImVec4(0.23f, 0.26f, 0.29f, 1.00f);
  // ImVec4 textColor = ImVec4(0.86f, 0.87f, 0.88f, 1.00f);
  // ImVec4 textDisabledColor = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  // ImVec4 borderColor = ImVec4(0.14f, 0.16f, 0.18f, 1.00f);
  //
  // // Text
  // colors[ImGuiCol_Text] = textColor;
  // colors[ImGuiCol_TextDisabled] = textDisabledColor;
  //
  // // Windows
  // colors[ImGuiCol_WindowBg] = bgColor;
  // colors[ImGuiCol_ChildBg] = bgColor;
  // colors[ImGuiCol_PopupBg] = bgColor;
  // colors[ImGuiCol_Border] = borderColor;
  // colors[ImGuiCol_BorderShadow] = borderColor;
  //
  // // Headers
  // colors[ImGuiCol_Header] = panelColor;
  // colors[ImGuiCol_HeaderHovered] = panelHoverColor;
  // colors[ImGuiCol_HeaderActive] = panelActiveColor;
  //
  // // Buttons
  // colors[ImGuiCol_Button] = panelColor;
  // colors[ImGuiCol_ButtonHovered] = panelHoverColor;
  // colors[ImGuiCol_ButtonActive] = panelActiveColor;
  //
  // // Frame BG
  // colors[ImGuiCol_FrameBg] = lightBgColor;
  // colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
  // colors[ImGuiCol_FrameBgActive] = panelActiveColor;
  //
  // // Tabs
  // colors[ImGuiCol_Tab] = panelColor;
  // colors[ImGuiCol_TabHovered] = panelHoverColor;
  // colors[ImGuiCol_TabActive] = panelActiveColor;
  // colors[ImGuiCol_TabUnfocused] = panelColor;
  // colors[ImGuiCol_TabUnfocusedActive] = panelHoverColor;
  //
  // // Title
  // colors[ImGuiCol_TitleBg] = bgColor;
  // colors[ImGuiCol_TitleBgActive] = bgColor;
  // colors[ImGuiCol_TitleBgCollapsed] = bgColor;
  //
  // // Scrollbar
  // colors[ImGuiCol_ScrollbarBg] = bgColor;
  // colors[ImGuiCol_ScrollbarGrab] = panelColor;
  // colors[ImGuiCol_ScrollbarGrabHovered] = panelHoverColor;
  // colors[ImGuiCol_ScrollbarGrabActive] = panelActiveColor;
  //
  // // Checkmark
  // colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  //
  // // Slider
  // colors[ImGuiCol_SliderGrab] = panelHoverColor;
  // colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
  //
  // // Resize Grip
  // colors[ImGuiCol_ResizeGrip] = panelColor;
  // colors[ImGuiCol_ResizeGripHovered] = panelHoverColor;
  // colors[ImGuiCol_ResizeGripActive] = panelActiveColor;
  //
  // // Separator
  // colors[ImGuiCol_Separator] = borderColor;
  // colors[ImGuiCol_SeparatorHovered] = panelHoverColor;
  // colors[ImGuiCol_SeparatorActive] = panelActiveColor;
  //
  // // Plot
  // colors[ImGuiCol_PlotLines] = textColor;
  // colors[ImGuiCol_PlotLinesHovered] = panelActiveColor;
  // colors[ImGuiCol_PlotHistogram] = textColor;
  // colors[ImGuiCol_PlotHistogramHovered] = panelActiveColor;
  //
  // // Text Selected BG
  // colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
  //
  // // Modal Window Dim Bg
  // colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.105f, 0.11f, 0.5f);
  //
  // // Tables
  // colors[ImGuiCol_TableHeaderBg] = panelColor;
  // colors[ImGuiCol_TableBorderStrong] = borderColor;
  // colors[ImGuiCol_TableBorderLight] = borderColor;
  // colors[ImGuiCol_TableRowBg] = bgColor;
  // colors[ImGuiCol_TableRowBgAlt] = lightBgColor;

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

  // Styles
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

  // Reduced Padding and Spacing
  style.WindowPadding = ImVec2(0.0f, 0.0f);
  style.FramePadding = ImVec2(3.0f, 3.0f);
  style.ItemSpacing = ImVec2(3.0f, 4.0f);
  style.ItemInnerSpacing = ImVec2(0.0f, 0.0f);

  // Font Scaling
  // ImGuiIO &io = ImGui::GetIO();
  // io.FontGlobalScale = 0.95f;
  //
  // io.Fonts->AddFontDefault();
  // float baseFontSize = 18.0f;
  // float iconFontSize = baseFontSize * 2.0f / 3.0f;
  //
  // // merge in icons from Font Awesome
  // ImFontConfig icons_config;
  // icons_config.MergeMode = true;
  // icons_config.PixelSnapH = true;
  // icons_config.GlyphMinAdvanceX = iconFontSize;

  // {
  //   static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  //   io.Fonts->AddFontFromFileTTF(
  //       (std::string(RESOURCE_DIR) + "/fonts/" + FONT_ICON_FILE_NAME_FA)
  //           .c_str(),
  //       iconFontSize, &icons_config, icons_ranges);
  // }
  // {
  //   static const ImWchar icons_ranges[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};
  //   io.Fonts->AddFontFromFileTTF(
  //       (std::string(RESOURCE_DIR) + "/fonts/" + FONT_ICON_FILE_NAME_MD)
  //           .c_str(),
  //       iconFontSize, &icons_config, icons_ranges);
  // }
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
