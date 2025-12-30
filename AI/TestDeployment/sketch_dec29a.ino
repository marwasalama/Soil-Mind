/*
 * SoilMind - Direct Tensor Access for INT8 Model
 */
#include <ArduTFLite.h>
#include "irrigation_model.h"

constexpr int kArenaSize = 8192;
uint8_t tensor_arena[kArenaSize];

// External access to interpreter (defined in ArduTFLite)
extern tflite::MicroInterpreter* tflInterpreter;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n================================");
    Serial.println("SoilMind - Direct Tensor Access");
    Serial.println("================================\n");

    if (!modelInit(irrigation_model_data, tensor_arena, kArenaSize)) {
        Serial.println("Model init failed!");
        while(1);
    }
    
    // Get tensor info
    TfLiteTensor* input = tflInterpreter->input(0);
    TfLiteTensor* output = tflInterpreter->output(0);
    
    Serial.printf("Input tensor - dims: %d, size: [", input->dims->size);
    for (int i = 0; i < input->dims->size; i++) {
        Serial.printf("%d", input->dims->data[i]);
        if (i < input->dims->size - 1) Serial.print(", ");
    }
    Serial.printf("], type: %d\n", input->type);
    
    Serial.printf("Output tensor - dims: %d, size: [", output->dims->size);
    for (int i = 0; i < output->dims->size; i++) {
        Serial.printf("%d", output->dims->data[i]);
        if (i < output->dims->size - 1) Serial.print(", ");
    }
    Serial.printf("], type: %d\n", output->type);
    
    Serial.println("\nModel loaded!\n");
    runTests();
}

bool predictIrrigation(float soil_moisture, float temperature, float humidity) {
    TfLiteTensor* input = tflInterpreter->input(0);
    TfLiteTensor* output = tflInterpreter->output(0);
    
    // Normalize
    float norm[3];
    norm[0] = normalize(soil_moisture, SOIL_MOISTURE_MIN, SOIL_MOISTURE_MAX);
    norm[1] = normalize(temperature, TEMPERATURE_MIN, TEMPERATURE_MAX);
    norm[2] = normalize(humidity, HUMIDITY_MIN, HUMIDITY_MAX);
    
    // Set inputs based on tensor type
    if (input->type == kTfLiteInt8) {
        input->data.int8[0] = quantize(norm[0]);
        input->data.int8[1] = quantize(norm[1]);
        input->data.int8[2] = quantize(norm[2]);
    } else if (input->type == kTfLiteFloat32) {
        input->data.f[0] = norm[0];
        input->data.f[1] = norm[1];
        input->data.f[2] = norm[2];
    }

    // Run inference
    if (tflInterpreter->Invoke() != kTfLiteOk) {
        Serial.println("Invoke failed!");
        return false;
    }

    // Get output
    float probability;
    if (output->type == kTfLiteInt8) {
        probability = dequantize(output->data.int8[0]);
    } else {
        probability = output->data.f[0];
    }
    
    Serial.printf("  Probability: %.4f -> %s\n", probability, 
                  probability >= 0.5 ? "IRRIGATE" : "OFF");
    
    return probability >= IRRIGATION_THRESHOLD;
}

void runTests() {
    Serial.println("=== Tests ===\n");
    
    struct Test { float sm, temp, hum; };
    Test tests[] = {{20,30,50}, {75,25,60}, {35,38,30}, {60,22,65}};

    for (int i = 0; i < 4; i++) {
        Serial.printf("Test %d: SM=%.0f%%, T=%.0f, H=%.0f%%\n", 
                      i+1, tests[i].sm, tests[i].temp, tests[i].hum);
        predictIrrigation(tests[i].sm, tests[i].temp, tests[i].hum);
        Serial.println();
    }
}

void loop() {
    delay(5000);
    float sm = random(10, 80);
    float temp = 15.0 + random(0, 30);
    float hum = 20.0 + random(0, 60);
    
    Serial.printf("\nSM=%.1f%%, T=%.1f, H=%.1f%%\n", sm, temp, hum);
    predictIrrigation(sm, temp, hum);
}