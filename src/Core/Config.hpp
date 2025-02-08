#pragma once

struct Config {

  int windowWidth = 800;
  int windowHeight = 600;

  int groundLevel = 500;
  int groundThreshold = 5;
  int stableGroundFrames = 3;

  float gravity = 980.0f;
  float defaultMoveForce = 500.0f;
  float jumpVelocity = -500.0f;
  float enemyFollowForce = 200.0f;
  float friction = 2.0f;

  float minDistance = 100.0f;
  float maxDistance = 500.0f;
  float maxZoom = 2.0f;
  float minZoom = 0.5f;
  float cameraSmoothFactor = 0.1f;
};
