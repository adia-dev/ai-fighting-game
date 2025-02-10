#pragma once
#include "AI/NeuralNetwork.hpp"
#include "imgui.h"
#include <string>

class NeuralNetworkVisualizer {
public:
  // Construct with a pointer to the neural network to visualize.
  NeuralNetworkVisualizer(NeuralNetwork *network);

  // Render the network structure and interactive UI.
  void render();

  // Export the current network model to a JSON file.
  bool ExportModel(const std::string &filename);

  // Import a model from a JSON file.
  bool ImportModel(const std::string &filename);

  // Capture a snapshot of the network (for example, save to a snapshot file).
  void CaptureSnapshot(const std::string &filename);

private:
  NeuralNetwork *network;
};
