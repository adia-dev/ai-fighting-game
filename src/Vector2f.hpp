// Vector2f.hpp
#pragma once
#include <cmath>

struct Vector2f {
  float x, y;
  Vector2f() : x(0.f), y(0.f) {}
  Vector2f(float x, float y) : x(x), y(y) {}

  Vector2f operator+(const Vector2f &other) const {
    return Vector2f(x + other.x, y + other.y);
  }
  Vector2f operator-(const Vector2f &other) const {
    return Vector2f(x - other.x, y - other.y);
  }
  Vector2f operator*(float scalar) const {
    return Vector2f(x * scalar, y * scalar);
  }
  Vector2f &operator+=(const Vector2f &other) {
    x += other.x;
    y += other.y;
    return *this;
  }
  Vector2f &operator-=(const Vector2f &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
  float length() const { return std::sqrt(x * x + y * y); }
  Vector2f normalized() const {
    float len = length();
    return (len > 0.f) ? Vector2f(x / len, y / len) : Vector2f(0.f, 0.f);
  }
};
