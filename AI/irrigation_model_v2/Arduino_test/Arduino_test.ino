/*
 * Smart Irrigation Controller - SCENARIO SIMULATION MODE
 * 
 * This version simulates different real-world scenarios to demonstrate
 * why trend-based forecasting (Approach 2) is superior to simple thresholds.
 * 
 * Scenarios:
 *   1. Hot Day - Rapid moisture loss due to high temperature
 *   2. Normal Day - Stable conditions, gradual changes
 *   3. Recovery - Soil recovering after rain/irrigation
 *   4. Morning Dew - Moisture rising naturally in early morning
 *   5. Sudden Heatwave - Temperature spike causing rapid drying
 *   6. Night Cooling - Temperature drops, moisture stabilizes
 */

#include <ArduTFLite.h>

// Uncomment if you have the model file:
// #include "irrigation_model.h"

#ifndef IRRIGATION_MODEL_H
const unsigned char irrigation_model[] PROGMEM = {0};
const unsigned int irrigation_model_len = 0;
#endif

// =============================================================================
// CONFIGURATION
// =============================================================================
#define LED_PIN             2
#define RELAY_PIN           26

constexpr int NUM_FEATURES = 8;
constexpr int kTensorArenaSize = 8 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

bool modelReady = false;

// Scaler parameters (from training)
const float featureMeans[NUM_FEATURES] = {
    29.599089f, 243.692406f, 29.599089f, 243.692406f,
    0.0f, 0.0f, 243.692406f, 243.692406f
};
const float featureStds[NUM_FEATURES] = {
    5.842685f, 76.176855f, 5.842685f, 76.176855f,
    1.0f, 10.0f, 76.176855f, 76.176855f
};

// =============================================================================
// SCENARIO DEFINITIONS
// Each scenario tells a story through sensor data
// =============================================================================
enum Scenario {
    SCENARIO_HOT_DAY,           // High temp, rapid moisture loss
    SCENARIO_NORMAL_DAY,        // Stable, gradual changes
    SCENARIO_RECOVERY,          // After rain/irrigation, moisture rising
    SCENARIO_MORNING_DEW,       // Natural morning moisture increase
    SCENARIO_SUDDEN_HEATWAVE,   // Temperature spike, accelerated drying
    SCENARIO_NIGHT_COOLING,     // Cool night, moisture stabilizes
    SCENARIO_CRITICAL_DRY,      // Already very dry, urgent need
    NUM_SCENARIOS
};

// Scenario names for display
const char* scenarioNames[] = {
    "HOT DAY - Rapid Drying",
    "NORMAL DAY - Stable",
    "RECOVERY - After Rain",
    "MORNING DEW - Rising Moisture",
    "SUDDEN HEATWAVE - Temp Spike",
    "NIGHT COOLING - Stabilizing",
    "CRITICAL DRY - Urgent Need"
};

// Each scenario has a sequence of readings that tell a story
// Format: {temp, moisture} for each time step
// We'll use 8 readings per scenario to fill history and show prediction

struct ScenarioData {
    float temp[8];
    float moisture[8];
    const char* description;
    const char* expectedBehavior;
};

