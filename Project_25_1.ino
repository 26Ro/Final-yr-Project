#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// MLX90416 I2C address
#define MLX90416_ADDRESS 0x5A  // Typically 0x5A

// Define the analog pin for the voltage sensor
#define VOLTAGE_SENSOR_PIN 34   // Use a suitable GPIO pin

// Define the pin for LED
#define LED_PIN_WARNING 2     // GPIO pin for warning LED
#define LED_PIN_CRITICAL 4    // GPIO pin for critical LED

// WiFi credentials
const char* ssid = "V2040";           // Your WiFi SSID
const char* password = "16410325";    // Your WiFi password

// Google Apps Script Web App URL
const char* url = "https://script.google.com/macros/s/AKfycbwDTMKwzddSA55AUPNuvz0O51sSfivAJDXDcPhDjVqshO26R-AhRvXUTVLpSDDbXLe3/exec"; // Replace with your Google Apps Script URL

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Set LED pins as output
    pinMode(LED_PIN_WARNING, OUTPUT);
    pinMode(LED_PIN_CRITICAL, OUTPUT);
    digitalWrite(LED_PIN_WARNING, LOW);  // Initially turn off LEDs
    digitalWrite(LED_PIN_CRITICAL, LOW);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

void loop() {
    // Read ambient and core temperature from MLX90416
    float ambientTemperature = readAmbientTemperature();
    float coreTemperature = readCoreTemperature();

    // Read voltage from the voltage sensor
    float voltage = readVoltage();

    // Determine output value based on core temperature
    float output = 0;
    if (coreTemperature > 45) {
        output = 1;  // Critical level
        digitalWrite(LED_PIN_CRITICAL, HIGH);  // Turn on critical LED
        digitalWrite(LED_PIN_WARNING, LOW);    // Ensure warning LED is off
    } else if (coreTemperature > 35) {
        output = 0.5;  // Warning level
        digitalWrite(LED_PIN_WARNING, HIGH);   // Turn on warning LED
        digitalWrite(LED_PIN_CRITICAL, LOW);   // Ensure critical LED is off
    } else {
        output = 0;  // Normal level
        digitalWrite(LED_PIN_WARNING, LOW);    // Turn off warning LED
        digitalWrite(LED_PIN_CRITICAL, LOW);   // Turn off critical LED
    }

    // Print the readings to the Serial Monitor
    Serial.print("Ambient Temperature: ");
    Serial.print(ambientTemperature);
    Serial.print(" Core Temperature: ");
    Serial.print(coreTemperature);
    Serial.print(" Voltage: ");
    Serial.print(voltage);
    Serial.print(" Output: ");
    Serial.println(output);

    // Send data to Google Sheets
    sendDataToGoogleSheets(ambientTemperature, coreTemperature, voltage, output);

    delay(20000); // 20 seconds delay between readings
}

// Function to read ambient temperature from MLX90416
float readAmbientTemperature() {
    Wire.beginTransmission(MLX90416_ADDRESS);
    Wire.write(0x06);  // Ambient temperature register
    Wire.endTransmission(false);
    Wire.requestFrom(MLX90416_ADDRESS, 2);

    if (Wire.available() == 2) {
        uint16_t tempRaw = Wire.read() | (Wire.read() << 8);
        return tempRaw * 0.02 - 273.15; // Convert raw data to Celsius
    } else {
        return -999.0;  // Error in reading
    }
}

// Function to read core temperature from MLX90416
float readCoreTemperature() {
    Wire.beginTransmission(MLX90416_ADDRESS);
    Wire.write(0x07);  // Core temperature register
    Wire.endTransmission(false);
    Wire.requestFrom(MLX90416_ADDRESS, 2);

    if (Wire.available() == 2) {
        uint16_t tempRaw = Wire.read() | (Wire.read() << 8);
        return tempRaw * 0.02 - 273.15; // Convert raw data to Celsius
    } else {
        return NAN;  // Error in reading
    }
}

// Function to read voltage from the voltage sensor
float readVoltage() {
    int rawValue = analogRead(VOLTAGE_SENSOR_PIN);
    float voltage = (rawValue / 4095.0) * 3.3; // Convert to voltage
    return voltage;
}

// Function to send data to Google Sheets
void sendDataToGoogleSheets(float ambientTemperature, float coreTemperature, float voltage, float output) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Prepare the URL with query parameters
        String postData = "ambientTemperature=" + String(ambientTemperature) +
                          "&coreTemperature=" + String(coreTemperature) +
                          "&voltage=" + String(voltage) +
                          "&output=" + String(output);

        // Make the HTTP POST request
        http.begin(url); // Specify the URL
        http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Specify content-type header

        int httpResponseCode = http.POST(postData); // Send the actual POST request
        if (httpResponseCode > 0) {
            String response = http.getString(); // Get the response to the request
            Serial.println(httpResponseCode);   // Print return code
            Serial.println(response);           // Print response
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);   // Print error
        }

        http.end(); // Free resources
    } else {
        Serial.println("WiFi Disconnected. Cannot send data.");
    }
}
