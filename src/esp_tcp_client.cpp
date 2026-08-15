#include "esp_tcp_client.h"

bool EspTcpClient::_at_ok(const String& cmd, const String& attendu, uint32_t timeout_ms) {
    ESP_SERIAL.println(cmd);
    uint32_t debut = millis();
    String rep = "";
    while (millis() - debut < timeout_ms) {
        while (ESP_SERIAL.available()) rep += (char)ESP_SERIAL.read();
        if (rep.indexOf(attendu) != -1) return true;
    }
    return false;
}

int EspTcpClient::connect(const char* host, uint16_t port) {
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += host;
    cmd += "\",";
    cmd += port;
    _connected = _at_ok(cmd, "CONNECT", 8000);
    return _connected ? 1 : 0;
}

size_t EspTcpClient::write(const uint8_t* buf, size_t size) {
    String cmd = "AT+CIPSEND=";
    cmd += size;
    if (!_at_ok(cmd, ">", 3000)) return 0;
    ESP_SERIAL.write(buf, size);
    if (_at_ok("", "SEND OK", 5000)) return size;
    return 0;
}

int EspTcpClient::read(uint8_t* buf, size_t size) {
    size_t n = 0;
    uint32_t debut = millis();
    while (n < size && millis() - debut < 1000) {
        if (ESP_SERIAL.available()) {
            buf[n++] = ESP_SERIAL.read();
        }
    }
    return (int)n;
}

void EspTcpClient::stop() {
    _at_ok("AT+CIPCLOSE", "OK", 2000);
    _connected = false;
}
