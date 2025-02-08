#include "Mover.hpp"

Mover::Mover()
    : position(0, 0), velocity(0, 0), acceleration(0, 0), mass(1.0f),
      friction(2.0f) {}

void Mover::update(float deltaTime) {

  velocity += acceleration * deltaTime;

  float decay = std::exp(-friction * deltaTime);

  velocity = velocity * decay;

  position += velocity * deltaTime;

  acceleration = Vector2f(0, 0);
}

void Mover::applyForce(const Vector2f &force) {
  acceleration += force * (1.0f / mass);
}