// Define the scenarios with realistic data progressions
ScenarioData scenarios[] = {
    // SCENARIO_HOT_DAY: Temperature high, moisture dropping steadily
    {
        {32.0, 32.5, 33.0, 33.5, 34.0, 34.2, 34.5, 34.8},  // Rising temp
        {320,  300,  275,  250,  225,  200,  175,  150},    // Rapid moisture loss
        "Simulating a hot afternoon. Temperature rising above 32°C\n"
        "causing rapid evaporation. Moisture dropping ~25 units/reading.",
        "SHOULD IRRIGATE: Trend shows rapid drying, will hit critical soon"
    },
    
    // SCENARIO_NORMAL_DAY: Moderate conditions, slow changes
    {
        {28.5, 28.8, 29.0, 29.2, 29.0, 28.8, 28.5, 28.3},  // Stable temp
        {280,  278,  275,  273,  270,  268,  265,  263},    // Slow moisture loss
        "Simulating a normal day. Temperature stable around 29°C.\n"
        "Moisture decreasing slowly at ~2-3 units/reading.",
        "NO IRRIGATION: Trend is gradual, no urgency"
    },
    
    // SCENARIO_RECOVERY: After irrigation/rain, moisture rising
    {
        {29.0, 28.5, 28.0, 27.5, 27.8, 28.0, 28.2, 28.5},  // Cooling slightly
        {180,  210,  240,  270,  285,  295,  305,  310},    // Moisture recovering
        "Simulating recovery after rain or irrigation.\n"
        "Moisture rising steadily, soil rehydrating.",
        "NO IRRIGATION: Trend is POSITIVE, soil is recovering!"
    },
    
    // SCENARIO_MORNING_DEW: Natural moisture increase in early morning
    {
        {24.0, 23.5, 23.0, 22.8, 23.0, 23.5, 24.0, 24.5},  // Cool morning
        {250,  258,  265,  272,  275,  273,  270,  268},    // Dew effect
        "Simulating early morning. Cool temperatures cause\n"
        "condensation, moisture temporarily rises then stabilizes.",
        "NO IRRIGATION: Natural moisture gain, wait and monitor"
    },
    
    // SCENARIO_SUDDEN_HEATWAVE: Rapid temperature spike
    {
        {28.0, 29.5, 31.0, 33.0, 35.0, 36.5, 37.0, 37.5},  // Rapid heating
        {300,  290,  270,  245,  215,  185,  155,  130},    // Accelerating loss
        "Simulating sudden heatwave! Temperature spiking rapidly.\n"
        "Moisture loss ACCELERATING as temp rises.",
        "URGENT IRRIGATION: Dangerous trend, act immediately!"
    },
    
    // SCENARIO_NIGHT_COOLING: Evening cooling, moisture stabilizes
    {
        {32.0, 30.5, 29.0, 27.5, 26.5, 25.5, 25.0, 24.5},  // Cooling down
        {220,  222,  225,  228,  230,  232,  233,  234},    // Slight recovery
        "Simulating evening/night. Temperature dropping.\n"
        "Evaporation slowing, moisture stabilizing.",
        "NO IRRIGATION: Conditions improving naturally"
    },
    
    // SCENARIO_CRITICAL_DRY: Already in critical state
    {
        {31.0, 31.5, 32.0, 32.0, 31.5, 31.0, 31.0, 31.0},  // Hot and stable
        {180,  170,  160,  155,  150,  148,  145,  142},    // Already critical
        "Simulating critical dry conditions.\n"
        "Moisture already very low and still dropping.",
        "CRITICAL IRRIGATION: Below safe threshold, immediate action!"
    }
};

// =============================================================================
// SENSOR HISTORY BUFFER
// =============================================================================
#define HISTORY_SIZE 4

struct SensorHistory {
    float temperature[HISTORY_SIZE];
    float soilmoisture[HISTORY_SIZE];
    int index;
    int count;
    
    void init() {
        index = 0;
        count = 0;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            temperature[i] = 0;
            soilmoisture[i] = 0;
        }
    }
    
    void addReading(float temp, float moisture) {
        temperature[index] = temp;
        soilmoisture[index] = moisture;
        index = (index + 1) % HISTORY_SIZE;
        if (count < HISTORY_SIZE) count++;
    }
    
    float getTemp(int stepsAgo) {
        int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return temperature[idx];
    }
    
    float getMoisture(int stepsAgo) {
        int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return soilmoisture[idx];
    }
    
    float tempMean() {
        float sum = 0;
        for (int i = 0; i < count; i++) sum += temperature[i];
        return count > 0 ? sum / count : 0;
    }
    
    float moistureMean() {
        float sum = 0;
        for (int i = 0; i < count; i++) sum += soilmoisture[i];
        return count > 0 ? sum / count : 0;
    }
    
    float tempTrend() {
        if (count < 2) return 0;
        return getTemp(0) - getTemp(count - 1);
    }
    
    float moistureTrend() {
        if (count < 2) return 0;
        return getMoisture(0) - getMoisture(count - 1);
    }
    
    bool isReady() { return count >= HISTORY_SIZE; }
};

SensorHistory history;

// =============================================================================
// SIMULATION STATE
// =============================================================================
int currentScenario = 0;
int currentStep = 0;
unsigned long lastStepTime = 0;
const unsigned long STEP_INTERVAL = 2000;  // 2 seconds per reading

