#include <esp_camera.h>
#include <Arduino.h>

#define CAMERA_MODEL_XIAO_ESP32S3
#include "camera_pins.h"

#define RX_PIN D7
#define TX_PIN D6

const uint8_t END_MARKER[] = { 0xFF, 0xD9, 0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF }; // Beispiel
const size_t END_MARKER_LEN = sizeof(END_MARKER);


void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG;  // for streaming
    //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    //                      for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound()) {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_QVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    } else {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
#endif
    }

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);        // flip it back
        s->set_brightness(s, 1);   // up the brightness just a bit
        s->set_saturation(s, -2);  // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG) {
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
#endif

}

void loop() {
    if (Serial1.available() > 0) {
        String cmd = Serial1.readStringUntil('\n');
        cmd.trim(); // Entfernt führende und nachfolgende Leerzeichen
        Serial.println("Command received: " + cmd);
        if (strcmp(cmd.c_str(), "capture") == 0) {
            // Foto aufnehmen
            //Serial.println("Capture command received, taking picture...");
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                Serial.println("image was taken, size: " + String(fb->len) + " bytes");

                if (false) {
                    Serial.println("raw image data:");
                    for (size_t i = 0; i < fb->len; i++) {
                        if (fb->buf[i] < 16) {
                            Serial.print("0"); // Füge führende Null hinzu, wenn nötig
                        }
                        Serial.print(fb->buf[i], HEX);
                        Serial.print(" ");
                    }
                    Serial.println();
                }

                // Senden des Bildes
                for (size_t i = 0; i < fb->len; i++) {
                    Serial1.write(fb->buf[i]);
                }

                // Senden des Endmarkers
                for (size_t i = 0; i < END_MARKER_LEN; i++) {
                    Serial1.write(END_MARKER[i]);
                }
                Serial1.flush();
                esp_camera_fb_return(fb);
            }

            Serial.println("Command finished, picture taken and sent!");
        }
    }
}