#include "Game.hpp"
#include <iostream>

int main() {
  try {
    Game game;
    game.run();
  } catch (const std::exception &e) {
    std::cerr << "Game failed to start: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
