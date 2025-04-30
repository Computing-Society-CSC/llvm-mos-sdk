#include "mnist_weights_int8.h" // Use the header file with int8 weights and quantization params
#include <limits.h>             // Required for INT8_MIN
#include <stdint.h>             // Required for int8_t, int32_t
#include <math.h>               // Required for round
#include <stdio.h>
#include <6502fun.h>

// Define intermediate buffers
static float layer1_accumulators_float[L1_OUTPUT_SIZE];

// Input quantization parameters (implicit in the quantization step)
// Assuming input image pixels are 0-255 grayscale
#define INPUT_ZERO_POINT 0
#define INPUT_SCALE (1.0f / 255.0f)

#define LEAKY_RELU_ALPHA 0.01f

float roundf(float x) {
    if (x >= 0) {
        if (x - (int)x >= 0.5) {
            return (int)x + 1;
        } else {
            return (int)x;
        }
    } else {
        if ((int)x - x >= 0.5) {
            return (int)x - 1;
        } else {
            return (int)x;
        }
    }
}

// Helper function for quantization
static inline int8_t quantize_float_to_int8(float value, float scale, int32_t zero_point)
{
  int32_t quant_val = (int32_t)roundf(value / scale) + zero_point;
  if (quant_val < -128)
    quant_val = -128;
  if (quant_val > 127)
    quant_val = 127;
  return (int8_t)quant_val;
}

// Helper function for dequantization
static inline float dequantize_int8_to_float(int8_t value, float scale, int32_t zero_point)
{
  return ((float)value - zero_point) * scale;
}

static inline float dequantize_int8_to_float16(int8_t value, float scale, int32_t zero_point)
{
  return ((float)value - zero_point) * scale;
}

int main()
{
printf("Starting mnist_ml_test\n");
  for (uint8_t j = 0; j < L1_OUTPUT_SIZE; j++)
  {
    int8_t bias_quant = L1_BIASES_INT8[j];
    layer1_accumulators_float[j] = dequantize_int8_to_float(bias_quant, L1_BIAS_SCALE, L1_BIAS_ZERO_POINT);
  }
  printf("Accumulator initialized\n");

  for (uint8_t i = 0; i < L1_INPUT_SIZE; i++)
  {
    uint8_t p = 0; // Placeholder: should be each pixel

    float dequantized_input = (float)p * INPUT_SCALE;

    for (uint8_t j = 0; j < L1_OUTPUT_SIZE; j++)
    {
      int8_t weight_quant = L1_WEIGHTS_INT8[j][i];
      float dequantized_weight = dequantize_int8_to_float(weight_quant, L1_WEIGHT_SCALE, L1_WEIGHT_ZERO_POINT);

      layer1_accumulators_float[j] += dequantized_weight * dequantized_input;
    }
    printf("Layer 1: %d/%d neurons\n", i+1, L1_INPUT_SIZE);
  }

  for (uint8_t j = 0; j < L1_OUTPUT_SIZE; j++)
  {
    if (layer1_accumulators_float[j] < 0.0f)
    {
      layer1_accumulators_float[j] *= LEAKY_RELU_ALPHA;
    }
    printf("LeakyRelu: %d/%d neurons\n", j+1, L1_OUTPUT_SIZE);
  }

  float max_val = -__FLT_MAX__;
  uint8_t pred = 0;
  for (uint8_t j = 0; j < L2_OUTPUT_SIZE; j++)
  {
    int8_t bias_quant = L2_BIASES_INT8[j];
    float accumulator_float = dequantize_int8_to_float(bias_quant, L2_BIAS_SCALE, L2_BIAS_ZERO_POINT);

    for (uint8_t i = 0; i < L2_INPUT_SIZE; i++)
    {
      float dequantized_hidden_input = layer1_accumulators_float[i];

      int8_t weight_quant = L2_WEIGHTS_INT8[j][i];
      float dequantized_weight = dequantize_int8_to_float(weight_quant, L2_WEIGHT_SCALE, L2_WEIGHT_ZERO_POINT);

      accumulator_float += dequantized_weight * dequantized_hidden_input;
    }
    if (accumulator_float > max_val)
    {
      max_val = accumulator_float;
      pred = (uint8_t)j;
    }
    printf("Layer 2: %d/%d neurons\n", j+1, L2_OUTPUT_SIZE);
  }

  printf("Prediction: %d", pred);
}