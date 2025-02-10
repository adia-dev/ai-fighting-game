#pragma once

#include <Data/Vector2f.hpp>

struct Camera {
  Vector2f position;
  float scale;
  Vector2f targetPosition;
  float targetScale;

  // New properties for character-focused zoom
  float minZoom = 0.8f;
  float maxZoom = 1.5f;
  float defaultZoom = 1.0f;
  float zoomSpeed = 2.0f;
  float moveSpeed = 5.0f;

  // Boundaries for camera movement
  float boundaryLeft = 0.0f;
  float boundaryRight = 1440.0f;
  float boundaryTop = 0.0f;
  float boundaryBottom = 900.0f;

  // Focus rectangle (area where both characters should stay)
  float focusMarginX = 200.0f;
  float focusMarginY = 150.0f;
};
