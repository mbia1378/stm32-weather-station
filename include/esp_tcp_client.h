#pragma once
#include <Arduino.h>
#include <Client.h>
#include "config.h"

// Implémentation minimale de Client pour PubSubClient via ESP-01 AT
class EspTcpClient : public Client {
public:
    int connect(IPAddress ip, uint16_t port) override { return connect(ip.toString().c_str(), port); }
    int connect(const char* host, uint16_t port) override;

    size_t write(uint8_t b) override          { return write(&b, 1); }
    size_t write(const uint8_t* buf, size_t size) override;

    int available() override                  { return ESP_SERIAL.available(); }
    int read() override                       { return ESP_SERIAL.read(); }
    int read(uint8_t* buf, size_t size) override;
    int peek() override                       { return ESP_SERIAL.peek(); }
    void flush() override                     { ESP_SERIAL.flush(); }
    void stop() override;

    uint8_t connected() override              { return _connected ? 1 : 0; }
    operator bool() override                  { return _connected; }

private:
    bool _connected = false;
    bool _at_ok(const String& cmd, const String& attendu, uint32_t timeout_ms = 5000);
};
