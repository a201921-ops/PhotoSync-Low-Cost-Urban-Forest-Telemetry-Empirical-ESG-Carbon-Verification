#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MAX31865.h>

// --- NETWORK CREDENTIALS ---
const char* ssid = "ur ssid";
const char* password = "ur password";

// Replace with the Deployment ID of the new (or existing) Google Web App
String scriptID = "AKfycbyvvRxfzT2t5TwdufWMV4oi_T-Eu0Pd_3pok3Jmw_r8zIIWM8mpeOMVp4Dqr503GmUo";

// --- SENSOR SETUP ---
// Bus pins: CS=10, MOSI=12, MISO=13, SCK=11
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 12, 13, 11);
#define RREF      430.0
#define RNOMINAL  100.0

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setTextSize(2);
    M5.Display.println("WiFi Connecting...");

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Display.print(".");
    }
    M5.Display.println("\nWiFi OK!");

    Serial1.begin(9600, SERIAL_8N1, 17, 18); // K30 CO2 (Port C)
    thermo.begin(MAX31865_3WIRE);           // PT100 3-wire
}

void logToGoogle(int c, float t) {
    if (WiFi.status() == WL_CONNECTED && t > -100 && t < 900) {
        HTTPClient http;
        String url = "https://script.google.com/macros/s/" + scriptID + "/exec";

        http.begin(url);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.addHeader("Content-Type", "application/json");

        String json = "{\"co2\":" + String(c) + ",\"temp\":" + String(t) + "}";
        int httpCode = http.POST(json);

        M5.Display.setCursor(10, 180);
        M5.Display.setTextColor(WHITE);
        M5.Display.printf("Sheets: %d", httpCode);

        http.end();
    }
}

void loop() {
    M5.update();

    // 1. Read K30 CO2
    byte request[] = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};
    Serial1.write(request, 7);
    delay(150);

    int co2Val = 0;
    if (Serial1.available() >= 7) {
        byte response[7];
        Serial1.readBytes(response, 7);
        co2Val = (response[3] << 8) | response[4];
    }

    // 2. Read PT100 Temp
    float tempVal = thermo.temperature(RNOMINAL, RREF);
    uint8_t fault = thermo.readFault();
    if (fault) thermo.clearFault();

    // 3. UI Display
    M5.Display.startWrite();
    M5.Display.fillScreen(BLACK);
    M5.Display.setCursor(10, 30);
    M5.Display.setTextColor(GREEN);
    M5.Display.printf("CO2: %d ppm", co2Val);

    M5.Display.setCursor(10, 80);
    if (tempVal < -100 || tempVal > 900) {
        M5.Display.setTextColor(RED);
        M5.Display.printf("Temp: FAULT");
    } else {
        M5.Display.setTextColor(CYAN);
        M5.Display.printf("Temp: %.2f C", tempVal);
    }
    M5.Display.endWrite();

    // 4. Log to Google
    logToGoogle(co2Val, tempVal);

    delay(30000);
}

