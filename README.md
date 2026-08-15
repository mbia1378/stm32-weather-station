# Station Météo STM32

Firmware embarqué pour station météo connectée sur STM32, avec affichage local, connectivité WiFi/LoRa, publication MQTT, mise à jour OTA et stack cloud complète (Grafana + InfluxDB).

---

## Matériel requis

| Composant | Rôle |
|---|---|
| STM32 Nucleo-F401RE | Microcontrôleur principal |
| Module ESP-01 (ESP8266) | Connectivité WiFi via commandes AT |
| BME280 | Température, humidité, pression (I2C) |
| DHT22 | Température et humidité (backup) |
| OLED SSD1306 128x64 | Affichage local (I2C) |
| SX1276 / RFM95 *(optionnel)* | Communication LoRa 868 MHz (SPI) |
| OV7670 *(optionnel)* | Caméra QVGA JPEG (DCMI) |

---

## Architecture logicielle

```
main.cpp (FreeRTOS)
├── vTaskSensors   — lecture BME280 + DHT22 toutes les 30s
├── vTaskDisplay   — mise à jour écran OLED
├── vTaskNetwork   — WiFi + MQTT + LoRa + heartbeat fleet
├── vTaskCamera    — capture OV7670 toutes les 5 min (optionnel)
└── vTaskWatchdog  — uptime + rafraîchissement IWDG
```

Les tâches communiquent via une **queue FreeRTOS** (`xQueueMesures`) sans partage de variable globale.

---

## Câblage

### I2C — BME280 + OLED SSD1306

| Signal | Broche STM32 |
|---|---|
| SDA | PB7 |
| SCL | PB6 |
| Adresse BME280 | 0x76 (SDO=GND) ou 0x77 (SDO=VCC) |
| Adresse OLED | 0x3C |

### DHT22

| Signal | Broche STM32 |
|---|---|
| DATA | PA1 |

### ESP-01 (UART)

| Signal | Broche STM32 |
|---|---|
| TX (ESP) → RX (STM32) | PA3 (Serial2) |
| RX (ESP) → TX (STM32) | PA2 (Serial2) |
| VCC | 3.3V ⚠️ (pas 5V) |

> L'ESP-01 doit tourner avec le **firmware AT standard Espressif** (v2.x ou supérieur).

### SX1276 LoRa *(optionnel)*

| Signal | Broche STM32 |
|---|---|
| NSS (CS) | PA4 |
| RST | PC3 |
| DIO0 | PC4 |
| SCK / MISO / MOSI | SPI1 par défaut |

---

## Installation et compilation

### Prérequis

- [PlatformIO](https://platformio.org/) (extension VS Code ou CLI)
- Python 3.x

### Cloner et compiler

```bash
git clone https://github.com/mbia1378/stm32-weather-station.git
cd stm32-weather-station

# Copier et remplir la configuration
cp include/config.h.example include/config.h
# Éditer config.h avec ton SSID, mot de passe et IP du broker MQTT

# Compiler et flasher
pio run -t upload
```

### Variantes de build

```bash
# WiFi uniquement (défaut)
pio run -e nucleo_f401re

# Avec LoRa activé
pio run -e nucleo_f401re_lora

# Tout activé (WiFi + LoRa + Caméra)
pio run -e nucleo_f401re_full
```

---

## Configuration

Édite `include/config.h` avant de compiler :

```c
// Identité
#define DEVICE_ID       "stm32-meteo-01"   // unique par appareil

// WiFi
#define WIFI_SSID       "TON_SSID"
#define WIFI_PASSWORD   "TON_MOT_DE_PASSE"

// MQTT (IP de ta machine avec la stack Docker)
#define MQTT_SERVER     "192.168.1.100"

// Feature flags
#define USE_LORA        0   // 1 pour activer le module SX1276
#define USE_CAMERA      0   // 1 pour activer l'OV7670
#define USE_OTA         1   // mise à jour firmware à distance
```

> `config.h` est exclu du dépôt git (`.gitignore`) — tes credentials ne sont jamais publiés.

---

## Topics MQTT

| Topic | Direction | Contenu |
|---|---|---|
| `meteo/{id}/temperature` | STM32 → broker | `float` en °C |
| `meteo/{id}/humidite` | STM32 → broker | `float` en % |
| `meteo/{id}/pression` | STM32 → broker | `float` en hPa |
| `meteo/{id}/image` | STM32 → broker | JPEG binaire |
| `fleet/{id}/heartbeat` | STM32 → broker | JSON statut |
| `fleet/{id}/status` | STM32 → broker | `online` / `offline` (will) |
| `fleet/{id}/config` | broker → STM32 | JSON config distante |
| `fleet/{id}/ota/start` | broker → STM32 | JSON info firmware |
| `fleet/{id}/ota/chunk` | broker → STM32 | Binaire chunk firmware |
| `fleet/{id}/ota/ack` | STM32 → broker | JSON progression OTA |

---

## Stack cloud (Grafana + InfluxDB)

```bash
cd docker/
docker compose up -d
```

| Service | URL | Identifiants |
|---|---|---|
| **Grafana** | http://localhost:3000 | admin / meteo1234 |
| **Node-RED** | http://localhost:1880 | — |
| **InfluxDB** | http://localhost:8086 | admin / meteo1234 |
| **MQTT Broker** | localhost:1883 | accès libre |

Le dashboard Grafana est **pré-configuré** et s'affiche automatiquement au démarrage.

Le flux Node-RED reçoit les topics MQTT et les écrit dans InfluxDB automatiquement.

---

## Mise à jour OTA

Envoyer un firmware à distance en 3 étapes :

**1. Annoncer la mise à jour**
```bash
mosquitto_pub -h localhost -t "fleet/stm32-meteo-01/ota/start" \
  -m '{"version":"2.1.0","size":65536,"chunks":128,"crc32":3456789012}'
```

**2. Envoyer les chunks** (script Python fourni à venir)

**3. Le STM32 vérifie le CRC32, écrit en Flash et redémarre automatiquement.**

---

## Gestion de flotte à distance

### Changer l'intervalle de mesure

```bash
mosquitto_pub -h localhost -t "fleet/stm32-meteo-01/config" \
  -m '{"interval":60000}'
```

### Redémarrer un appareil à distance

```bash
mosquitto_pub -h localhost -t "fleet/stm32-meteo-01/config" \
  -m '{"reboot":true}'
```

---

## Structure du projet

```
stm32-weather-station/
├── include/
│   ├── config.h          — configuration (non versionné)
│   ├── sensors.h
│   ├── display.h
│   ├── wifi_manager.h
│   ├── mqtt_client.h
│   ├── esp_tcp_client.h
│   ├── ota_manager.h
│   ├── lora_manager.h
│   ├── camera_manager.h
│   └── fleet_manager.h
├── src/
│   ├── main.cpp          — tâches FreeRTOS
│   ├── sensors.cpp
│   ├── display.cpp
│   ├── wifi_manager.cpp
│   ├── mqtt_client.cpp
│   ├── esp_tcp_client.cpp
│   ├── ota_manager.cpp
│   ├── lora_manager.cpp
│   ├── camera_manager.cpp
│   └── fleet_manager.cpp
├── docker/
│   ├── docker-compose.yml
│   ├── mosquitto/
│   ├── grafana/
│   └── node-red/
└── platformio.ini
```

---

## Licence

MIT
