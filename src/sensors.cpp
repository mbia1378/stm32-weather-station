#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <DHT.h>

static Adafruit_BME280 bme;
static DHT dht(DHT_PIN, DHT_TYPE);
static bool bme_disponible = false;

bool sensors_init() {
    dht.begin();
    bme_disponible = bme.begin(0x76);  // adresse I2C BME280 (0x77 si SDO=VCC)
    if (!bme_disponible) {
        Serial.println("[SENSOR] BME280 non trouvé à 0x76, essai 0x77...");
        bme_disponible = bme.begin(0x77);
    }
    if (bme_disponible) {
        // Mode weather monitoring : échantillonnage faible consommation
        bme.setSampling(Adafruit_BME280::MODE_FORCED,
                        Adafruit_BME280::SAMPLING_X1,  // temp
                        Adafruit_BME280::SAMPLING_X1,  // pression
                        Adafruit_BME280::SAMPLING_X1,  // humidité
                        Adafruit_BME280::FILTER_OFF);
        Serial.println("[SENSOR] BME280 OK");
    } else {
        Serial.println("[SENSOR] ERREUR: BME280 introuvable");
    }
    return bme_disponible;
}

MesureMeteo sensors_lire() {
    MesureMeteo m = {};

    if (bme_disponible) {
        bme.takeForcedMeasurement();
        m.temperature_bme = bme.readTemperature();
        m.humidite_bme    = bme.readHumidity();
        m.pression_hpa    = bme.readPressure() / 100.0f;
        m.bme_ok = !isnan(m.temperature_bme);
    }

    m.temperature_dht = dht.readTemperature();
    m.humidite_dht    = dht.readHumidity();
    m.dht_ok = !isnan(m.temperature_dht) && !isnan(m.humidite_dht);

    return m;
}

void sensors_afficher_serie(const MesureMeteo& m) {
    Serial.println("─── Mesures ───────────────────────");
    if (m.bme_ok) {
        Serial.printf("  BME280 Temp    : %.2f °C\n", m.temperature_bme);
        Serial.printf("  BME280 Humidité: %.2f %%\n", m.humidite_bme);
        Serial.printf("  BME280 Pression: %.2f hPa\n", m.pression_hpa);
    }
    if (m.dht_ok) {
        Serial.printf("  DHT22  Temp    : %.2f °C\n", m.temperature_dht);
        Serial.printf("  DHT22  Humidité: %.2f %%\n", m.humidite_dht);
    }
    Serial.println("───────────────────────────────────");
}
