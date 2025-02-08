#include "CollisionSystem.hpp"
#include <SDL.h>

bool CollisionSystem::checkCollision(const SDL_Rect &a, const SDL_Rect &b) {
  return SDL_HasIntersection(&a, &b);
}

void CollisionSystem::resolveCollision(Character &a, Character &b) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();

  if (!checkCollision(rectA, rectB))
    return;

  SDL_Rect intersection;
  SDL_IntersectRect(&rectA, &rectB, &intersection);

  if (intersection.w < intersection.h) {
    int separation = intersection.w / 2;
    if (rectA.x < rectB.x) {
      a.mover.position.x -= separation;
      b.mover.position.x += separation;
    } else {
      a.mover.position.x += separation;
      b.mover.position.x -= separation;
    }
  } else {
    int separation = intersection.h / 2;
    if (rectA.y < rectB.y) {
      a.mover.position.y -= separation;
      b.mover.position.y += separation;
    } else {
      a.mover.position.y += separation;
      b.mover.position.y -= separation;
    }
  }
}

void CollisionSystem::applyCollisionImpulse(Character &a, Character &b,
                                            float impulseStrength) {
  SDL_Rect rectA = a.getCollisionRect();
  SDL_Rect rectB = b.getCollisionRect();

  // Compute centers of the collision rectangles.
  Vector2f centerA(rectA.x + rectA.w / 2.0f, rectA.y + rectA.h / 2.0f);
  Vector2f centerB(rectB.x + rectB.w / 2.0f, rectB.y + rectB.h / 2.0f);
  Vector2f collisionNormal = (centerB - centerA).normalized();

  a.mover.applyForce(collisionNormal * -impulseStrength);
  b.mover.applyForce(collisionNormal * impulseStrength);
}
