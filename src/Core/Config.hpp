#pragma once

struct Config {

  int windowWidth = 1440;
  int windowHeight = 900;

  float gravity = 980.0f;
  float friction = 2.0f;
  float moveForce = 500.0f;
  float enemyFollowForce = 200.0f;

  int groundLevel = 800;
  int groundThreshold = 10;
  int stableGroundFrames = 3;

  float jumpVelocity = -700.0f;
  float dashForce = 2000.0f;
  float attackForce = 500.0f;

  float minDistance = 100.0f;
  float maxDistance = 500.0f;
  float maxZoom = 2.0f;
  float minZoom = 0.5f;
  float cameraSmoothFactor = 0.1f;

  float baseBlockReduction = 0.1f;
  float comboScaling = 1.25f;
  float knockbackForce = 500.0f;
  float knockbackComboScaling = 0.1f;

  struct AIConfig {

    float deadzoneBoundary = 150.0f;
    float optimalDistance = 200.0f;

    float healthDiffReward = 15.0f;
    float hitReward = 25.0f;
    float missPenalty = -8.0f;
    float blockReward = 15.0f;
    float blockPenalty = -5.0f;

    float deadzoneBasePenalty = -20.0f;
    float deadzoneDepthPenalty = -30.0f;
    float moveIntoDeadzonePenalty = -25.0f;
    float escapeDeadzoneReward = 15.0f;
    float distanceMultiplier = 0.05f;

    float comboBaseMultiplier = 0.5f;
    float maxComboMultiplier = 3.0f;
    float optimalDistanceBonus = 15.0f;
    float farWhiffPenalty = -12.0f;

    float noStaminaPenalty = -50.0f;
    float lowStaminaPenalty = -15.0f;
    float lowStaminaThreshold = 0.2f;

    float repeatActionPenalty = -20.0f;
    float wellTimedBlockBonus = 10.0f;
  } ai;
};
