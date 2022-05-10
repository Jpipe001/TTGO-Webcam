/*
    Sketch from Arduino Example (modified) for TTGO Camera
    with SSD1306 OLED Display using U8x8lib.h Driver.
    Supporting Display Code: include app_httpd.cpp in the same libary and camera_index.h
    camera_pins.h included here in this code.
    U8x8lib.h Driver provided by olikraus. Thank you!
*/
//  IMPORTANT   Change ~ Pin definitions   TTGO
//  Board Selections:
//***   Board: "ESP32 Dev Module" or "ESP32 Wrover Module" or "ESP32 Wrover Kit"
//***   Upload Speed: "115200"     *** Double Check!!
//***   Flash Mode: "QIO"          *** Double Check!!
//***   Flash Size: "Default 4MB"  *** Double Check!!
//***   PSRAM: "Enabled"           *** Double Check!!
//***   Partition Scheme: "Default 4MB with spiffs" *** Double Check!!
//      COM Port: Depends          *** Double Check!!

// Include Required Libraries
#include "esp_camera.h"
#include <WiFi.h>
#include "ESPmDNS.h"
#include "U8x8lib.h"     //  for TTGO https://github.com/olikraus/U8g2_Arduino

// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size

// CONFIGURE CAMERA PINS
// Pin definitions for CAMERA_MODEL_TTGO_WHITE
#define CAMERA_MODEL_TTGO_WHITE_BME280
#define PWDN_GPIO_NUM     26
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     32
#define SIOD_GPIO_NUM     13
#define SIOC_GPIO_NUM     12
#define Y9_GPIO_NUM       39
#define Y8_GPIO_NUM       36
#define Y7_GPIO_NUM       23
#define Y6_GPIO_NUM       18
#define Y5_GPIO_NUM       15
#define Y4_GPIO_NUM       4
#define Y3_GPIO_NUM       14
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    27
#define HREF_GPIO_NUM     25
#define PCLK_GPIO_NUM     19

// CONFIGURE OLED DRIVER
// I2C OLED Display works with SSD1306 driver U8x8lib.h (text) for TTGO
#define OLED_SDA 21            //  for TTGO
#define OLED_SCL 22            //  for TTGO
#define OLED_RST -1            //  for TTGO
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ OLED_RST); // Unbuffered, basic graphics, software I2C

// COMFIGURE WiFi NETWORK
const char* ssid = "NETWORK NAME";
const char* password = "PASSWORD";
String local_hwaddr;                    // WiFi local hardware Address
String local_swaddr;                    // WiFi local software Address
const char* ServerName = "webcam";      // Logical Address http://webcam.local

void startCameraServer();

String Filename() {
  return String(__FILE__).substring(String(__FILE__).lastIndexOf("\\") + 1);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nProgram ~ " + Filename());
  Serial.printf("Starting ...\n");

  // Configure OLED Display
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  String Display_text;
  const char* Display;

  // Examples of Display Codes
  //u8x8.clearDisplay();                          // Clear the OLED display buffer.
  //u8x8.drawString(0, 0, "STARTING UP");         // Display on OLED  Normal Size
  //u8x8.draw2x2String(1, 1, "RUNNING");          // Display on OLED  Double Size
  //u8x8.setInverseFont(1);                       // Inverse mode 1= ON 0=OFF
  u8x8.setFlipMode(1);                          // Flip mode 1= ON 0=OFF

  // CONNECT to WiFi network:
  Serial.print("WiFi Connecting to " + String(ssid) + " ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf(" ~ Connected OK\n");

  // Print the Signal Strength:
  long rssi = WiFi.RSSI() + 100;
  Serial.print("Signal Strength = " + String(rssi));
  if (rssi > 50)  Serial.printf(" (>50 - Good)\n");  else   Serial.printf(" (Could be Better)\n");

  if (!MDNS.begin(ServerName)) {     // Start mDNS with ServerName
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);                   // Stay here
    }
  }
  local_hwaddr = WiFi.localIP().toString();
  Serial.println("MDNS started ~\tUse IP Address\t: " + local_hwaddr);
  local_swaddr = "http://" + String(ServerName) + ".local";
  Serial.println("\t\tOr Use Address\t: " + local_swaddr);

  // Display on the OLED
  u8x8.clearDisplay();                  // Clear the OLED display
  u8x8.drawString(1, 1, "WiFi CONNECTED");    // On OLED

  Display_text = "Strength = " + String(rssi);
  Display = Display_text.c_str();
  u8x8.drawString(1, 2, Display);       // On OLED

  Display_text = "Use " + local_hwaddr;
  Display = Display_text.c_str();
  u8x8.drawString(1, 3, Display);       // On OLED

  Display_text = "Or " + String(ServerName) + ".local";
  Display = Display_text.c_str();
  u8x8.drawString(1, 4, Display);       // On OLED

  // Configure Camera Settings
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Initial setting are FRAMESIZE_UXGA (320x240),  Reset to SVGA (800x600)
  // Initalize with high specs to pre-allocate larger buffers
  if (psramFound()) {
    Serial.printf(" ~ PSRAM Found OK! Frame Set to SVGA\n");
    config.frame_size = FRAMESIZE_SVGA;     // (800x600)
    config.jpeg_quality = 10; // 10-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    Serial.printf("NO PSRAM found, Check Board Configuration !! Frame Set to UXGA\n");
    config.frame_size = FRAMESIZE_UXGA;    // (320x240)
    config.jpeg_quality = 12;
    config.fb_count = 2;
  }

  // Initialize Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    if (err == 0x105)  Serial.printf("Check Camera Pins Configuration !!\n");
    Serial.printf("Camera init failed with error 0x%x", err);
    u8x8.drawString(1, 5, "Camera Failed");   // On OLED
    while (1)
      delay(1000);          // Stay here
  }

  sensor_t * s = esp_camera_sensor_get();
  // Initial sensors are flipped and colors are a bit saturated
  s->set_vflip(s, 1);       // Flip it Vert
  s->set_hmirror(s, 1);     // Flip it Horz
  s->set_brightness(s, 1);  // Increase the brightness just a bit
  s->set_saturation(s, -2); // Decrease the saturation

  Serial.print(" ~ Camera Set Up OK!\n");
  u8x8.drawString(1, 5, "Camera Ready");   // On OLED

  startCameraServer();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
  u8x8.clearDisplay();                  // Clear the OLED display
}
