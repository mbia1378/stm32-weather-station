#pragma once
#include "sensors.h"

bool display_init();
void display_afficher_mesures(const MesureMeteo& m);
void display_afficher_statut(const char* ligne1, const char* ligne2 = nullptr);
void display_effacer();
