#include "Mover.hpp"
#include <iostream>

Mover::Mover()
    : position(0, 0), velocity(0, 0), acceleration(0, 0), mass(1.0f),
      friction(2.0f) {}

void Mover::update(float deltaTime) {

  velocity += acceleration * deltaTime;

  float decay = std::exp(-friction * deltaTime);

  static int frameCounter = 0;
  frameCounter++;
  if (frameCounter % 60 == 0) {

    std::cout << "Before friction: velocity=(" << velocity.x << ", "
              << velocity.y << ")\n";
  }

  velocity = velocity * decay;

  if (frameCounter % 60 == 0) {

    std::cout << "After friction: velocity=(" << velocity.x << ", "
              << velocity.y << ")\n";
  }

  position += velocity * deltaTime;

  acceleration = Vector2f(0, 0);
}

void Mover::applyForce(const Vector2f &force) {
  acceleration += force * (1.0f / mass);
}
