enum class HitboxType {
  Hit,       // For an attacking hitbox
  Collision, // For general collision (e.g. physics)
  Block,     // For a blocking/hurtbox (when defending)
  Grab       // For grab moves
};

enum class FramePhase {
  None,
  Startup, // Before the move is “active”
  Active,  // The move is active: hitboxes can hit the opponent
  Recovery // After active, move is winding down
};

namespace {
static inline const char *frame_phase_to_string(FramePhase phase) {
  switch (phase) {
  case FramePhase::None:
    return "None";
  case FramePhase::Startup:
    return "Startup";
  case FramePhase::Active:
    return "Active";
  case FramePhase::Recovery:
    return "Recovery";
  }
}
} // namespace
