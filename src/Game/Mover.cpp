#include "Mover.hpp"

Mover::Mover()
    : position(0, 0), velocity(0, 0), acceleration(0, 0), mass(1.0f),
      friction(2.0f) {}

void Mover::update(float deltaTime) {
  // Update velocity based on current acceleration
  velocity += acceleration * deltaTime;

  // Apply friction only to the horizontal (x) velocity:
  float decay = std::exp(-friction * deltaTime);
  velocity.x *= decay;
  // (Leave velocity.y untouched so that gravity is not dampened)

  // Update position with the new velocity
  position += velocity * deltaTime;

  // Reset acceleration after applying forces
  acceleration = Vector2f(0, 0);
}

void Mover::applyForce(const Vector2f &force) {
  acceleration += force * (1.0f / mass);
}
