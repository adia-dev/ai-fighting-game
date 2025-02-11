enum class HitboxType { Hit, Collision, Block, Grab };

enum class FramePhase { None, Startup, Active, Recovery };

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
