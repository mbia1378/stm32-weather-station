#pragma once
#include "sensors.h"

bool mqtt_init();
bool mqtt_connecter();
bool mqtt_publier(const MesureMeteo& m);
void mqtt_boucle();
bool mqtt_est_connecte();
