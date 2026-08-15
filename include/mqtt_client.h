#pragma once
#include "sensors.h"

bool mqtt_init();
bool mqtt_connecter();
bool mqtt_publier(const MesureMeteo& m);
bool mqtt_publier_image(const uint8_t* data, uint32_t taille);
bool mqtt_publier_heartbeat();
void mqtt_boucle();
bool mqtt_est_connecte();
