#pragma once
#include <Arduino.h>

struct MesureMeteo {
    float temperature_bme;
    float humidite_bme;
    float pression_hpa;
    float temperature_dht;
    float humidite_dht;
    bool  bme_ok;
    bool  dht_ok;
};

bool    sensors_init();
MesureMeteo sensors_lire();
void    sensors_afficher_serie(const MesureMeteo& m);
