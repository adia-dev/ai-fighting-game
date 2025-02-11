#pragma once
#include "AI/NeuralNetwork.hpp"
#include "imgui.h"
#include <string>

class NeuralNetworkVisualizer {
public:
  NeuralNetworkVisualizer(NeuralNetwork *network);

  void render();

  bool ExportModel(const std::string &filename);

  bool ImportModel(const std::string &filename);

  void CaptureSnapshot(const std::string &filename);

private:
  NeuralNetwork *network;
};
