#include "CombatSystem.hpp"
#include "Core/Logger.hpp"

CombatSystem::CombatSystem()
    : m_roundTime(ROUND_DURATION), m_isRoundActive(true), m_roundCount(0),
      m_playerWins(0), m_enemyWins(0) {}
void CombatSystem::update(float deltaTime, Character &player,
                          Character &enemy) {
  if (!m_isRoundActive)
    return;

  float timeMultiplier =
      m_trainingMode ? 3.0f : 1.0f; // Training runs 3x faster
  m_roundTime -= deltaTime * timeMultiplier;

  // Additional penalties in training mode
  if (m_trainingMode) {
    // Penalize stalling by reducing the time more quickly if no damage is
    // dealt
    if (m_timeSinceLastDamage > 5.0f) {
      m_roundTime -= deltaTime; // Additional time penalty
    }

    // Track damage dealt/received for penalties
    bool damageDealt = (player.health != m_lastPlayerHealth ||
                        enemy.health != m_lastEnemyHealth);

    if (damageDealt) {
      m_timeSinceLastDamage = 0.0f;
    } else {
      m_timeSinceLastDamage += deltaTime * timeMultiplier;
    }
  }

  // End round conditions
  if (m_roundTime <= 0 || player.health <= 0 || enemy.health <= 0) {
    endRound(player, enemy);
  }

  // Store health values for next frame
  m_lastPlayerHealth = player.health;
  m_lastEnemyHealth = enemy.health;
}

void CombatSystem::startNewRound(Character &player, Character &enemy) {
  m_roundTime = ROUND_DURATION;
  m_isRoundActive = true;
  m_roundCount++;

  resetCharacter(player, Vector2f(200, 100));
  resetCharacter(enemy, Vector2f(600, 100));
}

void CombatSystem::render(SDL_Renderer *renderer) {
  renderTimer(renderer);
  renderRoundInfo(renderer);
}

void CombatSystem::endRound(Character &player, Character &enemy) {
  m_isRoundActive = false;

  if (player.health <= 0) {
    Logger::info("Round " + std::to_string(m_roundCount) +
                 " ended - Enemy wins!");
    m_enemyWins++;
  } else if (enemy.health <= 0) {
    Logger::info("Round " + std::to_string(m_roundCount) +
                 " ended - Player wins!");
    m_playerWins++;
  } else {
    if (player.health > enemy.health) {
      Logger::info("Round " + std::to_string(m_roundCount) +
                   " ended - Player wins on health!");
      m_playerWins++;
    } else if (enemy.health > player.health) {
      Logger::info("Round " + std::to_string(m_roundCount) +
                   " ended - Enemy wins on health!");
      m_enemyWins++;
    } else {
      Logger::info("Round " + std::to_string(m_roundCount) + " ended - Draw!");
    }
  }

  Logger::info("Player Wins: %d/%d (%f)", m_playerWins, m_roundCount,
               static_cast<float>(m_playerWins) / m_roundCount);
  Logger::info("Enemy Wins: %d/%d (%f)", m_enemyWins, m_roundCount,
               static_cast<float>(m_enemyWins) / m_roundCount);
}

void CombatSystem::resetCharacter(Character &character,
                                  const Vector2f &position) {
  character.mover.position = position;
  character.mover.velocity = Vector2f(0, 0);
  character.health = character.maxHealth;
  character.onGround = false;
  character.animator->play("Idle");
}

void CombatSystem::renderTimer(SDL_Renderer *renderer) {

  SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
  SDL_Rect bgRect = {350, 10, 100, 30};
  SDL_RenderFillRect(renderer, &bgRect);

  float timeRatio = m_roundTime / ROUND_DURATION;
  SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(255 * (1.0f - timeRatio)),
                         static_cast<Uint8>(255 * timeRatio), 0, 255);
  SDL_Rect timerRect = {bgRect.x + 2, bgRect.y + 2,
                        static_cast<int>((bgRect.w - 4) * timeRatio),
                        bgRect.h - 4};
  SDL_RenderFillRect(renderer, &timerRect);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDrawRect(renderer, &bgRect);
}

void CombatSystem::renderRoundInfo(SDL_Renderer *renderer) {

  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  SDL_Rect roundRect = {10, 10, 80, 30};
  SDL_RenderDrawRect(renderer, &roundRect);
}
void CombatSystem::setTrainingMode(bool enabled) {
  m_trainingMode = enabled;
  m_roundTime = enabled ? TRAINING_ROUND_DURATION : NORMAL_ROUND_DURATION;
}

