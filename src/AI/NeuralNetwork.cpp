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
      layer.weights[i][j] = dist(gen) * 1e-3;
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
  const float delta_threshold = 1.0f;
  for (size_t i = 0; i < output.size(); ++i) {
    float error = output[i] - target[i];
    if (std::fabs(error) <= delta_threshold)
      delta[i] = error;
    else
      delta[i] = delta_threshold * ((error > 0) ? 1.0f : -1.0f);
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
void NeuralNetwork::heInitialization(Layer &layer) {
  std::random_device rd;
  std::mt19937 gen(rd());
  float std_dev = std::sqrt(2.0f / layer.inputSize);
  std::normal_distribution<float> dist(0.0f, std_dev);

  for (int i = 0; i < layer.outputSize; ++i) {
    for (int j = 0; j < layer.inputSize; ++j) {
      layer.weights[i][j] = dist(gen);
    }
    layer.biases[i] = 0.0f;
  }
}

std::vector<float>
NeuralNetwork::normalizeInput(const std::vector<float> &input,
                              const std::vector<float> &input_min,
                              const std::vector<float> &input_max) {
  std::vector<float> normalized(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    float range = input_max[i] - input_min[i];
    if (range > 0) {
      normalized[i] = 2.0f * (input[i] - input_min[i]) / range - 1.0f;
    } else {
      normalized[i] = input[i];
    }
  }
  return normalized;
}
void NeuralNetwork::clipGradients(std::vector<std::vector<float>> &gradients,
                                  float max_norm) {
  float total_norm = 0.0f;
  for (const auto &grad_vec : gradients) {
    for (float grad : grad_vec) {
      total_norm += grad * grad;
    }
  }
  total_norm = std::sqrt(total_norm);

  if (total_norm > max_norm) {
    float scale = max_norm / total_norm;
    for (auto &grad_vec : gradients) {
      for (float &grad : grad_vec) {
        grad *= scale;
      }
    }
  }
}
