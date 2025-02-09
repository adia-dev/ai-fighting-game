#pragma once
#include "Game/Character.hpp"
#include <SDL.h>

class CombatSystem {
public:
  static constexpr float ROUND_DURATION = 60.0f;
  static constexpr float NORMAL_ROUND_DURATION = 60.0f;
  static constexpr float TRAINING_ROUND_DURATION = 20.0f;
  //
  CombatSystem();

  void update(float deltaTime, Character &player, Character &enemy);

  void startNewRound(Character &player, Character &enemy);
  void setTrainingMode(bool enabled);

  void render(SDL_Renderer *renderer);

  bool isRoundActive() const { return m_isRoundActive; }
  float getRoundTime() const { return m_roundTime; }
  int getRoundCount() const { return m_roundCount; }

private:
  void endRound(Character &player, Character &enemy);

  void resetCharacter(Character &character, const Vector2f &position);

  void renderTimer(SDL_Renderer *renderer);

  void renderRoundInfo(SDL_Renderer *renderer);

  float m_roundTime;
  bool m_isRoundActive;
  int m_roundCount;
  int m_playerWins;
  int m_enemyWins;
  bool m_trainingMode = false;
  float m_timeSinceLastDamage = 0.0f;
  int m_lastPlayerHealth = 100;
  int m_lastEnemyHealth = 100;
};
