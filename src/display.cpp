#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

bool display_init() {
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[DISPLAY] ERREUR: SSD1306 introuvable");
        return false;
    }
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(20, 20);
    oled.setTextSize(1);
    oled.println("Station Meteo");
    oled.setCursor(30, 35);
    oled.println("Demarrage...");
    oled.display();
    Serial.println("[DISPLAY] SSD1306 OK");
    return true;
}

void display_afficher_mesures(const MesureMeteo& m) {
    oled.clearDisplay();

    // Titre
    oled.setTextSize(1);
    oled.setCursor(25, 0);
    oled.println("Station Meteo");
    oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Température (source BME280 prioritaire)
    float temp = m.bme_ok ? m.temperature_bme : m.temperature_dht;
    float hum  = m.bme_ok ? m.humidite_bme   : m.humidite_dht;

    oled.setTextSize(2);
    oled.setCursor(0, 14);
    oled.printf("%.1f C", temp);

    oled.setTextSize(1);
    oled.setCursor(0, 36);
    oled.printf("Hum: %.1f%%", hum);

    if (m.bme_ok) {
        oled.setCursor(0, 46);
        oled.printf("Pres: %.0fhPa", m.pression_hpa);
    }

    // Indicateur source
    oled.setCursor(90, 56);
    oled.setTextSize(1);
    oled.println(m.bme_ok ? "BME" : "DHT");

    oled.display();
}

void display_afficher_statut(const char* ligne1, const char* ligne2) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 20);
    oled.println(ligne1);
    if (ligne2) {
        oled.setCursor(0, 35);
        oled.println(ligne2);
    }
    oled.display();
}

void display_effacer() {
    oled.clearDisplay();
    oled.display();
}
