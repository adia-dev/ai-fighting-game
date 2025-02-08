// Mover.hpp
#pragma once
#include "Vector2f.hpp"

class Mover {
public:
  Vector2f position;
  Vector2f velocity;
  Vector2f acceleration;
  float mass;

  Mover() : position(0, 0), velocity(0, 0), acceleration(0, 0), mass(1.0f) {}

  // Apply a force (F = m * a  ==>  a = F / m)
  void applyForce(const Vector2f &force) {
    acceleration += force * (1.0f / mass);
  }

  // Update velocity and position given a delta time (in seconds)
  void update(float deltaTime) {
    velocity += acceleration * deltaTime;
    position += velocity * deltaTime;
    // Reset acceleration each frame
    acceleration = Vector2f(0, 0);
  }
};
