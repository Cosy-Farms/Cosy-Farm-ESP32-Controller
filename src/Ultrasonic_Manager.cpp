#include <Arduino.h>
#include <algorithm>
#include <vector>

// Configuration constants
#define TANK_HEIGHT_CM 30.0
#define SENSOR_DEAD_ZONE_CM 20.0
#define TRIG_PIN 12  // Example pins, adjust to your wiring
#define ECHO_PIN 13

/**
 * Performs a raw distance measurement.
 */
float getRawDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    if (duration == 0) return -1; // Timeout or no echo

    // Calculate distance in cm (Speed of sound is ~343m/s)
    return (duration * 0.0343) / 2.0;
}

/**
 * Uses a Median Filter to get a stable distance reading.
 * This helps ignore "random" spikes caused by the blind zone or reflections.
 */
float getFilteredDistance(int samples = 5) {
    std::vector<float> readings;
    for (int i = 0; i < samples; i++) {
        float d = getRawDistance();
        if (d > 0) readings.push_back(d);
        delay(20); 
    }

    if (readings.empty()) return -1;

    // Sort to find the median
    std::sort(readings.begin(), readings.end());
    return readings[readings.size() / 2];
}

/**
 * Calculates tank percentage with dead-zone handling.
 */
int calculateTankPercentage() {
    float distance = getFilteredDistance(7);

    // Error handling
    if (distance < 0) return -1; 

    // Logic: If distance is less than the dead zone, the tank is likely full.
    // Waterproof sensors are unreliable below 20cm.
    if (distance < SENSOR_DEAD_ZONE_CM) {
        Serial.println("Warning: Water in Blind Zone. Assuming > 85% full.");
        return 100; 
    }

    // Calculate water depth
    float waterDepth = TANK_HEIGHT_CM - distance;
    
    // Constrain depth to physical limits
    if (waterDepth < 0) waterDepth = 0;
    if (waterDepth > TANK_HEIGHT_CM) waterDepth = TANK_HEIGHT_CM;

    int percentage = (int)((waterDepth / TANK_HEIGHT_CM) * 100.0);
    
    Serial.printf("Distance: %.2f cm | Water Depth: %.2f cm | Fill: %d%%\n", 
                  distance, waterDepth, percentage);
                  
    return percentage;
}