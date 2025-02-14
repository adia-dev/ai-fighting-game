#pragma once
#include "LayerNormalization.hpp"
#include <cassert>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

enum class ActivationType { None, ReLU, Sigmoid };

inline float activate(float x, ActivationType act) {
  switch (act) {
  case ActivationType::ReLU:
    return x > 0 ? x : 0;
  case ActivationType::Sigmoid:
    return 1.0f / (1.0f + std::exp(-x));
  case ActivationType::None:
  default:
    return x;
  }
}

inline float activateDerivative(float x, ActivationType act) {
  switch (act) {
  case ActivationType::ReLU:
    return x > 0 ? 1.0f : 0.0f;
  case ActivationType::Sigmoid: {
    float sig = 1.0f / (1.0f + std::exp(-x));
    return sig * (1 - sig);
  }
  case ActivationType::None:
  default:
    return 1.0f;
  }
}

struct Layer {
  int inputSize;
  int outputSize;
  ActivationType activation;
  std::vector<std::vector<float>> weights;
  std::vector<float> biases;

  std::vector<float> lastInput;
  std::vector<float> lastZ;
  std::vector<float> lastOutput;

  LayerNormalization normalization;
  bool use_normalization;

  Layer(int inSize, int outSize, ActivationType act)
      : inputSize(inSize), outputSize(outSize), activation(act),
        normalization(outSize), use_normalization(true) {
    weights.resize(outSize, std::vector<float>(inSize, 0.0f));
    biases.resize(outSize, 0.0f);
  }
};

class NeuralNetwork {
public:
  NeuralNetwork(int inputSize);

  void addLayer(int numNeurons, ActivationType activation);

  std::vector<float> forward(const std::vector<float> &input);

  void train(const std::vector<float> &input, const std::vector<float> &target,
             float learningRate);

  static std::vector<float> normalizeInput(const std::vector<float> &input,
                                           const std::vector<float> &input_min,
                                           const std::vector<float> &input_max);

  void heInitialization(Layer &layer);

  const std::vector<Layer> &getLayers() const { return layers; }
  void clearLayers() { layers.clear(); }
  void setLayerParameters(size_t layerIndex,
                          const std::vector<std::vector<float>> &weights,
                          const std::vector<float> &biases) {
    if (layerIndex >= layers.size())
      throw std::runtime_error("Invalid layer index");
    layers[layerIndex].weights = weights;
    layers[layerIndex].biases = biases;
  }
  size_t numLayers() const { return layers.size(); }

private:
  int inputSize;
  std::vector<Layer> layers;

  void initializeLayer(Layer &layer, std::mt19937 &gen);
  static void clipGradients(std::vector<std::vector<float>> &gradients,
                            float max_norm = 5.0f);
};
