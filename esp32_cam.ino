#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <WiFiManager.h>
#include <ESPmDNS.h>

// ================= CẤU HÌNH TELEGRAM =================
String BOT_TOKEN = "8893233580:AAFpQzmGJIU5vAabV41UkS1qTgNQhA0Jbms";
String CHAT_ID   = "5099971415";
// =============================================================

WebServer server(80);

// Khai báo chân cho module AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define FLASH_LED_PIN      4

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
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
    Serial.printf("Khoi tao camera that bai: 0x%x", err);
    return;
  }
}

String sendPhotoToTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  camera_fb_t * fb = NULL;

  fb = esp_camera_fb_get();
  
  if(!fb) {
    Serial.println("Loi chup anh");
    return "Loi camera";
  }

  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  Serial.println("Dang ket noi Telegram...");

  if (client_tcp.connect(myDomain, 443)) {
    String head = "--SmartHome\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--SmartHome\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--SmartHome--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t totalLen = head.length() + imageLen + tail.length();

    client_tcp.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
    client_tcp.println("Host: " + String(myDomain));
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=SmartHome");
    client_tcp.println();
    client_tcp.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client_tcp.write(fbBuf, remainder);
      }
    }  
    client_tcp.print(tail);
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) {
        char c = client_tcp.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r') getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    client_tcp.stop();
    Serial.println(getBody);
    return getBody;
  }
  else {
    esp_camera_fb_return(fb);
    return "Khong the ket noi Telegram";
  }
}

// ====================================================
// CÁC HÀM XỬ LÝ SERVER
// ====================================================

void handleRoot() {
  server.send(200, "text/plain", "He thong Camera An Ninh dang hoat dong o che do cho. San sang nhan lenh chup anh!");
}

void handleCapture() {
  server.send(200, "text/plain", "Da nhan lenh tu Main Board. Dang chup va gui anh...");
  Serial.println("Phat hien canh bao! Dang chup anh...");
  sendPhotoToTelegram();
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // --- BẮT ĐẦU CẤU HÌNH WIFI BẰNG WIFIMANAGER ---
  WiFiManager wm;

  Serial.println("Dang ket noi WiFi hoac mo cong truy cap WiFiManager...");
  
  bool res = wm.autoConnect("ESP32-CAM-Setup"); 

  if(!res) {
    Serial.println("Ket noi that bai! Dang khoi dong lai...");
    delay(3000);
    ESP.restart(); 
  } 
  
  Serial.println("\nWiFi ket noi thanh cong!");
  Serial.print("IP ESP32-CAM cua ban la: ");
  Serial.println(WiFi.localIP()); 

  // ==========================================================
  // KHỞI TẠO TÊN MIỀN MDNS CHO CAMERA
  // ==========================================================
  if (!MDNS.begin("camera-anninh")) { 
    Serial.println("[Loi] Khong the khoi tao mDNS!");
  } else {
    Serial.println("[Thanh cong] mDNS da chay!");
    Serial.println("Ten mien cua Camera la: http://camera-anninh.local");
  }
  // ==========================================================

  setupCamera();

  server.on("/", HTTP_GET, handleRoot);           
  server.on("/capture", HTTP_GET, handleCapture);
  server.begin();
  
  Serial.println("Web server da khoi dong. San sang hoat dong!");
}

void loop() {
  server.handleClient();
  delay(10);
}
