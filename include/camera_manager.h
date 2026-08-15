#pragma once
#include <Arduino.h>

#define CAM_FRAME_WIDTH   320
#define CAM_FRAME_HEIGHT  240
#define CAM_JPEG_BUF_SIZE (20 * 1024)   // 20 KB buffer JPEG

bool     camera_init();
bool     camera_capturer();
uint8_t* camera_get_buffer();
uint32_t camera_get_taille();
void     camera_liberer();
