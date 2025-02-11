#include "CombatSystem.hpp"
#include "Core/Logger.hpp"

CombatSystem::CombatSystem(Config &config, RLAgent *playerAgent,
                           RLAgent *enemyAgent)
    : m_config(config), m_roundTime(ROUND_DURATION), m_isRoundActive(true),
      m_roundCount(0), m_playerWins(0), m_enemyWins(0),
      m_enemyAgent(enemyAgent), m_playerAgent(playerAgent) {}
void CombatSystem::update(float deltaTime, Character &player,
                          Character &enemy) {
  if (!m_isRoundActive)
    return;

  float timeMultiplier = m_trainingMode ? 3.0f : 1.0f;
  m_roundTime -= deltaTime * timeMultiplier;

  if (m_roundTime <= 0 || player.health <= 0 || enemy.health <= 0) {
    endRound(player, enemy);
  }

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
  bool playerWon = false;

  if (player.health <= 0) {
    Logger::info("Round " + std::to_string(m_roundCount) +
                 " ended - Enemy wins!");
    m_enemyWins++;
    playerWon = false;
  } else if (enemy.health <= 0) {
    Logger::info("Round " + std::to_string(m_roundCount) +
                 " ended - Player wins!");
    m_playerWins++;
    playerWon = true;
  } else {

    float playerHealthPercent =
        static_cast<float>(player.health) / player.maxHealth;
    float enemyHealthPercent =
        static_cast<float>(enemy.health) / enemy.maxHealth;

    if (playerHealthPercent > enemyHealthPercent) {
      Logger::info("Round " + std::to_string(m_roundCount) +
                   " ended - Player wins on health!");
      m_playerWins++;
      playerWon = true;
    } else if (enemyHealthPercent > playerHealthPercent) {
      Logger::info("Round " + std::to_string(m_roundCount) +
                   " ended - Enemy wins on health!");
      m_enemyWins++;
      playerWon = false;
    } else {
      Logger::info("Round " + std::to_string(m_roundCount) + " ended - Draw!");

      playerWon = false;
    }
  }

  if (m_playerAgent) {
    m_playerAgent->reportWin(playerWon);
    m_playerAgent->incrementEpisodeCount();
  }
  if (m_enemyAgent) {
    m_enemyAgent->reportWin(!playerWon);
    m_enemyAgent->incrementEpisodeCount();
  }
}

void CombatSystem::resetCharacter(Character &character,
                                  const Vector2f &position) {
  SDL_Rect charRect = character.animator->getCurrentFrameRect();
  float groundedY = m_config.groundLevel - charRect.h;

  character.mover.position = Vector2f(position.x, groundedY);
  character.mover.velocity = Vector2f(0, 0);
  character.health = character.maxHealth;
  character.onGround = true;
  character.groundFrames = m_config.stableGroundFrames;
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
