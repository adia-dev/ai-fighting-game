// src/Core/NeuralNetworkTreeView.cpp
#include "NeuralNetworkTreeView.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <sstream>

// Constructor
NeuralNetworkTreeView::NeuralNetworkTreeView(const NeuralNetwork *network)
    : m_network(network) {}

// Helper: map weight magnitude to a color (blue for low, red for high)
ImU32 NeuralNetworkTreeView::getWeightColor(float weight) const {
  float normalized = std::min(1.0f, std::fabs(weight)); // clamp to [0,1]
  // For example, low weights blue, high weights red.
  int r = static_cast<int>(normalized * 255);
  int g = static_cast<int>((1.0f - normalized) * 255);
  int b = 128;
  return IM_COL32(r, g, b, 255);
}

void NeuralNetworkTreeView::render() {
  if (!m_network)
    return;
  // Retrieve the network layers using our accessor.
  const std::vector<Layer> &layers = m_network->getLayers();
  if (layers.empty())
    return;

  ImGui::Begin("Neural Network Tree View");

  // Get current ImGui window draw list and window position.
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 winPos = ImGui::GetWindowPos();
  ImVec2 winSize = ImGui::GetWindowSize();

  // Define margins.
  float marginX = 50.0f;
  float marginY = 50.0f;
  float availableWidth = winSize.x - 2 * marginX;
  float availableHeight = winSize.y - 2 * marginY;

  int numLayers = layers.size();
  std::vector<float> layerXPositions;
  // Compute X positions for each layer.
  for (int i = 0; i < numLayers; ++i) {
    float x = winPos.x + marginX + (availableWidth * i) / (numLayers - 1);
    layerXPositions.push_back(x);
  }

  // Compute node positions for each layer.
  std::vector<std::vector<ImVec2>> nodePositions(numLayers);
  for (int l = 0; l < numLayers; ++l) {
    const Layer &layer = layers[l];
    int nNeurons = layer.outputSize;
    std::vector<ImVec2> positions;
    for (int i = 0; i < nNeurons; ++i) {
      // Evenly space neurons vertically.
      float y = winPos.y + marginY + availableHeight * (i + 0.5f) / nNeurons;
      positions.push_back(ImVec2(layerXPositions[l], y));
    }
    nodePositions[l] = positions;
  }

  // Draw connections between layers.
  for (int l = 0; l < numLayers - 1; ++l) {
    const Layer &currentLayer = layers[l];
    const Layer &nextLayer = layers[l + 1];
    for (int i = 0; i < currentLayer.outputSize; ++i) {
      ImVec2 start = nodePositions[l][i];
      for (int j = 0; j < nextLayer.outputSize; ++j) {
        ImVec2 end = nodePositions[l + 1][j];
        // Use weight value from the next layer (from neuron j connected to i).
        float weight = nextLayer.weights[j][i];
        ImU32 col = getWeightColor(weight);
        draw_list->AddLine(start, end, col, 1.0f);
      }
    }
  }

  // Draw nodes for each layer.
  float nodeRadius = 5.0f;
  for (int l = 0; l < numLayers; ++l) {
    for (const auto &pos : nodePositions[l]) {
      draw_list->AddCircleFilled(pos, nodeRadius, IM_COL32(200, 200, 200, 255));
    }
  }

  // Optionally, draw layer labels.
  for (int l = 0; l < numLayers; ++l) {
    std::stringstream ss;
    ss << "Layer " << l;
    draw_list->AddText(ImVec2(layerXPositions[l] - 20, winPos.y + 10),
                       IM_COL32(255, 255, 255, 255), ss.str().c_str());
  }

  ImGui::End();
}
