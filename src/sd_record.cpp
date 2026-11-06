#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "img_converters.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "control.h"
#include "sd_record.h"

#define SD_CS        21
#define REC_QUEUE    4
#define JPEG_QUALITY 80

volatile int  g_record_req = 0;
volatile bool g_record     = false;

extern int g_dnn_deg;

struct RecItem {
  uint8_t     *buf;
  size_t       len;
  uint16_t     w, h;
  pixformat_t  fmt;
  int64_t      ts_ms;
  int          steer_deg;
  int          dnn_deg;
  int          throttle;
};

static QueueHandle_t q = NULL;
static File vid, csv;
static uint32_t n = 0, dropped = 0;
static bool sd_ok = false;

static uint16_t s_w, s_h;
static pixformat_t s_fmt;

static bool open_files() {
  char path[24];
  int i = 0;
  do {
    snprintf(path, sizeof(path), "/rec/%03d.mjpg", i);
    i++;
  } while (SD.exists(path));
  int id = i - 1;

  vid = SD.open(path, FILE_WRITE);
  if (!vid) {
    Serial.printf("Failed to open %s\n", path);
    return false;
  }

  snprintf(path, sizeof(path), "/rec/%03d.csv", id);
  csv = SD.open(path, FILE_WRITE);
  if (!csv) {
    Serial.printf("Failed to open %s\n", path);
    vid.close();
    return false;
  }

  csv.println("ts,frame,wheel,ai,throttle");
  n = 0;
  dropped = 0;
  Serial.printf("Recording to /rec/%03d.mjpg + .csv\n", id);
  return true;
}

static void close_files() {
  if (vid) { vid.flush(); vid.close(); }
  if (csv) { csv.flush(); csv.close(); }
  Serial.printf("Recording stopped: %u frames, %u dropped\n", n, dropped);
}

static void write_item(RecItem &it) {
  uint8_t *jpg = it.buf;
  size_t jlen = it.len;

  if (it.fmt != PIXFORMAT_JPEG) {
    uint8_t *out = NULL;
    size_t olen = 0;
    bool ok = fmt2jpg(it.buf, it.len, it.w, it.h, it.fmt,
                      JPEG_QUALITY, &out, &olen);
    free(it.buf);
    if (!ok || !out) {
      Serial.println("JPEG encode failed");
      return;
    }
    jpg = out;
    jlen = olen;
  }

  size_t w = vid.write(jpg, jlen);
  free(jpg);

  if (w != jlen) {
    Serial.println("SD video write failed, stopping");
    g_record_req = 0;
    return;
  }

  n++;
  csv.printf("%lld,%u,%d,%d,%d\n",
             (long long)it.ts_ms, n, it.steer_deg, it.dnn_deg, it.throttle);

  if (n % 50 == 0) {
    vid.flush();
    csv.flush();
  }
}

static void rec_task(void *) {
  RecItem it;
  for (;;) {
    if (g_record_req && !g_record) {
      if (open_files()) {
        g_record = true;
        Serial.println("Recording started");
      } else {
        g_record_req = 0;
      }
    }

    if (xQueueReceive(q, &it, pdMS_TO_TICKS(50)) == pdTRUE) {
      if (g_record) write_item(it);
      else          free(it.buf);
      vTaskDelay(1);
      continue;
    }

    if (!g_record_req && g_record) {
      g_record = false;
      close_files();
    }
  }
}

bool sd_init() {
  sd_ok = SD.begin(SD_CS, SPI, 20000000) && SD.cardType() != CARD_NONE;

  if (sd_ok) {
    SD.mkdir("/rec");
    Serial.println("SD card initialized");
    q = xQueueCreate(REC_QUEUE, sizeof(RecItem));
    xTaskCreatePinnedToCore(rec_task, "rec", 8192, NULL, 1, NULL, 0);
  } else {
    Serial.println("SD init failed");
  }

  g_record_req = 0;
  g_record = false;
  return sd_ok;
}

void record_service() {
  if (g_record_req && !sd_ok) {
    Serial.println("Recording start failed: no SD");
    g_record_req = 0;
  }
}

uint8_t *record_encode(camera_fb_t *fb, size_t *len) {
  if (!g_record_req || !g_record || !fb || !q) return NULL;

  uint8_t *copy = (uint8_t *)ps_malloc(fb->len);
  if (!copy) {
    dropped++;
    return NULL;
  }
  memcpy(copy, fb->buf, fb->len);
  *len = fb->len;
  s_w = fb->width;
  s_h = fb->height;
  s_fmt = fb->format;
  return copy;
}

void record_write(uint8_t *buf, size_t len) {
  if (!buf) return;

  RecItem it = {
    buf, len, s_w, s_h, s_fmt,
    esp_timer_get_time() / 1000,
    (int)lroundf(get_steering_deg()),
    g_dnn_deg,
    (int)get_throttle_pct()
  };

  if (!g_record_req || xQueueSend(q, &it, 0) != pdTRUE) {
    free(buf);
    dropped++;
  }
}