#include "DebugEvents.hpp"

std::vector<DamageEvent> g_damageEvents;

void addDamageEvent(const Vector2f &position, int damage) {
  DamageEvent ev;
  ev.position = position;
  ev.damage = damage;
  ev.timeRemaining = 1.0f;
  g_damageEvents.push_back(ev);
}
