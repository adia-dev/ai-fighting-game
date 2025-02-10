#include "NeuralNetworkVisualizer.hpp"
#include "Core/Logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp> // Make sure you have the JSON library available
#include <sstream>

using json = nlohmann::json;

NeuralNetworkVisualizer::NeuralNetworkVisualizer(NeuralNetwork *network)
    : network(network) {}

void NeuralNetworkVisualizer::render() {
  if (!network)
    return;

  ImGui::Begin("Neural Network Visualizer");

  // We assume your NeuralNetwork has a GetLayers() method (added later)
  const auto &layers = network->getLayers();
  ImGui::Text("Neural Network Structure:");
  for (size_t i = 0; i < layers.size(); i++) {
    const Layer &layer = layers[i];
    std::stringstream ss;
    ss << "Layer " << i << ": " << layer.inputSize << " -> " << layer.outputSize
       << " ("
       << (layer.activation == ActivationType::ReLU
               ? "ReLU"
               : (layer.activation == ActivationType::Sigmoid ? "Sigmoid"
                                                              : "None"))
       << ")";
    if (ImGui::CollapsingHeader(ss.str().c_str())) {
      ImGui::Text("First few weights:");
      for (int r = 0; r < layer.outputSize && r < 5; r++) {
        std::string row;
        for (int c = 0; c < layer.inputSize && c < 5; c++) {
          row += std::to_string(layer.weights[r][c]) + " ";
        }
        ImGui::Text("%s", row.c_str());
      }
      ImGui::Text("Biases (first 10):");
      std::string biasesStr;
      for (int i = 0; i < layer.outputSize && i < 10; i++) {
        biasesStr += std::to_string(layer.biases[i]) + " ";
      }
      ImGui::Text("%s", biasesStr.c_str());
    }
  }

  // Buttons for model operations.
  if (ImGui::Button("Export Model")) {
    if (ExportModel("model_export.json"))
      Logger::info("Model exported successfully.");
    else
      Logger::error("Failed to export model.");
  }
  ImGui::SameLine();
  if (ImGui::Button("Import Model")) {
    if (ImportModel("model_export.json"))
      Logger::info("Model imported successfully.");
    else
      Logger::error("Failed to import model.");
  }
  ImGui::SameLine();
  if (ImGui::Button("Capture Snapshot")) {
    CaptureSnapshot("snapshot.json");
    Logger::info("Snapshot captured.");
  }

  ImGui::End();
}

bool NeuralNetworkVisualizer::ExportModel(const std::string &filename) {
  json j;
  const auto &layers = network->getLayers();
  j["layers"] = json::array();
  for (const auto &layer : layers) {
    json jLayer;
    jLayer["inputSize"] = layer.inputSize;
    jLayer["outputSize"] = layer.outputSize;
    jLayer["activation"] = static_cast<int>(layer.activation);
    jLayer["biases"] = layer.biases;
    jLayer["weights"] = layer.weights;
    j["layers"].push_back(jLayer);
  }
  std::ofstream ofs(filename);
  if (!ofs.is_open())
    return false;
  ofs << j.dump(4);
  ofs.close();
  return true;
}

bool NeuralNetworkVisualizer::ImportModel(const std::string &filename) {
  std::ifstream ifs(filename);
  if (!ifs.is_open())
    return false;
  json j;
  ifs >> j;
  ifs.close();
  network->clearLayers();
  for (const auto &jLayer : j["layers"]) {
    int inputSize = jLayer["inputSize"];
    int outputSize = jLayer["outputSize"];
    ActivationType activation =
        static_cast<ActivationType>(jLayer["activation"].get<int>());
    network->addLayer(outputSize, activation);
    size_t layerIndex = network->numLayers() - 1;
    network->setLayerParameters(
        layerIndex, jLayer["weights"].get<std::vector<std::vector<float>>>(),
        jLayer["biases"].get<std::vector<float>>());
  }
  return true;
}

void NeuralNetworkVisualizer::CaptureSnapshot(const std::string &filename) {
  ExportModel(filename);
}
