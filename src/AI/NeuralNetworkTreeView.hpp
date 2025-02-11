#pragma once
#include "AI/NeuralNetwork.hpp"
#include "imgui.h"

class NeuralNetworkTreeView {
public:
  NeuralNetworkTreeView(const NeuralNetwork *network);

  void render();

private:
  const NeuralNetwork *m_network;

  ImU32 getWeightColor(float weight) const;
};
