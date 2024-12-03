#include "esp_camera.h"
#include "tensor_flow_lite.h"
#include "model.h" // include the TensorFlow Lite model file
#include "esp_log.h"

const char* TAG = "TFLITE";

// Function to initialize the camera
void init_camera() {
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
  config.pixel_format = PIXFORMAT_JPEG; 

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera Init Failed");
  }
}

// Function to capture an image and classify it using TensorFlow Lite
void capture_and_classify() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return;
  }

  // Process the image and run inference
  uint8_t* input_data = tflite_get_input_buffer();
  memcpy(input_data, fb->buf, fb->len);

  // Perform the inference
  TfLiteStatus invoke_status = tflite_invoke();

  if (invoke_status != kTfLiteOk) {
    ESP_LOGE(TAG, "Invoke failed");
  }

  // Get the output from the model
  int8_t* output_data = tflite_get_output_buffer();
  ESP_LOGI(TAG, "Classification Result: %d", output_data[0]);

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  init_camera();
  tflite_setup();
}

void loop() {
  capture_and_classify();
  delay(10000); // Capture and classify every 10 seconds
}