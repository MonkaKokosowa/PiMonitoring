#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <LiquidCrystalIO.h>
#include <IoAbstractionWire.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// --- WIFI CREDENTIALS ---
const char* ssid = "CHANGE_ME";
const char* password = "CHANGE_ME";

// --- CONFIGURATION ---
// Poland (CET) is UTC+1 (3600s). Change to 7200 for Summer Time (DST).
const long utcOffsetInSeconds = 3600;

// --- OBJECTS ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

struct RaspberryPi {
    const char* ip;
    const char* name;
    String currentTemp;
};

RaspberryPi myPis[] = {
    {"192.168.0.2", "PI2", "N/A"},
    {"192.168.0.3", "PI3", "N/A"},
    {"192.168.0.4", "PI4", "N/A"}
};

const int deviceCount = 3;
int scrollIndex = 0;
String displayTime = "00:00"; // Holds HH:MM

LiquidCrystalI2C_RS_EN(lcd, 0x27, false)

void fetchTemperatures() {
    WiFiClient client;
    for (int i = 0; i < deviceCount; i++) {
        if (client.connect(myPis[i].ip, 8080)) {
            client.println("GET / HTTP/1.1");
            client.println("Host: " + String(myPis[i].ip));
            client.println("Connection: close");
            client.println();

            while (client.connected()) {
                String line = client.readStringUntil('\n');
                if (line == "\r") break;
            }

            String result = client.readStringUntil('\n');
            if (result.indexOf(":") != -1) {
                myPis[i].currentTemp = result.substring(result.indexOf(":") + 1);
                myPis[i].currentTemp.trim();
            } else {
                myPis[i].currentTemp = result;
            }
            client.stop();
        } else {
            myPis[i].currentTemp = "Off"; // Shortened to ensure space
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    lcd.begin(16, 2);
    lcd.configureBacklightPin(3);
    lcd.backlight();

    lcd.print("WiFi Connect...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    lcd.clear();
    lcd.print("WiFi Online!");

    // Start NTP
    timeClient.begin();

    // OTA Init
    ArduinoOTA.begin(WiFi.localIP(), "PiMonitor", "CHANGE_ME", InternalStorage);

    // Initial fetch
    timeClient.update();
    fetchTemperatures();

    // 1. Fetch Temps (Every 10s)
    taskManager.scheduleFixedRate(10000, fetchTemperatures);

    // 2. Update Time Variable (Every 1s)
    taskManager.scheduleFixedRate(1000, [] {
        timeClient.update();
        // Get HH:MM (substring 0 to 5)
        displayTime = timeClient.getFormattedTime().substring(0, 5);
    });

    // 3. Update Screen (Every 3s)
    taskManager.scheduleFixedRate(3000, [] {
        lcd.clear();

        int first = scrollIndex % deviceCount;
        int second = (scrollIndex + 1) % deviceCount;

        // --- ROW 0 (Top) ---
        lcd.setCursor(0, 0);
        lcd.print(myPis[first].name);
        lcd.print(":");
        lcd.print(myPis[first].currentTemp);

        // Force cursor to column 11 (Right side) for Time
        // "12:00" is 5 chars. 16 - 5 = 11.
        lcd.setCursor(11, 0);
        lcd.print(displayTime);

        // --- ROW 1 (Bottom) ---
        lcd.setCursor(0, 1);
        lcd.print(myPis[second].name);
        lcd.print(":");
        lcd.print(myPis[second].currentTemp);

        scrollIndex++;
    });
}

void loop() {
    ArduinoOTA.poll();
    taskManager.runLoop();
}
