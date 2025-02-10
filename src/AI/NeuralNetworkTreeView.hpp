#pragma once
#include "AI/NeuralNetwork.hpp"
#include "imgui.h"

class NeuralNetworkTreeView {
public:
  // Construct with a pointer to the network to visualize.
  NeuralNetworkTreeView(const NeuralNetwork *network);

  // Render the network tree view in the current ImGui window.
  void render();

private:
  const NeuralNetwork *m_network;

  // Helper function: map a weight's absolute value to a color.
  ImU32 getWeightColor(float weight) const;
};
