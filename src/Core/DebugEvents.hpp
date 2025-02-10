#pragma once
#include "Data/Vector2f.hpp"
#include <vector>

struct DamageEvent {
  Vector2f position;
  int damage;
  float timeRemaining;
};

extern std::vector<DamageEvent> g_damageEvents;

void addDamageEvent(const Vector2f &position, int damage);