bool irrigationActive = false;

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    
    // Try to initialize model (will likely fail without proper header)
    if (irrigation_model_len > 0) {
        modelReady = modelInit(irrigation_model, tensorArena, kTensorArenaSize);
    }
    
    printWelcome();
    history.init();
}

void printWelcome() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║     SMART IRRIGATION SYSTEM - SCENARIO DEMONSTRATION       ║");
    Serial.println("║                                                            ║");
    Serial.println("║  This demo shows why TREND-BASED forecasting (Approach 2)  ║");
    Serial.println("║  is superior to simple threshold-based irrigation.         ║");
    Serial.println("║                                                            ║");
    Serial.println("║  Watch how the same moisture level (e.g., 220) triggers    ║");
    Serial.println("║  DIFFERENT decisions based on whether it's rising/falling. ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("Model Status: %s\n", modelReady ? "ML READY" : "FALLBACK MODE");
    Serial.println("\nStarting scenario demonstration in 3 seconds...\n");
    delay(3000);
}

// =============================================================================
// PRINT FUNCTIONS
// =============================================================================
void printScenarioHeader(int scenario) {
    Serial.println("\n");
    Serial.println("################################################################");
    Serial.printf("##  SCENARIO %d: %s\n", scenario + 1, scenarioNames[scenario]);
    Serial.println("################################################################");
    Serial.println();
    Serial.println(scenarios[scenario].description);
    Serial.println();
    Serial.printf("Expected: %s\n", scenarios[scenario].expectedBehavior);
    Serial.println();
    Serial.println("Reading#  Temp(°C)  Moisture  Temp_Trend  Moist_Trend  Decision");
    Serial.println("--------  --------  --------  ----------  -----------  --------");
}

void printReading(int step, float temp, float moisture) {
    // Calculate trends if we have enough history
    float tempTrend = history.tempTrend();
    float moistTrend = history.moistureTrend();
    
    // Format the trend with + or - sign
    char tempTrendStr[10], moistTrendStr[10];
    sprintf(tempTrendStr, "%+.1f", tempTrend);
    sprintf(moistTrendStr, "%+.0f", moistTrend);
    
    Serial.printf("   %d       %.1f      %.0f       %s        %s       ",
                  step + 1, temp, moisture, 
                  history.count >= 2 ? tempTrendStr : " N/A",
                  history.count >= 2 ? moistTrendStr : " N/A");
}

void printAnalysis() {
    Serial.println("\n--- ANALYSIS ---");
    
    float currentMoisture = history.getMoisture(0);
    float moistureTrend = history.moistureTrend();
    float currentTemp = history.getTemp(0);
    
    // What simple threshold would say
    bool thresholdSays = false;
    int threshold;
    if (currentTemp < 28) { threshold = 256; }
    else if (currentTemp < 30) { threshold = 237; }
    else if (currentTemp < 32) { threshold = 229; }
    else { threshold = 240; }
    thresholdSays = currentMoisture < threshold;
    
    Serial.printf("Current moisture: %.0f | Threshold for %.1f°C: %d\n", 
                  currentMoisture, currentTemp, threshold);
    Serial.printf("Simple Threshold Decision: %s\n", 
                  thresholdSays ? "IRRIGATE" : "DON'T IRRIGATE");
    
    // What trend-based approach would say
    Serial.println();
    Serial.println("Trend-Based Analysis:");
    Serial.printf("  • Moisture trend: %+.0f units over last 4 readings\n", moistureTrend);
    
    if (moistureTrend < -50) {
        Serial.println("  • RAPID DRYING detected! Urgent irrigation recommended.");
    } else if (moistureTrend < -20) {
        Serial.println("  • Moderate drying trend. Consider irrigation soon.");
    } else if (moistureTrend > 20) {
        Serial.println("  • RECOVERY detected! Soil is rehydrating. No irrigation needed.");
    } else {
        Serial.println("  • Stable conditions. Monitor but no urgent action.");
    }
    
    // Demonstrate the key difference
    Serial.println();
    Serial.println("═══════════════════════════════════════════════════════════");
    
    // Case where trend matters more than current value
    if (currentMoisture > threshold && moistureTrend < -40) {
        Serial.println("★ KEY INSIGHT: Moisture is ABOVE threshold, but trend shows");
        Serial.println("  rapid decline. Smart system should prepare for irrigation!");
        Serial.println("  Simple threshold would MISS this early warning.");
    } 
    else if (currentMoisture < threshold && moistureTrend > 20) {
        Serial.println("★ KEY INSIGHT: Moisture is BELOW threshold, but RISING!");
        Serial.println("  Smart system knows irrigation is NOT needed - soil recovering.");
        Serial.println("  Simple threshold would WASTE water here.");
    }
    else if (currentMoisture < threshold && moistureTrend < -30) {
        Serial.println("★ KEY INSIGHT: Both threshold AND trend agree - IRRIGATE!");
        Serial.println("  Soil is dry AND getting drier rapidly.");
    }
    else {
        Serial.println("★ In this case, threshold and trend-based approaches agree.");
    }
    Serial.println("═══════════════════════════════════════════════════════════");
}

