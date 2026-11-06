 #pragma once
  #include "esp_camera.h"
  extern volatile int  g_record_req;
  extern volatile bool g_record;
  bool sd_init();
  void record_service();
  uint8_t *record_encode(camera_fb_t *fb, size_t *len);
  void record_write(uint8_t *buf, size_t len);