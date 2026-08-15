#include <Arduino.h>
#include <Wire.h>
#include <STM32FreeRTOS.h>
#include "config.h"
#include "sensors.h"
#include "display.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "ota_manager.h"
#include "fleet_manager.h"
#include "lora_manager.h"
#include "camera_manager.h"

// ─── Queues inter-tâches ────────────────────────────────
static QueueHandle_t  xQueueMesures;
static SemaphoreHandle_t xMutexSerial;

// ─── Tâche : lecture capteurs ───────────────────────────
static void vTaskSensors(void*) {
    TickType_t xLastWake = xTaskGetTickCount();
    for (;;) {
        MesureMeteo m = sensors_lire();

        if (xSemaphoreTake(xMutexSerial, pdMS_TO_TICKS(100)) == pdTRUE) {
            sensors_afficher_serie(m);
            xSemaphoreGive(xMutexSerial);
        }

        // Envoie aux autres tâches (non bloquant — écrase l'ancienne valeur si plein)
        xQueueOverwrite(xQueueMesures, &m);

        uint32_t intervalle = fleet_get_lecture_intervalle();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(intervalle));
    }
}

// ─── Tâche : affichage OLED ─────────────────────────────
static void vTaskDisplay(void*) {
    MesureMeteo m = {};
    for (;;) {
        if (xQueuePeek(xQueueMesures, &m, pdMS_TO_TICKS(5000)) == pdTRUE) {
            display_afficher_mesures(m);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ─── Tâche : réseau (WiFi + MQTT + LoRa) ────────────────
static void vTaskNetwork(void*) {
    MesureMeteo m    = {};
    uint32_t hb_last = 0;

    for (;;) {
        wifi_boucle();
        mqtt_boucle();

        if (xQueueReceive(xQueueMesures, &m, 0) == pdTRUE) {
            if (wifi_est_connecte()) {
                if (!mqtt_est_connecte()) mqtt_connecter();
                mqtt_publier(m);
            }
#if USE_LORA
            lora_envoyer(m);
#endif
        }

        // Heartbeat fleet toutes les HEARTBEAT_INTERVALLE_MS
        if (millis() - hb_last >= HEARTBEAT_INTERVALLE_MS) {
            hb_last = millis();
            mqtt_publier_heartbeat();
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ─── Tâche : caméra (capture + publication) ─────────────
#if USE_CAMERA
static void vTaskCamera(void*) {
    for (;;) {
        // Capture toutes les 5 minutes
        vTaskDelay(pdMS_TO_TICKS(300000));
        if (camera_capturer()) {
            mqtt_publier_image(camera_get_buffer(), camera_get_taille());
            camera_liberer();
        }
    }
}
#endif

// ─── Tâche : watchdog système ────────────────────────────
static void vTaskWatchdog(void*) {
    for (;;) {
        fleet_boucle();
        // En production : rafraîchir le hardware IWDG ici
        // HAL_IWDG_Refresh(&hiwdg);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ─── Setup ──────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Station Météo STM32 v" FIRMWARE_VER " ===");

    Wire.begin(I2C_SDA, I2C_SCL);

    display_init();
    sensors_init();
    fleet_init();
    ota_init();

    display_afficher_statut("WiFi...");
    bool wifi_ok = wifi_init() && wifi_connecter();

    if (wifi_ok) {
        mqtt_init();
        mqtt_connecter();
        display_afficher_statut("WiFi OK", mqtt_est_connecte() ? "MQTT OK" : "MQTT...");
    } else {
        display_afficher_statut("Hors ligne", "Mode local");
    }

#if USE_LORA
    lora_init();
#endif

#if USE_CAMERA
    camera_init();
#endif

    // Queues et mutex
    xQueueMesures  = xQueueCreate(1, sizeof(MesureMeteo));
    xMutexSerial   = xSemaphoreCreateMutex();

    // Création des tâches FreeRTOS
    xTaskCreate(vTaskSensors,  "sensors",   512, nullptr, 3, nullptr);
    xTaskCreate(vTaskDisplay,  "display",   256, nullptr, 1, nullptr);
    xTaskCreate(vTaskNetwork,  "network",   1024, nullptr, 2, nullptr);
    xTaskCreate(vTaskWatchdog, "watchdog",  128, nullptr, 4, nullptr);
#if USE_CAMERA
    xTaskCreate(vTaskCamera,   "camera",    512, nullptr, 1, nullptr);
#endif

    vTaskStartScheduler();
    // On n'arrive jamais ici
}

void loop() {}
