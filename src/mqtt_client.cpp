#include "mqtt_client.h"
#include "ota_manager.h"
#include "fleet_manager.h"
#include "config.h"
#include <PubSubClient.h>
#include "esp_tcp_client.h"

static EspTcpClient tcp_client;
static PubSubClient mqtt(tcp_client);

// Callback MQTT entrant (OTA + config fleet)
static void on_message(const char* topic, uint8_t* payload, unsigned int len) {
    Serial.printf("[MQTT] Reçu topic: %s (%u bytes)\n", topic, len);

    if (strcmp(topic, MQTT_TOPIC_OTA_CMD) == 0) {
        if (ota_demarrer(payload, len)) {
            mqtt.publish(MQTT_TOPIC_OTA_ACK, "{\"status\":\"started\"}");
        } else {
            mqtt.publish(MQTT_TOPIC_OTA_ACK, "{\"status\":\"error\"}");
        }
        return;
    }

    if (strcmp(topic, MQTT_TOPIC_OTA_CHUNK) == 0 && len >= 4) {
        // Format: [index:4 bytes little-endian][data:N bytes]
        uint32_t index;
        memcpy(&index, payload, 4);
        bool ok = ota_recevoir_chunk(index, payload + 4, len - 4);

        char ack[48];
        snprintf(ack, sizeof(ack), "{\"chunk\":%lu,\"ok\":%s,\"pct\":%u}",
                 index, ok ? "true" : "false", ota_get_progression());
        mqtt.publish(MQTT_TOPIC_OTA_ACK, ack);

        if (ok && ota_get_progression() == 100) {
            mqtt.publish(MQTT_TOPIC_OTA_ACK, "{\"status\":\"finalizing\"}");
            ota_finaliser();
        }
        return;
    }

    if (strcmp(topic, MQTT_TOPIC_CONFIG_IN) == 0) {
        fleet_on_config_recu(payload, len);
        return;
    }
}

bool mqtt_init() {
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    mqtt.setCallback(on_message);
    mqtt.setBufferSize(OTA_CHUNK_SIZE + 64);
    return true;
}

bool mqtt_connecter() {
    if (mqtt.connected()) return true;
    Serial.printf("[MQTT] Connexion à %s:%d...\n", MQTT_SERVER, MQTT_PORT);

    // Will message = "offline" si déconnexion inattendue
    bool ok = mqtt.connect(MQTT_CLIENT_ID,
                           nullptr, nullptr,
                           MQTT_TOPIC_STATUS, 0, true, "offline");
    if (ok) {
        mqtt.publish(MQTT_TOPIC_STATUS, "online", true);
        mqtt.subscribe(MQTT_TOPIC_OTA_CMD);
        mqtt.subscribe(MQTT_TOPIC_OTA_CHUNK);
        mqtt.subscribe(MQTT_TOPIC_CONFIG_IN);
        Serial.println("[MQTT] Connecté + abonnements actifs");
    } else {
        Serial.printf("[MQTT] ERREUR état=%d\n", mqtt.state());
    }
    return ok;
}

bool mqtt_publier(const MesureMeteo& m) {
    if (!mqtt.connected() && !mqtt_connecter()) return false;
    char buf[16];
    bool ok = true;

    if (m.bme_ok) {
        snprintf(buf, sizeof(buf), "%.2f", m.temperature_bme);
        ok &= mqtt.publish(MQTT_TOPIC_TEMP, buf, true);
        snprintf(buf, sizeof(buf), "%.2f", m.humidite_bme);
        ok &= mqtt.publish(MQTT_TOPIC_HUM, buf, true);
        snprintf(buf, sizeof(buf), "%.2f", m.pression_hpa);
        ok &= mqtt.publish(MQTT_TOPIC_PRES, buf, true);
    } else if (m.dht_ok) {
        snprintf(buf, sizeof(buf), "%.2f", m.temperature_dht);
        ok &= mqtt.publish(MQTT_TOPIC_TEMP, buf, true);
        snprintf(buf, sizeof(buf), "%.2f", m.humidite_dht);
        ok &= mqtt.publish(MQTT_TOPIC_HUM, buf, true);
    }

    if (ok) Serial.println("[MQTT] Données publiées");
    return ok;
}

bool mqtt_publier_image(const uint8_t* data, uint32_t taille) {
    if (!mqtt.connected()) return false;
    // Publication en chunks de 4KB max (limite PubSubClient)
    return mqtt.publish(MQTT_TOPIC_CAMERA, data, taille, false);
}

bool mqtt_publier_heartbeat() {
    if (!mqtt.connected()) return false;
    String status = fleet_get_status_json();
    return mqtt.publish(MQTT_TOPIC_HEARTBEAT, status.c_str(), false);
}

void mqtt_boucle()         { mqtt.loop(); }
bool mqtt_est_connecte()   { return mqtt.connected(); }
