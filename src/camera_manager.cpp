#include "camera_manager.h"
#include "config.h"
#include <Wire.h>

// OV7670 registres clés
#define OV7670_ADDR     0x21
#define REG_COM7        0x12
#define REG_COM10       0x15
#define REG_COM14       0x3E
#define REG_CLKRC       0x11
#define REG_COM3        0x0C

static bool    cam_pret   = false;
static uint8_t jpeg_buf[CAM_JPEG_BUF_SIZE];
static uint32_t jpeg_taille = 0;

static bool ov7670_write(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(OV7670_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static uint8_t ov7670_read(uint8_t reg) {
    Wire.beginTransmission(OV7670_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)OV7670_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

static bool ov7670_configurer() {
    // Reset logiciel
    ov7670_write(REG_COM7, 0x80);
    delay(100);

    // QVGA (320x240) JPEG
    ov7670_write(REG_COM7,  0x10);  // JPEG output
    ov7670_write(REG_CLKRC, 0x01);  // prescaler /2
    ov7670_write(REG_COM10, 0x00);
    ov7670_write(REG_COM14, 0x19);
    ov7670_write(REG_COM3,  0x04);  // scale down

    // Vérification ID
    uint8_t id = ov7670_read(0x0A);
    if (id != 0x76) {
        Serial.printf("[CAM] ERREUR: ID inattendu 0x%02X (attendu 0x76)\n", id);
        return false;
    }
    return true;
}

bool camera_init() {
#if !USE_CAMERA
    Serial.println("[CAM] Désactivée (USE_CAMERA=0)");
    return false;
#endif

    // MCO1 sur PA8 pour fournir l'XCLK 24 MHz à l'OV7670
    // (à configurer via HAL_RCC_MCOConfig avant Wire.begin)

    if (!ov7670_configurer()) return false;

    // Configuration DCMI via HAL (simplifié — à compléter selon BSP)
    // Dans un vrai projet : activer DCMI + DMA via CubeMX ou HAL_DCMI_Init
    cam_pret = true;
    Serial.println("[CAM] OV7670 OK — QVGA JPEG");
    return true;
}

bool camera_capturer() {
    if (!cam_pret) return false;
    jpeg_taille = 0;

    // Déclenchement capture DCMI + attente VSYNC
    // En production : HAL_DCMI_Start_DMA(..., jpeg_buf, CAM_JPEG_BUF_SIZE/4)
    // puis attendre HAL_DCMI_STATE_READY ou callback
    // Ici : simulation pour l'architecture

    Serial.println("[CAM] Capture en cours...");
    delay(100);   // temps de frame ~33ms à 30fps

    // Marqueurs JPEG de test (à remplacer par le buffer DMA réel)
    jpeg_buf[0] = 0xFF; jpeg_buf[1] = 0xD8;  // SOI
    jpeg_buf[2] = 0xFF; jpeg_buf[3] = 0xD9;  // EOI
    jpeg_taille = 4;

    Serial.printf("[CAM] Image capturée — %lu bytes\n", jpeg_taille);
    return true;
}

uint8_t* camera_get_buffer() { return jpeg_buf; }
uint32_t camera_get_taille() { return jpeg_taille; }
void     camera_liberer()    { jpeg_taille = 0; }
