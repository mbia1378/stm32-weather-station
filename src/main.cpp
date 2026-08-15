#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "wifi_manager.h"
#include "mqtt_client.h"

static uint32_t derniere_lecture = 0;
static bool     wifi_pret = false;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Station Météo STM32 ===");

    Wire.begin(I2C_SDA, I2C_SCL);

    display_init();
    sensors_init();

    display_afficher_statut("WiFi...");
    wifi_pret = wifi_init() && wifi_connecter();

    if (wifi_pret) {
        mqtt_init();
        mqtt_connecter();
        display_afficher_statut("WiFi OK", mqtt_est_connecte() ? "MQTT OK" : "MQTT...");
    } else {
        display_afficher_statut("Hors ligne", "Mode local");
    }

    delay(1500);
    derniere_lecture = millis() - LECTURE_INTERVALLE_MS;  // force première lecture immédiate
}

void loop() {
    if (wifi_pret) {
        wifi_boucle();
        mqtt_boucle();
    }

    if (millis() - derniere_lecture >= LECTURE_INTERVALLE_MS) {
        derniere_lecture = millis();

        MesureMeteo mesure = sensors_lire();
        sensors_afficher_serie(mesure);
        display_afficher_mesures(mesure);

        if (wifi_pret) {
            mqtt_publier(mesure);
        }
    }
}
