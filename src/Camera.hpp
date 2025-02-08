#pragma once
#include "Vector2f.hpp"

struct Camera {
  Vector2f position;
  float scale;
  Vector2f targetPosition;
  float targetScale;
};
