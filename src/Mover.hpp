// Mover.hpp
#pragma once
#include "Vector2f.hpp"
#include <cmath>

class Mover {
public:
  Vector2f position;
  Vector2f velocity;
  Vector2f acceleration;
  float mass;
  float friction; // friction coefficient (per second)

  Mover();

  // Apply a force (F = m * a  ==>  a = F / m)
  void applyForce(const Vector2f &force);

  // Update velocity and position given a delta time (in seconds)
  // Here we apply friction (exponential decay of velocity) to smooth out
  // movement.
  void update(float deltaTime);
};
