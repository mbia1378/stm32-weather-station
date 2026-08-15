#include "fleet_manager.h"
#include "config.h"
#include <Arduino.h>

static uint32_t lecture_intervalle_ms = LECTURE_INTERVALLE_MS;
static DeviceStatus statut = {};

bool fleet_init() {
    strncpy(statut.device_id,    DEVICE_ID,    sizeof(statut.device_id) - 1);
    strncpy(statut.firmware_ver, FIRMWARE_VER, sizeof(statut.firmware_ver) - 1);
    Serial.println("[FLEET] Initialisé — ID: " DEVICE_ID " v" FIRMWARE_VER);
    return true;
}

void fleet_boucle() {
    statut.uptime_s = millis() / 1000;
}

// Reçoit {"interval":60000} ou {"reboot":true} via MQTT
void fleet_on_config_recu(const uint8_t* payload, uint32_t len) {
    // Parse JSON minimal sans librairie lourde
    String json = String((const char*)payload).substring(0, len);
    Serial.printf("[FLEET] Config reçue: %s\n", json.c_str());

    int idx = json.indexOf("\"interval\":");
    if (idx != -1) {
        uint32_t val = json.substring(idx + 11).toInt();
        if (val >= 5000 && val <= 3600000) {
            lecture_intervalle_ms = val;
            Serial.printf("[FLEET] Intervalle mis à jour: %lu ms\n", val);
        }
    }

    if (json.indexOf("\"reboot\":true") != -1) {
        Serial.println("[FLEET] Redémarrage demandé...");
        delay(500);
        NVIC_SystemReset();
    }
}

String fleet_get_status_json() {
    statut.uptime_s = millis() / 1000;
    char buf[256];
    snprintf(buf, sizeof(buf),
        "{\"id\":\"%s\",\"fw\":\"%s\",\"uptime\":%lu,"
        "\"wifi\":%s,\"mqtt\":%s,\"sensors\":%s,\"temp\":%.2f}",
        statut.device_id,
        statut.firmware_ver,
        statut.uptime_s,
        statut.wifi_ok    ? "true" : "false",
        statut.mqtt_ok    ? "true" : "false",
        statut.sensors_ok ? "true" : "false",
        statut.temp_last
    );
    return String(buf);
}

uint32_t fleet_get_lecture_intervalle() {
    return lecture_intervalle_ms;
}
