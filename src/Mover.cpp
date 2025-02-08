#include "Mover.hpp"
#include <iostream>

Mover::Mover()
    : position(0, 0), velocity(0, 0), acceleration(0, 0), mass(1.0f),
      friction(2.0f) // adjust as needed; higher values mean more friction
{}

void Mover::applyForce(const Vector2f &force) {
  acceleration += force * (1.0f / mass);
}

void Mover::update(float deltaTime) {
  // Apply acceleration to velocity.
  velocity += acceleration * deltaTime;

  // --- Apply friction ---
  // We use an exponential decay: v' = v * exp(-friction * dt)
  float decay = std::exp(-friction * deltaTime);

  // Log before applying friction every 60 frames for debugging.
  static int frameCounter = 0;
  frameCounter++;
  if (frameCounter % 60 == 0) {
    // Assuming you have a Logger (you can also use std::cout)
    // core::Logger::debug("Before friction: velocity=(%.2f, %.2f)",
    // velocity.x, velocity.y);
    std::cout << "Before friction: velocity=(" << velocity.x << ", "
              << velocity.y << ")\n";
  }

  velocity = velocity * decay;

  if (frameCounter % 60 == 0) {
    // core::Logger::debug("After friction: velocity=(%.2f, %.2f)",
    // velocity.x, velocity.y);
    std::cout << "After friction: velocity=(" << velocity.x << ", "
              << velocity.y << ")\n";
  }

  // Update position.
  position += velocity * deltaTime;

  // Reset acceleration after update.
  acceleration = Vector2f(0, 0);
}
