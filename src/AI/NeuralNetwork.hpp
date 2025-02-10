#pragma once
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

  Layer(int inSize, int outSize, ActivationType act)
      : inputSize(inSize), outputSize(outSize), activation(act) {
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

  size_t numLayers() const { return layers.size(); }

private:
  int inputSize;
  std::vector<Layer> layers;

  void initializeLayer(Layer &layer, std::mt19937 &gen);
};
