#pragma once
#include <Arduino.h>

bool    wifi_init();
bool    wifi_connecter();
bool    wifi_est_connecte();
void    wifi_boucle();            // à appeler dans loop() pour garder la connexion
String  wifi_ip();
