#include "lora_manager.h"
#include "config.h"
#include <LoRa.h>

static bool lora_pret = false;

bool lora_init() {
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(LORA_FREQUENCY)) {
        Serial.println("[LORA] ERREUR: module SX1276 introuvable");
        return false;
    }
    LoRa.setSpreadingFactor(9);        // SF9 — bon compromis portée/débit
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    LoRa.setTxPower(14);               // 14 dBm (~25 mW)
    LoRa.enableCrc();
    lora_pret = true;
    Serial.printf("[LORA] OK — %.0f MHz SF9 BW125\n", LORA_FREQUENCY / 1E6);
    return true;
}

// Format compact binaire : ID(4) + temp(4) + hum(4) + pres(4) = 16 bytes
bool lora_envoyer(const MesureMeteo& m) {
    if (!lora_pret) return false;

    float temp = m.bme_ok ? m.temperature_bme : m.temperature_dht;
    float hum  = m.bme_ok ? m.humidite_bme   : m.humidite_dht;
    float pres = m.bme_ok ? m.pression_hpa   : 0.0f;

    // Encodage fixe point pour économiser les bytes air-time
    int16_t t_enc = (int16_t)(temp * 100);
    int16_t h_enc = (int16_t)(hum  * 100);
    int16_t p_enc = (int16_t)((pres - 900.0f) * 10);  // offset 900 hPa

    LoRa.beginPacket();
    LoRa.write((const uint8_t*)DEVICE_ID, 4);  // 4 premiers octets de l'ID
    LoRa.write((uint8_t*)&t_enc, 2);
    LoRa.write((uint8_t*)&h_enc, 2);
    LoRa.write((uint8_t*)&p_enc, 2);
    int ok = LoRa.endPacket();

    if (ok) Serial.printf("[LORA] Paquet envoyé — T=%.1f H=%.1f P=%.1f\n", temp, hum, pres);
    else    Serial.println("[LORA] ERREUR: envoi échoué");
    return ok == 1;
}

int lora_rssi() {
    return lora_pret ? LoRa.rssi() : -999;
}

bool lora_disponible() {
    return lora_pret;
}
