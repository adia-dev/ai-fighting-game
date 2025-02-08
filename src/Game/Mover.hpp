#pragma once

#include "Data/Vector2f.hpp"

class Mover {
public:
  Vector2f position;
  Vector2f velocity;
  Vector2f acceleration;
  float mass;
  float friction;

  Mover();

  void applyForce(const Vector2f &force);

  void update(float deltaTime);
};
