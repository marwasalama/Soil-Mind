// ============================================================================
// PLANT HEALTH CLASSIFICATION - Using Chirale_TensorFlowLite Directly
// ============================================================================

#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "plant_health_model.h"

// Tensor arena
constexpr int kTensorArenaSize = 16 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

// TFLite Micro globals
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* inputTensor = nullptr;
TfLiteTensor* outputTensor = nullptr;

// Feature names
const char* FEATURE_NAMES[] = {"N", "P", "K", "pH", "Moisture", "Temp"};

// ============================================================================
// TEST DATA
// ============================================================================
float testHealthy[]     = {85.0, 55.0, 45.0, 6.5, 50.0, 25.0};
float testLowN[]        = {5.0,  50.0, 40.0, 6.5, 45.0, 24.0};
float testLowP[]        = {60.0, 10.0, 40.0, 6.4, 50.0, 25.0};
float testLowK[]        = {70.0, 55.0, 8.0,  6.5, 48.0, 24.0};
float testWaterStress[] = {80.0, 50.0, 42.0, 6.8, 5.0,  28.0};
float testAcidic[]      = {65.0, 48.0, 38.0, 4.5, 45.0, 23.0};
float testAlkaline[]    = {70.0, 52.0, 40.0, 8.5, 50.0, 26.0};

// ============================================================================
// INFERENCE FUNCTION
// ============================================================================
int predict(float* rawFeatures) {
    // 1. Normalize and quantize inputs
    for (int i = 0; i < NUM_FEATURES; i++) {
        float normalized = (rawFeatures[i] - FEATURE_MEANS[i]) / FEATURE_STDS[i];
        // Quantize: q = (value / scale) + zero_point
        int32_t quantized = (int32_t)(normalized / INPUT_SCALE) + INPUT_ZERO_POINT;
        inputTensor->data.int8[i] = (int8_t)constrain(quantized, -128, 127);
    }
    
    // 2. Run inference
    TfLiteStatus invokeStatus = interpreter->Invoke();
    if (invokeStatus != kTfLiteOk) {
        Serial.println("Invoke failed!");
        return -1;
    }
    
    // 3. Read and dequantize outputs
    Serial.println("\nProbabilities:");
    int maxIdx = 0;
    float maxProb = -999.0;
    
    for (int i = 0; i < NUM_CLASSES; i++) {
        // Dequantize: value = (q - zero_point) * scale
        int8_t q = outputTensor->data.int8[i];
        float prob = (q - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
        prob = constrain(prob, 0.0f, 1.0f);
        
        if (prob > maxProb) {
            maxProb = prob;
            maxIdx = i;
        }
        
        Serial.print("  ");
        Serial.print(CLASS_LABELS[i]);
        Serial.print(": ");
        Serial.print(prob * 100, 1);
        Serial.print("%");
        if (i == maxIdx) Serial.print(" <--");
        Serial.println();
    }
    
    return maxIdx;
}

// ============================================================================
// RUN TEST
// ============================================================================
void runTest(const char* name, float* features) {
    Serial.println("\n========================================");
    Serial.print("TEST: ");
    Serial.println(name);
    Serial.println("----------------------------------------");
    
    Serial.println("Input:");
    for (int i = 0; i < NUM_FEATURES; i++) {
        Serial.print("  ");
        Serial.print(FEATURE_NAMES[i]);
        Serial.print(": ");
        Serial.println(features[i], 1);
    }
    
    int result = predict(features);
    
    if (result >= 0) {
        Serial.print("\n>> PREDICTION: ");
        Serial.println(CLASS_LABELS[result]);
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n============================================");
    Serial.println("PLANT HEALTH - Chirale TFLite Direct");
    Serial.println("============================================");
    
    Serial.print("\nFree heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // 1. Load model
    Serial.println("\nLoading model...");
    tflModel = tflite::GetModel(MODEL_DATA);
    
    if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
        Serial.print("Model schema mismatch! Got ");
        Serial.print(tflModel->version());
        Serial.print(", expected ");
        Serial.println(TFLITE_SCHEMA_VERSION);
        while (1) delay(1000);
    }
    Serial.println("Model schema OK.");
    
    // 2. Create op resolver
    static tflite::AllOpsResolver resolver;
    
    // 3. Build interpreter
    Serial.println("Building interpreter...");
    static tflite::MicroInterpreter staticInterpreter(
        tflModel, resolver, tensorArena, kTensorArenaSize);
    interpreter = &staticInterpreter;
    
    // 4. Allocate tensors
    Serial.println("Allocating tensors...");
    TfLiteStatus allocateStatus = interpreter->AllocateTensors();
    if (allocateStatus != kTfLiteOk) {
        Serial.println("AllocateTensors() failed!");
        while (1) delay(1000);
    }
    
    // 5. Get tensor pointers
    inputTensor = interpreter->input(0);
    outputTensor = interpreter->output(0);
    
    // 6. Print tensor info
    Serial.println("\n>>> TENSOR INFO <<<");
    Serial.print("Input:  shape=[");
    for (int i = 0; i < inputTensor->dims->size; i++) {
        Serial.print(inputTensor->dims->data[i]);
        if (i < inputTensor->dims->size - 1) Serial.print(",");
    }
    Serial.print("], type=");
    Serial.println(inputTensor->type);
    
    Serial.print("Output: shape=[");
    for (int i = 0; i < outputTensor->dims->size; i++) {
        Serial.print(outputTensor->dims->data[i]);
        if (i < outputTensor->dims->size - 1) Serial.print(",");
    }
    Serial.print("], type=");
    Serial.println(outputTensor->type);
    
    int outputSize = outputTensor->dims->data[outputTensor->dims->size - 1];
    Serial.print("Output size: ");
    Serial.print(outputSize);
    Serial.print(" (expected ");
    Serial.print(NUM_CLASSES);
    Serial.println(")");
    
    if (outputSize != NUM_CLASSES) {
        Serial.println("WARNING: Output size mismatch!");
    }
    
    Serial.print("\nFree heap after init: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("\nModel ready!");
    
    // Print classes
    Serial.println("\nClasses:");
    for (int i = 0; i < NUM_CLASSES; i++) {
        Serial.print("  ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(CLASS_LABELS[i]);
    }
    
    // Run tests
    runTest("Healthy Plant", testHealthy);
    runTest("Low Nitrogen", testLowN);
    runTest("Low Phosphorus", testLowP);
    runTest("Low Potassium", testLowK);
    runTest("Water Stress", testWaterStress);
    runTest("Acidic Soil (pH 4.5)", testAcidic);
    runTest("Alkaline Soil (pH 8.5)", testAlkaline);
    
    Serial.println("\n============================================");
    Serial.println("ALL TESTS COMPLETE");
    Serial.println("============================================");
    Serial.println("\nEnter custom values: N,P,K,pH,moisture,temp");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    if (Serial.available()) {
        String str = Serial.readStringUntil('\n');
        str.trim();
        
        if (str.length() > 0) {
            float features[NUM_FEATURES];
            int idx = 0, start = 0;
            
            for (unsigned int i = 0; i <= str.length() && idx < NUM_FEATURES; i++) {
                if (i == str.length() || str[i] == ',') {
                    features[idx++] = str.substring(start, i).toFloat();
                    start = i + 1;
                }
            }
            
            if (idx == NUM_FEATURES) {
                runTest("Custom Input", features);
            } else {
                Serial.println("Error: Enter 6 values (N,P,K,pH,moisture,temp)");
            }
        }
    }
}