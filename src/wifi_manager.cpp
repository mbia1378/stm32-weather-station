#include "wifi_manager.h"
#include "config.h"

// Pilotage de l'ESP-01 via commandes AT sur UART
// L'ESP-01 doit tourner avec le firmware AT Espressif standard

static bool connecte = false;

static bool at_envoyer(const String& cmd, const String& attendu, uint32_t timeout_ms = 3000) {
    ESP_SERIAL.println(cmd);
    uint32_t debut = millis();
    String reponse = "";
    while (millis() - debut < timeout_ms) {
        while (ESP_SERIAL.available()) {
            reponse += (char)ESP_SERIAL.read();
        }
        if (reponse.indexOf(attendu) != -1) return true;
    }
    Serial.printf("[WiFi] AT timeout — cmd: %s | réponse: %s\n", cmd.c_str(), reponse.c_str());
    return false;
}

bool wifi_init() {
    ESP_SERIAL.begin(ESP_BAUD);
    delay(500);
    // Reset module
    if (!at_envoyer("AT+RST", "ready", 5000)) {
        Serial.println("[WiFi] Reset ESP-01 échoué");
        return false;
    }
    at_envoyer("ATE0", "OK");                  // désactive l'écho
    at_envoyer("AT+CWMODE=1", "OK");           // mode station
    Serial.println("[WiFi] ESP-01 initialisé");
    return true;
}

bool wifi_connecter() {
    Serial.printf("[WiFi] Connexion à %s...\n", WIFI_SSID);
    String cmd = "AT+CWJAP=\"";
    cmd += WIFI_SSID;
    cmd += "\",\"";
    cmd += WIFI_PASSWORD;
    cmd += "\"";
    connecte = at_envoyer(cmd, "WIFI GOT IP", 15000);
    if (connecte) {
        Serial.printf("[WiFi] Connecté — IP: %s\n", wifi_ip().c_str());
    } else {
        Serial.println("[WiFi] ERREUR: connexion échouée");
    }
    return connecte;
}

bool wifi_est_connecte() {
    return connecte;
}

void wifi_boucle() {
    // Vérifie la connexion toutes les 30s et reconnecte si nécessaire
    static uint32_t derniere_verif = 0;
    if (millis() - derniere_verif < 30000) return;
    derniere_verif = millis();

    if (!at_envoyer("AT+CWJAP?", WIFI_SSID, 3000)) {
        Serial.println("[WiFi] Connexion perdue, reconnexion...");
        connecte = false;
        wifi_connecter();
    }
}

String wifi_ip() {
    ESP_SERIAL.println("AT+CIFSR");
    uint32_t debut = millis();
    String reponse = "";
    while (millis() - debut < 3000) {
        while (ESP_SERIAL.available()) {
            reponse += (char)ESP_SERIAL.read();
        }
        if (reponse.indexOf("STAIP") != -1) break;
    }
    // Extrait l'IP depuis "+CIFSR:STAIP,\"192.168.x.x\""
    int debut_ip = reponse.indexOf("STAIP,\"") + 7;
    int fin_ip   = reponse.indexOf("\"", debut_ip);
    if (debut_ip > 6 && fin_ip > debut_ip) {
        return reponse.substring(debut_ip, fin_ip);
    }
    return "inconnue";
}
