#pragma once
#include "sensors.h"

bool lora_init();
bool lora_envoyer(const MesureMeteo& m);
int  lora_rssi();
bool lora_disponible();
