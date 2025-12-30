/*
 * ═══════════════════════════════════════════════════════════════════════════
 *  🌱 SMART IRRIGATION SYSTEM - PRODUCTION CODE WITH REAL SENSORS
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Hardware Required:
 *   - ESP32 DevKit V1
 *   - Capacitive Soil Moisture Sensor v1.2/v2.0
 *   - DHT22 Temperature & Humidity Sensor
 *   - 5V Relay Module
 *   - 12V Water Pump (optional, for real deployment)
 * 
 * Libraries Required:
 *   - ArduTFLite (for TensorFlow Lite inference)
 *   - DHT sensor library by Adafruit
 * 
 * Author: SoilMind Project
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <ArduTFLite.h>
#include <DHT.h>

// Include the trained model (generated from Colab notebook)
#include "irrigation_model.h"

// ═══════════════════════════════════════════════════════════════════════════
// PIN DEFINITIONS - Adjust these for your wiring
// ═══════════════════════════════════════════════════════════════════════════
#define SOIL_MOISTURE_PIN   34    // ADC pin for soil moisture (GPIO 34-39)
#define DHT_PIN             4     // Digital pin for DHT22
#define RELAY_PIN           26    // Digital pin for relay control
#define LED_PIN             2     // Built-in LED for status

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR CALIBRATION VALUES - ⚠️ YOU MUST CALIBRATE THESE! ⚠️
// ═══════════════════════════════════════════════════════════════════════════
// Step 1: Hold sensor in AIR and record the value → MOISTURE_AIR_VALUE
// Step 2: Put sensor in WATER and record the value → MOISTURE_WATER_VALUE
// Step 3: Replace these values with your readings

#define MOISTURE_AIR_VALUE    3100    // Reading when sensor is in AIR (dry)
#define MOISTURE_WATER_VALUE  1400    // Reading when sensor is in WATER (wet)

// Mapping to training data scale (your model was trained on ~100-400 range)
#define TRAINING_SCALE_MIN    100.0   // Minimum moisture in training data
#define TRAINING_SCALE_MAX    400.0   // Maximum moisture in training data

// ═══════════════════════════════════════════════════════════════════════════
// TIMING CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════
#define READING_INTERVAL_MS   60000   // Read sensors every 60 seconds (1 minute)
                                       // Adjust based on your training data interval

#define MIN_PUMP_ON_TIME      30000   // Minimum pump ON time (30 seconds)
#define MIN_PUMP_OFF_TIME     60000   // Minimum time between pump runs (60 seconds)
#define MAX_PUMP_RUN_TIME     300000  // Maximum continuous pump run (5 minutes)

// ═══════════════════════════════════════════════════════════════════════════
// MODEL CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════
#define NUM_FEATURES          8
#define HISTORY_SIZE          4       // Must match window_size from training
#define FORECAST_HORIZON      2       // Model predicts 2 steps ahead

// Decision thresholds with hysteresis
#define THRESHOLD_IRRIGATE    0.70    // Start irrigation above 70% probability
#define THRESHOLD_STOP        0.30    // Stop irrigation below 30% probability

// TFLite tensor arena
constexpr int kTensorArenaSize = 8 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

// ═══════════════════════════════════════════════════════════════════════════
// FEATURE NORMALIZATION PARAMETERS
// These values come from your training - check scaler_params.csv
// ═══════════════════════════════════════════════════════════════════════════
// Feature order: [temperature, soilmoisture, temp_mean, moisture_mean,
//                 temp_trend, moisture_trend, moisture_lag_1, moisture_lag_2]

const float featureMeans[NUM_FEATURES] = {
    29.599089,    // temperature
    243.692406,   // soilmoisture
    29.599089,    // temperature_mean
    243.692406,   // soilmoisture_mean
    0.0,          // temperature_trend
    0.0,          // soilmoisture_trend
    243.692406,   // soilmoisture_lag_1
    243.692406    // soilmoisture_lag_2
};

const float featureStds[NUM_FEATURES] = {
    5.842685,     // temperature
    76.176855,    // soilmoisture
    5.842685,     // temperature_mean
    76.176855,    // soilmoisture_mean
    5.0,          // temperature_trend (approximate)
    30.0,         // soilmoisture_trend (approximate)
    76.176855,    // soilmoisture_lag_1
    76.176855     // soilmoisture_lag_2
};

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL OBJECTS
// ═══════════════════════════════════════════════════════════════════════════
DHT dht(DHT_PIN, DHT22);

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR HISTORY BUFFER
// Stores the last N readings for trend and average calculations
// ═══════════════════════════════════════════════════════════════════════════
struct SensorHistory {
    float temperature[HISTORY_SIZE];
    float moisture[HISTORY_SIZE];
    int writeIndex;        // Where to write next
    int count;             // How many readings we have (0 to HISTORY_SIZE)
    
    void init() {
        writeIndex = 0;
        count = 0;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            temperature[i] = 0;
            moisture[i] = 0;
        }
    }
    
    void addReading(float temp, float moist) {
        temperature[writeIndex] = temp;
        moisture[writeIndex] = moist;
        writeIndex = (writeIndex + 1) % HISTORY_SIZE;
        if (count < HISTORY_SIZE) count++;
    }
    
    // Get value from N steps ago (0 = most recent, 1 = one before, etc.)
    float getTemp(int stepsAgo) {
        if (stepsAgo >= count) return temperature[0];  // Safety fallback
        int idx = (writeIndex - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return temperature[idx];
    }
    
    float getMoisture(int stepsAgo) {
        if (stepsAgo >= count) return moisture[0];  // Safety fallback
        int idx = (writeIndex - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return moisture[idx];
    }
    
    // Calculate rolling average
    float getTempMean() {
        if (count == 0) return 0;
        float sum = 0;
        for (int i = 0; i < count; i++) sum += temperature[i];
        return sum / count;
    }
    
    float getMoistureMean() {
        if (count == 0) return 0;
        float sum = 0;
        for (int i = 0; i < count; i++) sum += moisture[i];
        return sum / count;
    }
    
    // Calculate trend (current value minus oldest value)
    float getTempTrend() {
        if (count < 2) return 0;
        return getTemp(0) - getTemp(count - 1);
    }
    
    float getMoistureTrend() {
        if (count < 2) return 0;
        return getMoisture(0) - getMoisture(count - 1);
    }
    
    bool isReady() {
        return count >= HISTORY_SIZE;
    }
};

SensorHistory history;

// ═══════════════════════════════════════════════════════════════════════════
// PUMP CONTROL STATE
// ═══════════════════════════════════════════════════════════════════════════
struct PumpController {
    bool isRunning;
    unsigned long lastOnTime;
    unsigned long lastOffTime;
    unsigned long runStartTime;
    
    void init() {
        isRunning = false;
        lastOnTime = 0;
        lastOffTime = 0;
        runStartTime = 0;
    }
    
    bool canTurnOn(unsigned long now) {
        // Check minimum off time
        if (now - lastOffTime < MIN_PUMP_OFF_TIME) return false;
        return true;
    }
    
    bool shouldForceOff(unsigned long now) {
        // Check maximum run time
        if (isRunning && (now - runStartTime > MAX_PUMP_RUN_TIME)) return true;
        return false;
    }
    
    void turnOn(unsigned long now) {
        if (!isRunning) {
            isRunning = true;
            runStartTime = now;
            lastOnTime = now;
            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(LED_PIN, HIGH);
            Serial.println("💧 PUMP ON");
        }
    }
    
    void turnOff(unsigned long now) {
        // Check minimum on time
        if (isRunning && (now - runStartTime >= MIN_PUMP_ON_TIME)) {
            isRunning = false;
            lastOffTime = now;
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            Serial.println("🛑 PUMP OFF");
        }
    }
};

PumpController pump;

// ═══════════════════════════════════════════════════════════════════════════
// MODEL STATE
// ═══════════════════════════════════════════════════════════════════════════
bool modelReady = false;
unsigned long lastReadingTime = 0;

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR READING FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

float readTemperature() {
    float temp = dht.readTemperature();
    
    // Check for read error
    if (isnan(temp)) {
        Serial.println("⚠️ DHT22 read error!");
        return -999;  // Error indicator
    }
    
    return temp;
}

float readSoilMoistureRaw() {
    // Take multiple readings and average for stability
    long sum = 0;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += analogRead(SOIL_MOISTURE_PIN);
        delay(10);
    }
    
    return sum / samples;
}

float readSoilMoisturePercent() {
    float raw = readSoilMoistureRaw();
    
    // Map to percentage (0% = dry, 100% = wet)
    float percent = (float)(MOISTURE_AIR_VALUE - raw) / 
                    (float)(MOISTURE_AIR_VALUE - MOISTURE_WATER_VALUE) * 100.0;
    
    return constrain(percent, 0, 100);
}

float readSoilMoistureScaled() {
    // Get percentage and map to training data scale
    float percent = readSoilMoisturePercent();
    
    // Map 0-100% to TRAINING_SCALE_MIN - TRAINING_SCALE_MAX
    float scaled = TRAINING_SCALE_MIN + 
                   (percent / 100.0) * (TRAINING_SCALE_MAX - TRAINING_SCALE_MIN);
    
    return scaled;
}

// ═══════════════════════════════════════════════════════════════════════════
// SENSOR VALIDATION
// ═══════════════════════════════════════════════════════════════════════════

bool validateSensorReadings(float temp, float moisture) {
    // Temperature sanity check
    if (temp < -10 || temp > 60) {
        Serial.printf("⚠️ Invalid temperature: %.1f\n", temp);
        return false;
    }
    
    // Moisture sanity check (in training scale)
    if (moisture < 50 || moisture > 500) {
        Serial.printf("⚠️ Invalid moisture: %.1f\n", moisture);
        return false;
    }
    
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// FEATURE EXTRACTION
// This is the critical function that calculates the 8 features for the model
// ═══════════════════════════════════════════════════════════════════════════

void extractFeatures(float features[NUM_FEATURES]) {
    // Feature [0]: Current temperature
    features[0] = history.getTemp(0);
    
    // Feature [1]: Current soil moisture
    features[1] = history.getMoisture(0);
    
    // Feature [2]: Temperature rolling mean (last 4 readings)
    features[2] = history.getTempMean();
    
    // Feature [3]: Soil moisture rolling mean (last 4 readings)
    features[3] = history.getMoistureMean();
    
    // Feature [4]: Temperature trend (current - oldest)
    features[4] = history.getTempTrend();
    
    // Feature [5]: Soil moisture trend (current - oldest) ⭐ KEY FEATURE!
    features[5] = history.getMoistureTrend();
    
    // Feature [6]: Soil moisture lag 1 (previous reading)
    features[6] = history.getMoisture(1);
    
    // Feature [7]: Soil moisture lag 2 (2 readings ago)
    features[7] = history.getMoisture(2);
}

// ═══════════════════════════════════════════════════════════════════════════
// NORMALIZATION
// Applies the same scaling used during training
// ═══════════════════════════════════════════════════════════════════════════

float normalizeFeature(float value, int index) {
    return (value - featureMeans[index]) / featureStds[index];
}

// ═══════════════════════════════════════════════════════════════════════════
// MODEL INFERENCE
// ═══════════════════════════════════════════════════════════════════════════

float runInference() {
    if (!modelReady) {
        Serial.println("⚠️ Model not ready!");
        return -1;
    }
    
    if (!history.isReady()) {
        Serial.println("⏳ Waiting for more readings...");
        return -1;
    }
    
    // Extract features from sensor history
    float features[NUM_FEATURES];
    extractFeatures(features);
    
    // Normalize features and set model input
    for (int i = 0; i < NUM_FEATURES; i++) {
        float normalized = normalizeFeature(features[i], i);
        modelSetInput(normalized, i);
    }
    
    // Run inference
    unsigned long startTime = micros();
    
    if (!modelRunInference()) {
        Serial.println("❌ Inference failed!");
        return -1;
    }
    
    unsigned long inferenceTime = micros() - startTime;
    
    // Get output probability
    float probability = modelGetOutput(0);
    
    // Print debug info
    Serial.println("\n📊 ═══════════ INFERENCE RESULTS ═══════════");
    Serial.printf("   Temperature:     %.1f°C (trend: %+.1f)\n", 
                  features[0], features[4]);
    Serial.printf("   Soil Moisture:   %.0f (trend: %+.0f)\n", 
                  features[1], features[5]);
    Serial.printf("   Moisture Mean:   %.0f\n", features[3]);
    Serial.printf("   Recent History:  [%.0f, %.0f]\n", features[6], features[7]);
    Serial.println("   ────────────────────────────────────────");
    Serial.printf("   🎯 Irrigation Probability: %.1f%%\n", probability * 100);
    Serial.printf("   ⏱️  Inference Time: %lu µs\n", inferenceTime);
    Serial.println("═══════════════════════════════════════════\n");
    
    return probability;
}

// ═══════════════════════════════════════════════════════════════════════════
// FALLBACK DECISION (if model fails)
// Temperature-adaptive thresholds from training data analysis
// ═══════════════════════════════════════════════════════════════════════════

bool fallbackDecision(float temp, float moisture) {
    if (temp < 28) return moisture < 256;       // Cool
    else if (temp < 30) return moisture < 237;  // Moderate  
    else if (temp < 32) return moisture < 229;  // Warm
    else return moisture < 240;                  // Hot
}

// ═══════════════════════════════════════════════════════════════════════════
// IRRIGATION DECISION
// ═══════════════════════════════════════════════════════════════════════════

void makeIrrigationDecision(float probability, unsigned long now) {
    // Check for forced off (max run time exceeded)
    if (pump.shouldForceOff(now)) {
        Serial.println("⚠️ Max run time exceeded - forcing pump OFF");
        pump.turnOff(now);
        return;
    }
    
    // Apply hysteresis thresholds
    if (!pump.isRunning && probability > THRESHOLD_IRRIGATE) {
        // Currently OFF, probability HIGH → Turn ON
        if (pump.canTurnOn(now)) {
            Serial.println("📈 High probability detected - starting irrigation");
            pump.turnOn(now);
        } else {
            Serial.println("⏳ Waiting for minimum off time...");
        }
    } 
    else if (pump.isRunning && probability < THRESHOLD_STOP) {
        // Currently ON, probability LOW → Turn OFF
        Serial.println("📉 Low probability - stopping irrigation");
        pump.turnOff(now);
    }
    // If probability is between thresholds, maintain current state (hysteresis)
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════════════════════════╗");
    Serial.println("║       🌱 SMART IRRIGATION SYSTEM - STARTING UP            ║");
    Serial.println("╚═══════════════════════════════════════════════════════════╝");
    
    // Initialize GPIO
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    Serial.println("✓ GPIO initialized");
    
    // Initialize DHT22
    dht.begin();
    Serial.println("✓ DHT22 initialized");
    
    // Initialize history buffer
    history.init();
    Serial.println("✓ History buffer initialized");
    
    // Initialize pump controller
    pump.init();
    Serial.println("✓ Pump controller initialized");
    
    // Initialize TFLite model
    Serial.println("\n📦 Loading TFLite model...");
    
    if (irrigation_model_len > 0) {
        modelReady = modelInit(irrigation_model, tensorArena, kTensorArenaSize);
        
        if (modelReady) {
            Serial.println("✓ Model loaded successfully!");
            Serial.printf("  • Model size: %u bytes\n", irrigation_model_len);
            Serial.printf("  • Features: %d\n", NUM_FEATURES);
        } else {
            Serial.println("❌ Model initialization failed!");
            Serial.println("  → Running in FALLBACK mode (threshold rules)");
        }
    } else {
        Serial.println("⚠️ No model data found!");
        Serial.println("  → Running in FALLBACK mode (threshold rules)");
        modelReady = false;
    }
    
    // Sensor calibration reminder
    Serial.println("\n⚠️  CALIBRATION REMINDER:");
    Serial.printf("  • Air (dry) value: %d\n", MOISTURE_AIR_VALUE);
    Serial.printf("  • Water (wet) value: %d\n", MOISTURE_WATER_VALUE);
    Serial.println("  • Make sure these match YOUR sensor!\n");
    
    // System ready
    Serial.println("╔═══════════════════════════════════════════════════════════╗");
    Serial.printf("║  ✅ SYSTEM READY - Reading every %d seconds              ║\n", 
                  READING_INTERVAL_MS / 1000);
    Serial.println("╚═══════════════════════════════════════════════════════════╝\n");
    
    // Take first reading immediately
    lastReadingTime = millis() - READING_INTERVAL_MS;
}

// ═══════════════════════════════════════════════════════════════════════════
// MAIN LOOP
// ═══════════════════════════════════════════════════════════════════════════

void loop() {
    unsigned long now = millis();
    
    // Time for a new reading?
    if (now - lastReadingTime >= READING_INTERVAL_MS) {
        lastReadingTime = now;
        
        Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.printf("📡 Reading #%d at %lu ms\n", history.count + 1, now);
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // Read sensors
        float temperature = readTemperature();
        float moistureRaw = readSoilMoistureRaw();
        float moisturePercent = readSoilMoisturePercent();
        float moistureScaled = readSoilMoistureScaled();
        
        // Print raw readings
        Serial.println("\n📋 Sensor Readings:");
        Serial.printf("   🌡️  Temperature:    %.1f°C\n", temperature);
        Serial.printf("   💧 Moisture (raw):  %.0f\n", moistureRaw);
        Serial.printf("   💧 Moisture (%%):    %.1f%%\n", moisturePercent);
        Serial.printf("   💧 Moisture (scaled): %.0f (training scale)\n", moistureScaled);
        
        // Validate readings
        if (!validateSensorReadings(temperature, moistureScaled)) {
            Serial.println("⚠️ Invalid readings - skipping this cycle");
            return;
        }
        
        // Add to history buffer (use scaled moisture for model)
        history.addReading(temperature, moistureScaled);
        Serial.printf("\n📦 History buffer: %d/%d readings\n", 
                      history.count, HISTORY_SIZE);
        
        // Make prediction if history is ready
        if (history.isReady()) {
            float probability;
            
            if (modelReady) {
                // Use ML model
                probability = runInference();
                
                if (probability < 0) {
                    // Model failed, use fallback
                    Serial.println("⚠️ Using fallback rules...");
                    bool shouldIrrigate = fallbackDecision(temperature, moistureScaled);
                    probability = shouldIrrigate ? 0.9 : 0.1;
                }
            } else {
                // No model, use fallback
                Serial.println("📋 Using fallback rules (no model)...");
                bool shouldIrrigate = fallbackDecision(temperature, moistureScaled);
                probability = shouldIrrigate ? 0.9 : 0.1;
                Serial.printf("   Decision: %s\n", shouldIrrigate ? "IRRIGATE" : "NO IRRIGATION");
            }
            
            // Make irrigation decision
            makeIrrigationDecision(probability, now);
            
            // Print current pump status
            Serial.printf("\n🚰 Pump Status: %s\n", pump.isRunning ? "RUNNING 💧" : "OFF");
            
        } else {
            Serial.printf("\n⏳ Collecting data: need %d more readings before prediction\n",
                          HISTORY_SIZE - history.count);
        }
    }
    
    // Safety check: force pump off if running too long
    if (pump.shouldForceOff(now)) {
        pump.turnOff(now);
    }
    
    // Small delay to prevent watchdog issues
    delay(100);
}

// ═══════════════════════════════════════════════════════════════════════════
// CALIBRATION HELPER FUNCTION
// Uncomment and call from setup() to calibrate your sensor
// ═══════════════════════════════════════════════════════════════════════════
/*
void runCalibration() {
    Serial.println("\n\n🔧 CALIBRATION MODE");
    Serial.println("═══════════════════════════════════════════");
    Serial.println("Watch the values below and update your code:\n");
    
    while (true) {
        int raw = 0;
        for (int i = 0; i < 10; i++) {
            raw += analogRead(SOIL_MOISTURE_PIN);
            delay(100);
        }
        raw /= 10;
        
        Serial.printf("Raw Value: %d   ", raw);
        Serial.println("| Hold in AIR for DRY value, in WATER for WET value");
        
        delay(500);
    }
}
*/
