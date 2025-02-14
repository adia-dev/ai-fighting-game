#include <vector>

struct LayerNormalization {
  std::vector<float> running_mean;
  std::vector<float> running_var;
  std::vector<float> gamma;
  std::vector<float> beta;
  float epsilon = 1e-5f;
  float momentum = 0.99f;

  LayerNormalization(int size) {
    running_mean.resize(size, 0.0f);
    running_var.resize(size, 1.0f);
    gamma.resize(size, 1.0f);
    beta.resize(size, 0.0f);
  }

  std::vector<float> normalize(const std::vector<float> &input,
                               bool training = true) {
    std::vector<float> output(input.size());

    float mean = 0.0f;
    float var = 0.0f;

    for (float val : input) {
      mean += val;
    }
    mean /= input.size();

    for (float val : input) {
      float diff = val - mean;
      var += diff * diff;
    }
    var /= input.size();

    if (training) {
      for (size_t i = 0; i < input.size(); ++i) {
        running_mean[i] = momentum * running_mean[i] + (1 - momentum) * mean;
        running_var[i] = momentum * running_var[i] + (1 - momentum) * var;
      }
    }

    float std_dev = std::sqrt(var + epsilon);
    for (size_t i = 0; i < input.size(); ++i) {
      output[i] = gamma[i] * ((input[i] - mean) / std_dev) + beta[i];
    }

    return output;
  }
};
