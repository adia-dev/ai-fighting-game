#pragma once
#include <cassert>
#include <random>
#include <vector>

class NeuralNetwork {
public:
  // A simple network with one hidden layer.
  NeuralNetwork(int input_size, int hidden_size, int output_size)
      : input_size(input_size), hidden_size(hidden_size),
        output_size(output_size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.1f, 0.1f);

    // Initialize first layer: weights1[hidden_size][input_size] and
    // biases1[hidden_size]
    weights1.resize(hidden_size, std::vector<float>(input_size));
    biases1.resize(hidden_size, 0.0f);
    for (int i = 0; i < hidden_size; ++i)
      for (int j = 0; j < input_size; ++j)
        weights1[i][j] = dist(gen);

    // Initialize second layer: weights2[output_size][hidden_size] and
    // biases2[output_size]
    weights2.resize(output_size, std::vector<float>(hidden_size));
    biases2.resize(output_size, 0.0f);
    for (int i = 0; i < output_size; ++i)
      for (int j = 0; j < hidden_size; ++j)
        weights2[i][j] = dist(gen);
  }

  // Forward pass: returns a vector of Q-values.
  std::vector<float> forward(const std::vector<float> &input) {
    assert(input.size() == input_size);
    hidden_output.resize(hidden_size);
    // Hidden layer: h = ReLU(W1 * input + b1)
    for (int i = 0; i < hidden_size; ++i) {
      float sum = biases1[i];
      for (int j = 0; j < input_size; ++j)
        sum += weights1[i][j] * input[j];
      hidden_output[i] = relu(sum);
    }
    // Output layer: out = W2 * hidden_output + b2 (linear)
    std::vector<float> output(output_size, 0.0f);
    for (int i = 0; i < output_size; ++i) {
      float sum = biases2[i];
      for (int j = 0; j < hidden_size; ++j)
        sum += weights2[i][j] * hidden_output[j];
      output[i] = sum;
    }
    return output;
  }

  // Train on one sample (input and target output) using gradient descent.
  void train(const std::vector<float> &input, const std::vector<float> &target,
             float learning_rate) {
    std::vector<float> output = forward(input);
    assert(output.size() == target.size());

    // Compute derivative of MSE loss: dLoss/dOutput = output - target
    std::vector<float> dLoss_dOut(output_size);
    for (int i = 0; i < output_size; ++i)
      dLoss_dOut[i] = output[i] - target[i];

    // Gradients for layer 2.
    std::vector<std::vector<float>> gradW2(
        output_size, std::vector<float>(hidden_size, 0.0f));
    std::vector<float> gradB2(output_size, 0.0f);
    for (int i = 0; i < output_size; ++i) {
      gradB2[i] = dLoss_dOut[i];
      for (int j = 0; j < hidden_size; ++j)
        gradW2[i][j] = dLoss_dOut[i] * hidden_output[j];
    }

    // Backpropagate into hidden layer.
    std::vector<float> dLoss_dHidden(hidden_size, 0.0f);
    for (int j = 0; j < hidden_size; ++j) {
      float sum = 0.0f;
      for (int i = 0; i < output_size; ++i)
        sum += weights2[i][j] * dLoss_dOut[i];
      float d_relu = (hidden_output[j] > 0.0f) ? 1.0f : 0.0f;
      dLoss_dHidden[j] = sum * d_relu;
    }

    // Gradients for layer 1.
    std::vector<std::vector<float>> gradW1(
        hidden_size, std::vector<float>(input_size, 0.0f));
    std::vector<float> gradB1(hidden_size, 0.0f);
    for (int j = 0; j < hidden_size; ++j) {
      gradB1[j] = dLoss_dHidden[j];
      for (int k = 0; k < input_size; ++k)
        gradW1[j][k] = dLoss_dHidden[j] * input[k];
    }

    // Update parameters.
    for (int i = 0; i < output_size; ++i) {
      biases2[i] -= learning_rate * gradB2[i];
      for (int j = 0; j < hidden_size; ++j)
        weights2[i][j] -= learning_rate * gradW2[i][j];
    }
    for (int j = 0; j < hidden_size; ++j) {
      biases1[j] -= learning_rate * gradB1[j];
      for (int k = 0; k < input_size; ++k)
        weights1[j][k] -= learning_rate * gradW1[j][k];
    }
  }

private:
  inline float relu(float x) { return (x > 0) ? x : 0; }

  int input_size, hidden_size, output_size;
  std::vector<std::vector<float>> weights1;
  std::vector<float> biases1;
  std::vector<std::vector<float>> weights2;
  std::vector<float> biases2;
  std::vector<float> hidden_output;
};
