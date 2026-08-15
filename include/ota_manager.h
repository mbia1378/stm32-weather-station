#pragma once
#include <Arduino.h>

enum OtaEtat {
    OTA_IDLE,
    OTA_EN_COURS,
    OTA_SUCCES,
    OTA_ERREUR
};

struct OtaInfo {
    uint32_t taille_totale;
    uint32_t chunks_total;
    uint32_t chunk_actuel;
    char     version[16];
    uint32_t crc32_attendu;
};

bool     ota_init();
OtaEtat  ota_get_etat();
bool     ota_demarrer(const uint8_t* info_json, uint32_t len);
bool     ota_recevoir_chunk(uint32_t index, const uint8_t* data, uint32_t len);
void     ota_finaliser();
uint8_t  ota_get_progression();   // 0-100%
