#include "ota_manager.h"
#include "config.h"
#include <Arduino.h>
#include <stm32f4xx_hal.h>

static OtaEtat  etat        = OTA_IDLE;
static OtaInfo  info        = {};
static uint32_t ecrit_bytes = 0;
static uint32_t crc_calcule = 0;

// CRC32 table-based (IEEE 802.3)
static uint32_t crc32_update(uint32_t crc, const uint8_t* data, uint32_t len) {
    static const uint32_t table[16] = {
        0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,
        0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
        0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,
        0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
    };
    crc = ~crc;
    for (uint32_t i = 0; i < len; i++) {
        crc = table[(crc ^ data[i]) & 0x0F] ^ (crc >> 4);
        crc = table[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}

static bool flash_effacer_secteur(uint32_t secteur) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef ef = {};
    ef.TypeErase    = FLASH_TYPEERASE_SECTORS;
    ef.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    ef.Sector       = secteur;
    ef.NbSectors    = 1;
    uint32_t err = 0;
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&ef, &err);
    HAL_FLASH_Lock();
    return (st == HAL_OK && err == 0xFFFFFFFF);
}

static bool flash_ecrire(uint32_t addr, const uint8_t* data, uint32_t len) {
    HAL_FLASH_Unlock();
    bool ok = true;
    for (uint32_t i = 0; i < len && ok; i++) {
        ok = (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, data[i]) == HAL_OK);
    }
    HAL_FLASH_Lock();
    return ok;
}

bool ota_init() {
    etat = OTA_IDLE;
    Serial.println("[OTA] Prêt — addr Flash: 0x" + String(OTA_FLASH_ADDR, HEX));
    return true;
}

OtaEtat ota_get_etat() { return etat; }

// payload JSON: {"version":"2.1.0","size":65536,"chunks":128,"crc32":3456789012}
bool ota_demarrer(const uint8_t* payload, uint32_t len) {
    if (etat == OTA_EN_COURS) return false;
    String json((const char*)payload, len);

    // Parse minimal
    auto extract_uint = [&](const char* key) -> uint32_t {
        int i = json.indexOf(key);
        return i != -1 ? json.substring(i + strlen(key)).toInt() : 0;
    };

    info.taille_totale  = extract_uint("\"size\":");
    info.chunks_total   = extract_uint("\"chunks\":");
    info.crc32_attendu  = extract_uint("\"crc32\":");
    info.chunk_actuel   = 0;

    int vi = json.indexOf("\"version\":\"");
    if (vi != -1) {
        String ver = json.substring(vi + 11);
        strncpy(info.version, ver.substring(0, ver.indexOf('"')).c_str(), 15);
    }

    if (info.taille_totale == 0 || info.taille_totale > OTA_MAX_SIZE) {
        Serial.println("[OTA] ERREUR: taille invalide");
        etat = OTA_ERREUR;
        return false;
    }

    Serial.printf("[OTA] Démarrage v%s — %lu bytes / %lu chunks\n",
                  info.version, info.taille_totale, info.chunks_total);

    // Effacement secteur 6 (0x08040000 sur F401)
    if (!flash_effacer_secteur(FLASH_SECTOR_6)) {
        Serial.println("[OTA] ERREUR: effacement Flash échoué");
        etat = OTA_ERREUR;
        return false;
    }

    ecrit_bytes = 0;
    crc_calcule = 0;
    etat = OTA_EN_COURS;
    return true;
}

bool ota_recevoir_chunk(uint32_t index, const uint8_t* data, uint32_t len) {
    if (etat != OTA_EN_COURS || index != info.chunk_actuel) return false;

    uint32_t addr = OTA_FLASH_ADDR + (index * OTA_CHUNK_SIZE);
    if (!flash_ecrire(addr, data, len)) {
        Serial.printf("[OTA] ERREUR: écriture chunk %lu échouée\n", index);
        etat = OTA_ERREUR;
        return false;
    }

    crc_calcule  = crc32_update(crc_calcule, data, len);
    ecrit_bytes += len;
    info.chunk_actuel++;

    Serial.printf("[OTA] Chunk %lu/%lu OK (%u%%)\n",
                  index + 1, info.chunks_total, ota_get_progression());
    return true;
}

void ota_finaliser() {
    if (etat != OTA_EN_COURS) return;

    if (info.chunk_actuel != info.chunks_total) {
        Serial.printf("[OTA] ERREUR: chunks incomplets (%lu/%lu)\n",
                      info.chunk_actuel, info.chunks_total);
        etat = OTA_ERREUR;
        return;
    }

    if (crc_calcule != info.crc32_attendu) {
        Serial.printf("[OTA] ERREUR: CRC mismatch (reçu 0x%08lX, attendu 0x%08lX)\n",
                      crc_calcule, info.crc32_attendu);
        etat = OTA_ERREUR;
        return;
    }

    etat = OTA_SUCCES;
    Serial.printf("[OTA] Succès v%s — redémarrage dans 2s\n", info.version);
    delay(2000);

    // Saut vers le nouveau firmware
    typedef void (*pFunc)(void);
    uint32_t sp   = *(volatile uint32_t*)OTA_FLASH_ADDR;
    uint32_t pc   = *(volatile uint32_t*)(OTA_FLASH_ADDR + 4);
    __set_MSP(sp);
    ((pFunc)pc)();
}

uint8_t ota_get_progression() {
    if (info.chunks_total == 0) return 0;
    return (uint8_t)((info.chunk_actuel * 100) / info.chunks_total);
}
