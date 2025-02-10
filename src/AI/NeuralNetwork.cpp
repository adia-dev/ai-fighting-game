#include "NeuralNetwork.hpp"

NeuralNetwork::NeuralNetwork(int inputSize) : inputSize(inputSize) {}

void NeuralNetwork::addLayer(int numNeurons, ActivationType activation) {
  int currentInputSize = layers.empty() ? inputSize : layers.back().outputSize;
  Layer newLayer(currentInputSize, numNeurons, activation);

  std::random_device rd;
  std::mt19937 gen(rd());
  initializeLayer(newLayer, gen);

  layers.push_back(newLayer);
}

void NeuralNetwork::initializeLayer(Layer &layer, std::mt19937 &gen) {
  float stddev = 1.0f;
  if (layer.activation == ActivationType::ReLU) {
    stddev = std::sqrt(2.0f / layer.inputSize);
  } else {
    stddev = std::sqrt(1.0f / layer.inputSize);
  }
  std::normal_distribution<float> dist(0.0f, stddev);
  for (int i = 0; i < layer.outputSize; ++i) {
    for (int j = 0; j < layer.inputSize; ++j) {
      layer.weights[i][j] = dist(gen);
    }
    layer.biases[i] = 0.0f;
  }
}

std::vector<float> NeuralNetwork::forward(const std::vector<float> &input) {
  std::vector<float> activationInput = input;

  for (auto &layer : layers) {
    layer.lastInput = activationInput;
    layer.lastZ.resize(layer.outputSize, 0.0f);
    std::vector<float> layerOutput(layer.outputSize, 0.0f);
    for (int i = 0; i < layer.outputSize; ++i) {
      float sum = layer.biases[i];
      for (int j = 0; j < layer.inputSize; ++j) {
        sum += layer.weights[i][j] * activationInput[j];
      }
      layer.lastZ[i] = sum;
      layerOutput[i] = activate(sum, layer.activation);
    }
    layer.lastOutput = layerOutput;
    activationInput = layerOutput;
  }
  return activationInput;
}

void NeuralNetwork::train(const std::vector<float> &input,
                          const std::vector<float> &target,
                          float learningRate) {

  std::vector<float> output = forward(input);
  assert(output.size() == target.size());

  std::vector<float> delta(output.size(), 0.0f);
  for (size_t i = 0; i < output.size(); ++i) {
    delta[i] = output[i] - target[i];
  }

  for (int l = layers.size() - 1; l >= 0; --l) {
    Layer &layer = layers[l];
    std::vector<float> deltaPrev(layer.inputSize, 0.0f);

    for (int i = 0; i < layer.outputSize; ++i) {

      float dActivation = activateDerivative(layer.lastZ[i], layer.activation);
      float delta_i = delta[i] * dActivation;

      layer.biases[i] -= learningRate * delta_i;

      for (int j = 0; j < layer.inputSize; ++j) {
        float grad = delta_i * layer.lastInput[j];
        layer.weights[i][j] -= learningRate * grad;
        deltaPrev[j] += layer.weights[i][j] * delta_i;
      }
    }
    delta = deltaPrev;
  }
}
