#include "mqtt_client.h"
#include "config.h"
#include <PubSubClient.h>

// Couche TCP via AT commands (ESP-01)
// On utilise une classe TCP minimaliste qui wrape l'UART AT
#include "esp_tcp_client.h"

static EspTcpClient tcp_client;
static PubSubClient mqtt(tcp_client);

bool mqtt_init() {
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    return true;
}

bool mqtt_connecter() {
    if (mqtt.connected()) return true;
    Serial.printf("[MQTT] Connexion à %s:%d...\n", MQTT_SERVER, MQTT_PORT);
    bool ok = mqtt.connect(MQTT_CLIENT_ID);
    if (ok) {
        Serial.println("[MQTT] Connecté");
    } else {
        Serial.printf("[MQTT] ERREUR état=%d\n", mqtt.state());
    }
    return ok;
}

bool mqtt_publier(const MesureMeteo& m) {
    if (!mqtt.connected() && !mqtt_connecter()) return false;

    bool ok = true;
    char payload[16];

    if (m.bme_ok) {
        snprintf(payload, sizeof(payload), "%.2f", m.temperature_bme);
        ok &= mqtt.publish(MQTT_TOPIC_TEMP, payload, true);

        snprintf(payload, sizeof(payload), "%.2f", m.humidite_bme);
        ok &= mqtt.publish(MQTT_TOPIC_HUM, payload, true);

        snprintf(payload, sizeof(payload), "%.2f", m.pression_hpa);
        ok &= mqtt.publish(MQTT_TOPIC_PRES, payload, true);
    } else if (m.dht_ok) {
        snprintf(payload, sizeof(payload), "%.2f", m.temperature_dht);
        ok &= mqtt.publish(MQTT_TOPIC_TEMP, payload, true);

        snprintf(payload, sizeof(payload), "%.2f", m.humidite_dht);
        ok &= mqtt.publish(MQTT_TOPIC_HUM, payload, true);
    }

    if (ok) Serial.println("[MQTT] Données publiées");
    else    Serial.println("[MQTT] ERREUR: publication échouée");
    return ok;
}

void mqtt_boucle() {
    mqtt.loop();
}

bool mqtt_est_connecte() {
    return mqtt.connected();
}
