#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char *ssid     = "wifiname";
const char *password = "password";

void startCameraServer();
void setupLedFlash();

// ─── Sensor & Motor pins ───────────────────────────────────────────────────
const int IR_PIN_L    = 12;
const int IR_PIN_R    = 13;
const int MOTOR_PIN_L = 14;
const int MOTOR_PIN_R = 15;

const int PWM_FREQ       = 1000;
const int MOTOR_STRENGTH = 200;

const unsigned long PULSE_PERIOD = 150;

unsigned long prevTimeL = 0;
unsigned long prevTimeR = 0;
bool stateL = false;
bool stateR = false;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel  = LEDC_CHANNEL_0;
  config.ledc_timer    = LEDC_TIMER_0;
  config.pin_d0        = Y2_GPIO_NUM;
  config.pin_d1        = Y3_GPIO_NUM;
  config.pin_d2        = Y4_GPIO_NUM;
  config.pin_d3        = Y5_GPIO_NUM;
  config.pin_d4        = Y6_GPIO_NUM;
  config.pin_d5        = Y7_GPIO_NUM;
  config.pin_d6        = Y8_GPIO_NUM;
  config.pin_d7        = Y9_GPIO_NUM;
  config.pin_xclk      = XCLK_GPIO_NUM;
  config.pin_pclk      = PCLK_GPIO_NUM;
  config.pin_vsync     = VSYNC_GPIO_NUM;
  config.pin_href      = HREF_GPIO_NUM;
  config.pin_sccb_sda  = SIOD_GPIO_NUM;
  config.pin_sccb_scl  = SIOC_GPIO_NUM;
  config.pin_pwdn      = PWDN_GPIO_NUM;
  config.pin_reset     = RESET_GPIO_NUM;
  config.xclk_freq_hz  = 20000000;
  config.frame_size    = FRAMESIZE_QVGA;
  config.pixel_format  = PIXFORMAT_GRAYSCALE;
  config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location   = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality  = 12;
  config.fb_count      = 1;

  if (config.pixel_format != PIXFORMAT_JPEG) {
    config.frame_size = FRAMESIZE_240X240;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  pinMode(IR_PIN_L, INPUT);
  pinMode(IR_PIN_R, INPUT);

  ledcAttach(MOTOR_PIN_L, PWM_FREQ, 8);
  ledcAttach(MOTOR_PIN_R, PWM_FREQ, 8);
  ledcWrite(MOTOR_PIN_L, 0);
  ledcWrite(MOTOR_PIN_R, 0);

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  unsigned long now = millis();

  // TCRT5000 output is active LOW: LOW = obstacle detected
  bool obstacleL = (digitalRead(IR_PIN_L) == LOW);
  bool obstacleR = (digitalRead(IR_PIN_R) == LOW);

  // ── Left motor ──────────────────────────────────────────────────────────
  if (obstacleL) {
    if (now - prevTimeL >= PULSE_PERIOD) {
      prevTimeL = now;
      stateL = !stateL;
      ledcWrite(MOTOR_PIN_L, stateL ? MOTOR_STRENGTH : 0);
    }
  } else {
    ledcWrite(MOTOR_PIN_L, 0);
    stateL    = false;
    prevTimeL = now;
  }

  // ── Right motor ─────────────────────────────────────────────────────────
  if (obstacleR) {
    if (now - prevTimeR >= PULSE_PERIOD) {
      prevTimeR = now;
      stateR = !stateR;
      ledcWrite(MOTOR_PIN_R, stateR ? MOTOR_STRENGTH : 0);
    }
  } else {
    ledcWrite(MOTOR_PIN_R, 0);
    stateR    = false;
    prevTimeR = now;
  }

  delay(5);
}