// =============================================================================
// DECISION FUNCTIONS
// =============================================================================
bool thresholdDecision(float temp, float moisture) {
    if (temp < 28) return moisture < 256;
    else if (temp < 30) return moisture < 237;
    else if (temp < 32) return moisture < 229;
    else return moisture < 240;
}

String trendBasedDecision(float moisture, float moistureTrend, float temp) {
    // This simulates what the ML model learned
    
    // Critical: already very dry
    if (moisture < 160) {
        return "CRITICAL - IRRIGATE NOW!";
    }
    
    // Rapid drying - proactive irrigation even if not critical yet
    if (moistureTrend < -50 && moisture < 250) {
        return "IRRIGATE SOON (rapid drying)";
    }
    
    // Recovery - don't irrigate even if below typical threshold
    if (moistureTrend > 30) {
        return "NO - Soil recovering";
    }
    
    // Hot day acceleration
    if (temp > 33 && moistureTrend < -30) {
        return "IRRIGATE (heat + drying)";
    }
    
    // Stable conditions - use threshold as guide
    if (moistureTrend > -15 && moistureTrend < 15) {
        bool belowThreshold = thresholdDecision(temp, moisture);
        return belowThreshold ? "IRRIGATE (stable, low)" : "NO - Stable, adequate";
    }
    
    // Moderate drying
    if (moistureTrend < -20 && moisture < 220) {
        return "IRRIGATE (moderate drying)";
    }
    
    return "MONITOR";
}

void controlIrrigation(bool shouldIrrigate) {
    if (shouldIrrigate && !irrigationActive) {
        irrigationActive = true;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
    } else if (!shouldIrrigate && irrigationActive) {
        irrigationActive = false;
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
    }
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
    unsigned long now = millis();
    
    if (now - lastStepTime >= STEP_INTERVAL) {
        lastStepTime = now;
        
        // Start new scenario?
        if (currentStep == 0) {
            history.init();  // Reset history for new scenario
            irrigationActive = false;
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            printScenarioHeader(currentScenario);
        }
        
        // Get current reading from scenario data
        float temp = scenarios[currentScenario].temp[currentStep];
        float moisture = scenarios[currentScenario].moisture[currentStep];
        
        // Add to history
        history.addReading(temp, moisture);
        
        // Print reading
        printReading(currentStep, temp, moisture);
        
        // Make decisions once we have enough history
        if (history.isReady()) {
            float moistTrend = history.moistureTrend();
            String decision = trendBasedDecision(moisture, moistTrend, temp);
            Serial.println(decision);
            
            // Control pump based on decision
            bool shouldIrrigate = decision.indexOf("IRRIGATE") >= 0;
            controlIrrigation(shouldIrrigate);
        } else {
            Serial.println("(collecting)");
        }
        
        // Move to next step
        currentStep++;
        
        // End of scenario?
        if (currentStep >= 8) {
            // Print analysis
            printAnalysis();
            
            // Pause between scenarios
            Serial.println("\n>>> Next scenario in 5 seconds... <<<\n");
            delay(5000);
            
            // Move to next scenario
            currentStep = 0;
            currentScenario = (currentScenario + 1) % NUM_SCENARIOS;
        }
    }
    
    delay(10);
}