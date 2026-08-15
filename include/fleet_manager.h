#pragma once
#include <Arduino.h>

struct DeviceStatus {
    char    device_id[32];
    char    firmware_ver[16];
    uint32_t uptime_s;
    int8_t  rssi;
    bool    wifi_ok;
    bool    mqtt_ok;
    bool    sensors_ok;
    float   temp_last;
};

bool    fleet_init();
void    fleet_boucle();
void    fleet_on_config_recu(const uint8_t* payload, uint32_t len);
String  fleet_get_status_json();
uint32_t fleet_get_lecture_intervalle();
