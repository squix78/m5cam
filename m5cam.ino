/**
 * Based on https://github.com/mic159/m5cam
 * 
 * Reads an image from the camera and servers it to a connected client.
 * Starts a webserver and starts streaming when a client connects
 * 
 * After the device has started change your computer's WiFi to m5cam 
 * and open the browser at http://192.168.4.1 for a stream
 * or http://192.168.4.1/camera for a still picture
 */
//#define CONFIG_ENABLE_TEST_PATTERN n
#define CONFIG_OV2640_SUPPORT y
//#define CONFIG_OV7725_SUPPORT y
#define CONFIG_ESP32_DEFAULT_CPU_FREQ_240 y
#define CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ 240
#define CONFIG_MEMMAP_SMP y
#define CONFIG_TASK_WDT n
#define CONFIG_LOG_DEFAULT_LEVEL_INFO y
#define CONFIG_LOG_DEFAULT_LEVEL 3
#define CONFIG_D0 17
#define CONFIG_D1 35
#define CONFIG_D2 34
#define CONFIG_D3 5
#define CONFIG_D4 39
#define CONFIG_D5 18
#define CONFIG_D6 36
#define CONFIG_D7 19
#define CONFIG_XCLK 27
#define CONFIG_PCLK 21
#define CONFIG_VSYNC 22
#define CONFIG_HREF 26
#define CONFIG_SDA 25
#define CONFIG_SCL 23
#define CONFIG_RESET 15
#define CONFIG_XCLK_FREQ 20000000
#define CAMERA_PIXEL_FORMAT CAMERA_PF_GRAYSCALE
#define CAMERA_FRAME_SIZE CAMERA_FS_SVGA
#define CAMERA_LED_GPIO 16


#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include "camera.h"

WiFiServer server(80);

void serve()
{
  WiFiClient client = server.available();
  if (client) 
  {
    //Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        //Serial.write(c);
        if (c == '\n') 
        {
          if (currentLine.length() == 0) 
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(
              "<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
            client.println();
            break;
          } 
          else 
          {
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {
          currentLine += c;
        }
        
        if(currentLine.endsWith("GET /camera"))
        {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/jpeg");
            client.println();

              esp_err_t err = camera_run();
              if (err != ESP_OK) {
                  ESP_LOGW(TAG, "Camera capture failed with error = %d", err);
                  return;
              }
            
              size_t frame_size = camera_get_data_size();
              uint8_t* fb = camera_get_fb();
            

              client.write(fb, frame_size);
              

        }
      }
    }
    // close the connection:
    client.stop();
    //Serial.println("Client Disconnected.");
  }  
}



static camera_pixelformat_t s_pixel_format;
bool video_running = false;

int udp_server = -1;
struct sockaddr_in destination;


void setup() {
  esp_log_level_set("camera", ESP_LOG_DEBUG);
  Serial.begin(115200);
  esp_err_t err;

  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  pinMode(CAMERA_LED_GPIO, OUTPUT);
  digitalWrite(CAMERA_LED_GPIO, HIGH);
  
  WiFi.disconnect(true);

  camera_config_t camera_config;
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = CONFIG_D0;
  camera_config.pin_d1 = CONFIG_D1;
  camera_config.pin_d2 = CONFIG_D2;
  camera_config.pin_d3 = CONFIG_D3;
  camera_config.pin_d4 = CONFIG_D4;
  camera_config.pin_d5 = CONFIG_D5;
  camera_config.pin_d6 = CONFIG_D6;
  camera_config.pin_d7 = CONFIG_D7;
  camera_config.pin_xclk = CONFIG_XCLK;
  camera_config.pin_pclk = CONFIG_PCLK;
  camera_config.pin_vsync = CONFIG_VSYNC;
  camera_config.pin_href = CONFIG_HREF;
  camera_config.pin_sscb_sda = CONFIG_SDA;
  camera_config.pin_sscb_scl = CONFIG_SCL;
  camera_config.pin_reset = CONFIG_RESET;
  camera_config.xclk_freq_hz = CONFIG_XCLK_FREQ;

  camera_model_t camera_model;
  err = camera_probe(&camera_config, &camera_model);
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "Camera probe failed with error 0x%x", err);
      return;
  }

  if (camera_model == CAMERA_OV7725) {
      s_pixel_format = CAMERA_PIXEL_FORMAT;
      camera_config.frame_size = CAMERA_FRAME_SIZE;
      ESP_LOGI(TAG, "Detected OV7725 camera, using %s bitmap format",
              CAMERA_PIXEL_FORMAT == CAMERA_PF_GRAYSCALE ?
                      "grayscale" : "RGB565");
  } else if (camera_model == CAMERA_OV2640) {
      ESP_LOGI(TAG, "Detected OV2640 camera, using JPEG format");
      s_pixel_format = CAMERA_PF_JPEG;
      camera_config.frame_size = CAMERA_FRAME_SIZE;
      camera_config.jpeg_quality = 15;
  } else {
      ESP_LOGE(TAG, "Camera not supported");
      return;
  }

  camera_config.pixel_format = s_pixel_format;
  err = camera_init(&camera_config);
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
      return;
  }

  ESP_LOGI("Starting WiFi AP m5cam");
  WiFi.softAP("m5cam");

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.begin();
}


void loop() {
  serve();
}
