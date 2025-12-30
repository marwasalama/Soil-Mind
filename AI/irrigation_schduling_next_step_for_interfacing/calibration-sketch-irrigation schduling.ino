/*
 * ═══════════════════════════════════════════════════════════════════════════
 *  🔧 SENSOR CALIBRATION SKETCH
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * RUN THIS FIRST before using the main irrigation code!
 * 
 * This sketch helps you find the calibration values for YOUR specific
 * soil moisture sensor. Every sensor is slightly different.
 * 
 * Instructions:
 * 1. Upload this sketch to ESP32
 * 2. Open Serial Monitor at 115200 baud
 * 3. Hold sensor in AIR → Record the "Dry" value
 * 4. Put sensor in WATER (not past the line!) → Record the "Wet" value
 * 5. Update MOISTURE_AIR_VALUE and MOISTURE_WATER_VALUE in main code
 * 
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <DHT.h>

// ═══════════════════════════════════════════════════════════════════════════
// PIN DEFINITIONS - Match these to your wiring!
// ═══════════════════════════════════════════════════════════════════════════
#define SOIL_MOISTURE_PIN   34    // Analog pin for soil moisture
#define DHT_PIN             4     // Digital pin for DHT22
#define LED_PIN             2     // Built-in LED

DHT dht(DHT_PIN, DHT22);

// Variables to track min/max values seen
int minValue = 4095;
int maxValue = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    dht.begin();
    
    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════════════════════════╗");
    Serial.println("║          🔧 SENSOR CALIBRATION TOOL                       ║");
    Serial.println("╚═══════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("INSTRUCTIONS:");
    Serial.println("─────────────────────────────────────────────────────────────");
    Serial.println("1. Watch the readings below");
    Serial.println("2. Hold soil sensor in AIR → Note the HIGH value (DRY)");
    Serial.println("3. Put sensor in WATER → Note the LOW value (WET)");
    Serial.println("4. Copy these values to your main irrigation code:");
    Serial.println();
    Serial.println("   #define MOISTURE_AIR_VALUE    <your dry value>");
    Serial.println("   #define MOISTURE_WATER_VALUE  <your wet value>");
    Serial.println("─────────────────────────────────────────────────────────────");
    Serial.println();
    Serial.println("Starting readings in 3 seconds...\n");
    delay(3000);
}

void loop() {
    // Blink LED to show we're running
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    
    // Read soil moisture (average of 10 samples)
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(SOIL_MOISTURE_PIN);
        delay(10);
    }
    int moistureRaw = sum / 10;
    
    // Track min/max
    if (moistureRaw < minValue) minValue = moistureRaw;
    if (moistureRaw > maxValue) maxValue = moistureRaw;
    
    // Read temperature
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // Print readings
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println();
    
    // Soil Moisture
    Serial.printf("  💧 SOIL MOISTURE:  %d", moistureRaw);
    
    // Visual indicator
    if (moistureRaw > 2500) {
        Serial.print("  ◄── DRY (sensor in air?)");
    } else if (moistureRaw < 1500) {
        Serial.print("  ◄── WET (sensor in water?)");
    } else {
        Serial.print("  ◄── Medium");
    }
    Serial.println();
    
    // Temperature
    if (!isnan(temperature)) {
        Serial.printf("  🌡️  TEMPERATURE:   %.1f°C", temperature);
        if (temperature < 20) Serial.println("  (Cool)");
        else if (temperature < 30) Serial.println("  (Moderate)");
        else Serial.println("  (Hot)");
        
        Serial.printf("  💨 HUMIDITY:      %.1f%%\n", humidity);
    } else {
        Serial.println("  🌡️  TEMPERATURE:   Error reading DHT22!");
        Serial.println("      Check wiring: VCC→3.3V, GND→GND, DATA→GPIO4");
    }
    
    // Show min/max seen
    Serial.println();
    Serial.printf("  📊 Session Min: %d  |  Max: %d\n", minValue, maxValue);
    Serial.println();
    
    // Show what to copy
    Serial.println("  ┌─────────────────────────────────────────────────────┐");
    Serial.println("  │ 📋 COPY THESE TO YOUR CODE (when calibration done): │");
    Serial.println("  │                                                     │");
    Serial.printf("  │   #define MOISTURE_AIR_VALUE    %d               │\n", maxValue);
    Serial.printf("  │   #define MOISTURE_WATER_VALUE  %d               │\n", minValue);
    Serial.println("  │                                                     │");
    Serial.println("  └─────────────────────────────────────────────────────┘");
    
    Serial.println();
    
    delay(1000);  // Update every second
}
