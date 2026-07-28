/*
 * ============================================================
 * SMART HOME - MAIN (OPTIMIZED VERSION)
 * ============================================================
 */

#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"
#include <Preferences.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
#include <I2CKeyPad.h> 
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <BH1750.h>

const char *service_name = "NHATHONGMINH";
const char *pop          = "1234abcd";
const char *ota_hostname = "SmartHome-Main";
const char *ota_password = "smarthome123";
bool        otaReady     = false;

// ==============================================================================
// KHAI BÁO CHÂN GPIO
// ==============================================================================
#define BUTTON_PIN        0     
#define SUB_TX_PIN        17
#define SUB_RX_PIN        18
#define SUB_BAUD          115200

#define DHTPIN            10
#define DHTTYPE           DHT22
#define FLAME_SENSOR_PIN  4     
#define GAS_SENSOR_PIN    5     
#define GAS_ANALOG_PIN    3   
  
#define RAIN_SENSOR_PIN   12     
#define CLOTHES_SERVO_PIN 7     

#define OLED_SDA_PIN      8
#define OLED_SCL_PIN      9
#define OLED_I2C_ADDR     0x3C
#define OLED_WIDTH        128
#define OLED_HEIGHT       64

#define BUZZER_PIN        41
#define DOOR_SENSOR_PIN   40    
#define TOUCH_SENSOR_PIN  11    

#define RST_PIN           21
#define SS_PIN            14
#define SCK_PIN           48
#define MISO_PIN          13
#define MOSI_PIN          47
#define SERVO_PIN         15    
#define KEYPAD_I2C_ADDR   0x20

// ==============================================================================
// KHAI BÁO RÈM CỬA
// ==============================================================================
#define STEP_IN1 1  
#define STEP_IN2 2  
#define STEP_IN3 6  
#define STEP_IN4 16 

const int stepMatrix[8][4] = {
  {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0},
  {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 0, 0, 1}, {1, 0, 0, 1}
};
long so_buoc_mo_rem = 9000; 
volatile long current_step_pos = 0; 
volatile long target_step_pos  = 0; 
int stepDelay = 3;
TaskHandle_t StepperTask;

// ==============================================================================
// BIẾN TOÀN CỤC
// ==============================================================================
Preferences prefs;
volatile bool wifiJustConnected  = false;
volatile bool wifiDisconnected   = false;
bool          wifiProvDone       = false;
String esp32CamHost = "camera-anninh.local"; 

MFRC522 mfrc522(SS_PIN, RST_PIN);
unsigned long lastRFIDReset = 0;

#define MAX_CARDS 5
byte validCards[MAX_CARDS][4];
int numCards = 0;
bool quanLyTheMode = false;
bool doiMatKhauMode = false;
bool isArmed = true; // Chế độ an ninh (Mặc định bật)

BH1750 bh1750_trong(0x23); 
BH1750 bh1750_ngoai(0x5C);
int lux_trong = 0;
int lux_ngoai = 0;

int sensorSendState = 0; 
unsigned long lastSensorSendTick = 0;

// OLED Non-blocking variables
unsigned long oledMessageTimer = 0;
#define OLED_MSG_DURATION 2000

// --- THÔNG SỐ TỰ ĐỘNG ĐIỀU CHỈNH ĐÈN  ---
int target_lux = 300;           
#define LUX_TOLERANCE       20   
#define PWM_STEP            15   
#define MIN_PWM_DN          10   
#define MAX_PWM_DN          255  

int current_pwm_dn = 255;     
unsigned long lastAutoLightTime = 0;

// ==============================================================================
// HÀM ĐIỀU KHIỂN SERVO CỬA
// ==============================================================================
#define CLOSED_POS          0
#define OPEN_POS            90
#define CLOTHES_OUT_POS     0   
#define CLOTHES_IN_POS      130 

#define SERVO_FREQ          50 
#define SERVO_RES           14 

bool isDoorOpen = false;
unsigned long doorClosingGraceUntil = 0;         
const unsigned long DOOR_CLOSE_GRACE_MS = 1200;  
bool isRaining = false;

void setServoAngle(uint8_t pin, int angle) {
  int duty = map(angle, 0, 180, 410, 1966);
  ledcWrite(pin, duty);
  Serial.printf("[LOG] Da quay Servo chan %d goc %d do\n", pin, angle);
}

// ==============================================================================
// CÁC BIẾN PHỤ TRỢ KHÁC
// ==============================================================================
const char keypadMap[17] = "123N456N789N*0#N";
I2CKeyPad kpd(KEYPAD_I2C_ADDR, &Wire);
String matKhauDung   = "1234";
String inputBuffer   = "";
bool dangNhapMatKhau        = false;
unsigned long lastKeyTime   = 0;
#define KEYPAD_TIMEOUT_MS   5000UL

bool intrusionDetected      = false;
bool forcedEntryAlarm       = false;
unsigned long forcedEntryAlarmStart = 0;
#define FORCED_ALARM_DURATION  15000UL
bool simulatedIntrusion            = false; // Giả lập đột nhập từ nút nhấn trên App
unsigned long simulatedIntrusionStart = 0;
#define SIMULATED_ALARM_DURATION  15000UL
bool buzzerBlinkState       = false; 
bool fireDetected           = false;
bool gasDetected            = false;
int currentGasLevel         = 0; 
int currentSoilMoisture     = 0;
bool state_may_bom          = false;

static Switch den_khach ("Den Phong Khach");
static Switch den_ngu   ("Den Phong Ngu");
static Switch den_bep   ("Den Bep");
static Switch den_wc    ("Den WC");
static Switch quat_khach("Quat Phong Khach");
static Switch quat_ngu  ("Quat Phong Ngu");
static Switch quat_bep  ("Quat Bep");
static Switch may_bom   ("May Bom"); 
static Switch rem_cua   ("Rem Cua");
static Device cam_bien_moi_truong("Moi Truong", "esp.device.sensor");
static Device thiet_bi_an_ninh("An Ninh", "esp.device.sensor");     
static Device thiet_bi_gian_phoi("Gian Phoi", "esp.device.sensor"); 

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

bool state_den_khach  = false; 
bool state_den_ngu    = false;
bool state_den_bep    = false;
bool state_den_wc     = false;
bool state_quat_khach = false;
bool state_quat_ngu   = false;
bool state_quat_bep   = false;
bool state_rem_cua    = false;

// ==============================================================================
// GIAO TIẾP VỚI SUB BOARD (UART)
// ==============================================================================
void sendDeviceCmd(const char *name, bool state) {
  Serial1.printf("CMD:%s:%d\n", name, state ? 1 : 0);
}

void syncAllDevices() {
  sendDeviceCmd("DK", state_den_khach); sendDeviceCmd("DN", state_den_ngu);
  sendDeviceCmd("DB", state_den_bep);   sendDeviceCmd("DW", state_den_wc);
  sendDeviceCmd("QK", state_quat_khach);sendDeviceCmd("QN", state_quat_ngu);
  sendDeviceCmd("QB", state_quat_bep);  sendDeviceCmd("MB", state_may_bom);
  Serial.println(F("[LOG][UART] Da gui lenh dong bo tat ca thiet bi sang Sub."));
}

void capNhatTuSub(const String &maDevice, int value) {
  bool state = (value == 1);
  if (maDevice == "DN") { 
    state_den_ngu = state; 
    den_ngu.updateAndReportParam("Power", state); 
    prefs.putBool("den_khach", state);
    if (state) current_pwm_dn = MAX_PWM_DN;
    else current_pwm_dn = 0;
  }
  else if (maDevice == "DK") { state_den_khach = state; den_khach.updateAndReportParam("Power", state); prefs.putBool("den_khach", state); }
  else if (maDevice == "DB") { state_den_bep = state; den_bep.updateAndReportParam("Power", state); prefs.putBool("den_bep", state); }
  else if (maDevice == "DW") { state_den_wc = state; den_wc.updateAndReportParam("Power", state); prefs.putBool("den_wc", state); }
  else if (maDevice == "QK") { state_quat_khach = state; quat_khach.updateAndReportParam("Power", state); prefs.putBool("quat_khach", state); }
  else if (maDevice == "QN") { state_quat_ngu = state; quat_ngu.updateAndReportParam("Power", state); prefs.putBool("quat_ngu", state); }
  else if (maDevice == "QB") { state_quat_bep = state; quat_bep.updateAndReportParam("Power", state); prefs.putBool("quat_bep", state); }
  else if (maDevice == "MB") { state_may_bom = state; may_bom.updateAndReportParam("Power", state); prefs.putBool("may_bom", state); }
  else if (maDevice == "SOIL") { currentSoilMoisture = value; cam_bien_moi_truong.updateAndReportParam("Do Am Dat", currentSoilMoisture); }
}

void handleSubSerial() {
  while (Serial1.available()) {
    String line = Serial1.readStringUntil('\n'); 
    line.trim();
    if (line.startsWith("EVT:VAL:")) {
      String phanSau = line.substring(8); int colonPos = phanSau.indexOf(':');
      if (colonPos > 0) capNhatTuSub(phanSau.substring(0, colonPos), phanSau.substring(colonPos + 1).toInt());
    } else if (line == "EVT:READY") {
      syncAllDevices();
    }
  }
}

// ==============================================================================
// HỆ THỐNG TỰ ĐỘNG ĐIỀU CHỈNH ĐỘ SÁNG LED & BÙ SÁNG BẰNG RÈM CỬA
// ==============================================================================
void xuLyTuDongAnhSang() {
  if (!state_den_ngu) return;

  lux_trong = (int)bh1750_trong.readLightLevel();
  if (lux_trong < 0) return; 

  bool canCapNhatPwm = false;

  if (lux_trong < (target_lux - LUX_TOLERANCE)) {
    if (current_pwm_dn < MAX_PWM_DN) {
      current_pwm_dn += PWM_STEP;
      if (current_pwm_dn > MAX_PWM_DN) current_pwm_dn = MAX_PWM_DN;
      canCapNhatPwm = true;
    } 
    if (current_pwm_dn >= MAX_PWM_DN && !state_rem_cua) {
      state_rem_cua = true;
      target_step_pos = so_buoc_mo_rem; 
      rem_cua.updateAndReportParam("Power", true);
      prefs.putBool("rem_cua", true);
    }
  }
  else if (lux_trong > (target_lux + LUX_TOLERANCE)) {
    if (current_pwm_dn > MIN_PWM_DN) {
      current_pwm_dn -= PWM_STEP;
      if (current_pwm_dn < MIN_PWM_DN) current_pwm_dn = MIN_PWM_DN;
      canCapNhatPwm = true;
    }
    else if (state_rem_cua) {
      state_rem_cua = false;
      target_step_pos = 0; 
      rem_cua.updateAndReportParam("Power", false);
      prefs.putBool("rem_cua", false);
    }
  }
  
  if (canCapNhatPwm) {
    Serial1.printf("CMD:DN_PWM:%d\n", current_pwm_dn);
  }
}

// ==============================================================================
// HỆ THỐNG AN NINH & LOGIC CỬA
// ==============================================================================
void moCua() { 
  if (!isDoorOpen) { 
    setServoAngle(SERVO_PIN, OPEN_POS); 
    isDoorOpen = true;
  } 
  forcedEntryAlarm = false;
}

void dongCua() {
  if (isDoorOpen) {
    setServoAngle(SERVO_PIN, CLOSED_POS);
    isDoorOpen = false;
    doorClosingGraceUntil = millis() + DOOR_CLOSE_GRACE_MS; 
  }
}

void toggleCua() {
  if (isDoorOpen) dongCua();
  else moCua();
}

void xuLyTruyCapSai(const char *nguon) { 
  Serial.printf("[LOG] Canh bao! Truy cap sai tu: %s\n", nguon);
  if (digitalRead(DOOR_SENSOR_PIN) == HIGH && isArmed) { 
    forcedEntryAlarm = true; 
    forcedEntryAlarmStart = millis();
  } 
}

// FIX 1: Non-blocking OLED Message
void hienThiKetQuaMatKhau(bool dungMatKhau) {
  oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.setTextSize(2);
  if (dungMatKhau) { 
    oled.setCursor(20, 10); oled.println("DUNG!"); oled.setTextSize(1); oled.setCursor(22, 40);
    oled.println("Dang xu ly...");
  } else { 
    oled.setCursor(28, 10); oled.println("SAI!"); oled.setTextSize(1); oled.setCursor(12, 40); oled.println("Vui long thu lai"); 
  }
  oled.display();
  oledMessageTimer = millis(); // Đánh dấu thời điểm bắt đầu hiện thông báo
}

void hienThiNhapMatKhau() {
  oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.setTextSize(1); oled.setCursor(0, 0);
  if(doiMatKhauMode) oled.println("NHAP MAT KHAU MOI:");
  else oled.println("NHAP MAT KHAU:");
  oled.drawLine(0, 10, 128, 10, SSD1306_WHITE); oled.setTextSize(2);
  String masked = "";
  for (unsigned int i = 0; i < inputBuffer.length(); i++) masked += "*";
  int startX = max(0, (128 - (int)(masked.length() * 12)) / 2); oled.setCursor(startX, 20); oled.print(masked);
  oled.setTextSize(1); oled.setCursor(0, 48);
  oled.print("# Xac nhan  * Xoa"); oled.display();
}

void xuLyRFID() {
  if (millis() - lastRFIDReset > 300000UL) { 
    mfrc522.PCD_Reset(); mfrc522.PCD_Init();
    lastRFIDReset = millis(); 
  }
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    if (quanLyTheMode) {
      int foundIdx = -1;
      for (int i = 0; i < numCards; i++) {
        bool match = true;
        for (int j = 0; j < 4; j++) if (validCards[i][j] != mfrc522.uid.uidByte[j]) match = false;
        if (match) { foundIdx = i; break; }
      }
      if (foundIdx >= 0) {
        for (int i = foundIdx; i < numCards - 1; i++) {
          for (int j = 0; j < 4; j++) validCards[i][j] = validCards[i+1][j];
        }
        numCards--;
      } else if (numCards < MAX_CARDS) {
          for (int j = 0; j < 4; j++) validCards[numCards][j] = mfrc522.uid.uidByte[j];
          numCards++;
      }
      prefs.putBytes("rfid_cards", validCards, sizeof(validCards));
      prefs.putInt("num_cards", numCards);
      quanLyTheMode = false;
      thiet_bi_an_ninh.updateAndReportParam("Quan Ly The", false);
      hienThiKetQuaMatKhau(true); 
    } else {
      bool dungThe = false;
      for (int i = 0; i < numCards; i++) {
        bool match = true;
        for (int j = 0; j < 4; j++) if (validCards[i][j] != mfrc522.uid.uidByte[j]) match = false;
        if (match) { dungThe = true; break; }
      }
      hienThiKetQuaMatKhau(dungThe);
      if (dungThe) toggleCua(); 
      else xuLyTruyCapSai("The RFID");
    }
    mfrc522.PICC_HaltA(); mfrc522.PCD_StopCrypto1(); yield();
  }
}

void xuLyKeypad() {
  static uint8_t lastIdx = I2C_KEYPAD_NOKEY; uint8_t idx = kpd.getKey();
  if (idx >= I2C_KEYPAD_NOKEY || idx == lastIdx) { lastIdx = idx; return; }
  lastIdx = idx;
  char key = keypadMap[idx]; if (key == 'N') return;
  
  lastKeyTime = millis();
  dangNhapMatKhau = true;
  if (key == '#') {
    if (doiMatKhauMode) {
      if (inputBuffer.length() > 0) {
        matKhauDung = inputBuffer; prefs.putString("mat_khau", matKhauDung);
        doiMatKhauMode = false; thiet_bi_an_ninh.updateAndReportParam("Doi Mat Khau", false);
        hienThiKetQuaMatKhau(true);
      }
    } else {
      if (inputBuffer.length() == 0 && isDoorOpen) {
        dongCua();
      } else {
        bool dungMatKhau = (inputBuffer == matKhauDung); 
        hienThiKetQuaMatKhau(dungMatKhau);
        if (dungMatKhau) moCua();
        else xuLyTruyCapSai("Mat khau");
      }
    }
    inputBuffer = ""; dangNhapMatKhau = false;
  } else if (key == '*') { 
    inputBuffer = ""; hienThiNhapMatKhau();
  } else { 
    inputBuffer += key; 
    if (inputBuffer.length() > 8) inputBuffer = ""; 
    hienThiNhapMatKhau();
  }
}

void xuLyTouchTTP223() {
  static bool trangThaiCu = LOW;
  bool trangThaiMoi = digitalRead(TOUCH_SENSOR_PIN);
  if (trangThaiMoi == HIGH && trangThaiCu == LOW) {
    toggleCua();
    delay(200); 
  }
  trangThaiCu = trangThaiMoi;
}

void taskChupAnhCamera(void *pvParameters) {
  if (WiFi.status() == WL_CONNECTED) { 
    HTTPClient http;
    http.begin("http://" + esp32CamHost + "/capture"); 
    http.setTimeout(3000); http.GET(); http.end();
  }
  vTaskDelete(NULL);
}

// ==============================================================================
// NÚT NHẤN GIẢ LẬP ĐỘT NHẬP (Điều khiển từ App RainMaker)
// ==============================================================================
void kichHoatGiaLapDotNhap() {
  Serial.println(F("[LOG] Da kich hoat GIA LAP DOT NHAP tu App!"));
  simulatedIntrusion = true;
  simulatedIntrusionStart = millis();
  thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "GIA LAP DOT NHAP");
  // Kích hoạt camera chụp ảnh ngay lập tức (không chặn luồng chính)
  xTaskCreate(taskChupAnhCamera, "CameraTaskSim", 4096, NULL, 1, NULL);
  // Trả nút về trạng thái OFF ngay để App hiển thị như một nút nhấn (không phải công tắc giữ trạng thái)
  thiet_bi_an_ninh.updateAndReportParam("Gia Lap Dot Nhap", false);
}

// FIX 2 & 3: Oversampling Gas & Chế độ Armed/Disarmed
int readGasAnalogOversampled() {
  long sum = 0;
  for(int i = 0; i < 16; i++) {
    sum += analogRead(GAS_ANALOG_PIN);
  }
  return (int)(sum >> 4); // Chia 16 lấy trung bình
}

void xuLyCacCamBienAnNinh() {
  bool doorSensorOpen = (digitalRead(DOOR_SENSOR_PIN) == HIGH);
  static bool lastRawDoor = false; static bool stableDoorState = false;
  static unsigned long doorChangeTime = 0;
  if (doorSensorOpen != lastRawDoor) { lastRawDoor = doorSensorOpen; doorChangeTime = millis(); }
  if (millis() - doorChangeTime >= 300UL) stableDoorState = doorSensorOpen;

  static bool lastIntrusionState = false;
  // CHỈ BÁO ĐỘT NHẬP KHI ĐANG Ở CHẾ ĐỘ ARMED
  intrusionDetected = (stableDoorState && !isDoorOpen && millis() > doorClosingGraceUntil && isArmed);
  
  if (intrusionDetected != lastIntrusionState) {
    lastIntrusionState = intrusionDetected;
    if (intrusionDetected) {
      thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "DOT NHAP");
      xTaskCreate(taskChupAnhCamera, "CameraTask", 4096, NULL, 1, NULL);
    } else if (!forcedEntryAlarm) thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "Binh thuong");
  }

  static bool lastForcedAlarm = false;
  if (forcedEntryAlarm && (millis() - forcedEntryAlarmStart > FORCED_ALARM_DURATION)) forcedEntryAlarm = false;
  if (forcedEntryAlarm != lastForcedAlarm) {
    lastForcedAlarm = forcedEntryAlarm;
    if (forcedEntryAlarm) thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "NGHI VAN SAT NHAP");
    else if (!intrusionDetected) thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "Binh thuong");
  }

  bool currentFire = (digitalRead(FLAME_SENSOR_PIN) == LOW);
  if (currentFire != fireDetected) { 
    fireDetected = currentFire;
    cam_bien_moi_truong.updateAndReportParam("Bao Chay", fireDetected ? "CO LUA" : "An toan");
  }

  bool currentGas = (digitalRead(GAS_SENSOR_PIN) == LOW); 
  currentGasLevel = readGasAnalogOversampled(); // Dùng hàm mới có oversampling
  if (currentGas != gasDetected) { 
    gasDetected = currentGas;
    cam_bien_moi_truong.updateAndReportParam("Ro ri Gas", gasDetected ? "CO GAS" : "An toan");
  }

  static bool lastSimulatedAlarm = false;
  if (simulatedIntrusion && (millis() - simulatedIntrusionStart > SIMULATED_ALARM_DURATION)) simulatedIntrusion = false;
  if (simulatedIntrusion != lastSimulatedAlarm) {
    lastSimulatedAlarm = simulatedIntrusion;
    if (!simulatedIntrusion && !intrusionDetected && !forcedEntryAlarm) {
      thiet_bi_an_ninh.updateAndReportParam("Trang Thai An Ninh", "Binh thuong");
    }
  }

  bool canBaoDong = intrusionDetected || forcedEntryAlarm || fireDetected || gasDetected || simulatedIntrusion;
  static unsigned long lastBlinkTime = 0;
  if (canBaoDong) {
    if (millis() - lastBlinkTime > ((forcedEntryAlarm || fireDetected || gasDetected || simulatedIntrusion) ? 150UL : 400UL)) {
      lastBlinkTime = millis();
      buzzerBlinkState = !buzzerBlinkState;
      digitalWrite(BUZZER_PIN, buzzerBlinkState);
    }
  } else { 
    buzzerBlinkState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void xuLyThuDoTuDong() {
  bool currentRain = (digitalRead(RAIN_SENSOR_PIN) == LOW);
  if (currentRain != isRaining) {
    isRaining = currentRain;
    if (isRaining) { 
      setServoAngle(CLOTHES_SERVO_PIN, CLOTHES_IN_POS);
      thiet_bi_gian_phoi.updateAndReportParam("Thoi Tiet", "CO MUA - Da thu do");
    } else { 
      setServoAngle(CLOTHES_SERVO_PIN, CLOTHES_OUT_POS);
      thiet_bi_gian_phoi.updateAndReportParam("Thoi Tiet", "Khong Mua - Dang phoi");
    }
  }
}

float lastTemp = NAN; float lastHumid = NAN;
void docDHTVaHienThiLCD() {
  // Nếu đang nhập pass hoặc đang hiện kết quả pass (2 giây), không ghi đè LCD
  if (dangNhapMatKhau || doiMatKhauMode || (millis() - oledMessageTimer < OLED_MSG_DURATION)) return;
  
  float t = dht.readTemperature(); float h = dht.readHumidity();
  bool tValid = (!isnan(t) && t > -40.0f && t < 80.0f);
  bool hValid = (!isnan(h) && h >= 0.0f  && h <= 100.0f);
  
  oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE);
  if (tValid && hValid) {
    lastTemp = t; lastHumid = h;
    oled.setTextSize(1); oled.setCursor(0, 0); oled.println("SMART HOME");
    oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    oled.setTextSize(2); oled.setCursor(0, 18); oled.print(t, 1); oled.print((char)247); oled.println("C");
    oled.setCursor(0, 42); oled.print(h, 1); oled.println(" %");
  } else { 
    oled.setTextSize(1); oled.setCursor(0, 0); oled.println("Loi doc DHT!"); 
  }
  oled.display();
}

void guiSensorLenRainMaker() {
  if (isnan(lastTemp) || isnan(lastHumid)) return;
  lux_trong = (int)bh1750_trong.readLightLevel();
  lux_ngoai = (int)bh1750_ngoai.readLightLevel();
  sensorSendState = 1;
  lastSensorSendTick = millis();
}

void xuLyGuiSensorNonBlocking() {
  if (sensorSendState == 0) return; 
  if (millis() - lastSensorSendTick > 200UL) {
    lastSensorSendTick = millis();
    switch (sensorSendState) {
      case 1: cam_bien_moi_truong.updateAndReportParam("Nhiet Do", lastTemp); sensorSendState++; break;
      case 2: cam_bien_moi_truong.updateAndReportParam("Do Am", lastHumid); sensorSendState++; break;
      case 3: cam_bien_moi_truong.updateAndReportParam("Nong do Gas", currentGasLevel); sensorSendState++; break;
      case 4: if (lux_trong >= 0) cam_bien_moi_truong.updateAndReportParam("Anh Sang Trong", lux_trong); sensorSendState++; break;
      case 5: if (lux_ngoai >= 0) cam_bien_moi_truong.updateAndReportParam("Anh Sang Ngoai", lux_ngoai); sensorSendState = 0; break;
    }
  }
}

// ==============================================================================
// CALLBACK NHẬN SỰ KIỆN ĐIỀU KHIỂN TỪ APP RAINMAKER 
// ==============================================================================
void write_callback(Device *device, Param *param, const param_val_t val, void *priv_data, write_ctx_t *ctx) {
  const char *device_name = device->getDeviceName();
  const char *param_name  = param->getParamName();
  bool s = val.val.b;

  if (strcmp(device_name, "Moi Truong") == 0) {
    if (strcmp(param_name, "Nguong Sang") == 0) {
      target_lux = val.val.i; prefs.putInt("target_lux", target_lux);
    }
  }
  else if (strcmp(device_name, "An Ninh") == 0) {
    if (strcmp(param_name, "Quan Ly The") == 0) quanLyTheMode = s;
    else if (strcmp(param_name, "Doi Mat Khau") == 0) {
      doiMatKhauMode = s; if (doiMatKhauMode) { inputBuffer = ""; hienThiNhapMatKhau(); }
    }
    else if (strcmp(param_name, "Che Do An Ninh") == 0) {
      isArmed = s; // Cập nhật trạng thái Armed/Disarmed
      Serial.printf("[LOG] An Ninh -> %s\n", isArmed ? "DA BAT (ARMED)" : "DA TAT (DISARMED)");
    }
    else if (strcmp(param_name, "Gia Lap Dot Nhap") == 0) {
      if (s) kichHoatGiaLapDotNhap(); // Nhấn nút trên App -> chuông kêu + camera chụp
    }
  }
  else if (strcmp(param_name, "Power") == 0) {
    if (strcmp(device_name, "Den Phong Ngu") == 0) { 
      state_den_ngu = s; current_pwm_dn = s ? MAX_PWM_DN : 0;
      sendDeviceCmd("DN", s); prefs.putBool("den_ngu", s); 
    }
    else if (strcmp(device_name, "Den Phong Khach") == 0) { state_den_khach = s; sendDeviceCmd("DK", s); prefs.putBool("den_khach", s); }
    else if (strcmp(device_name, "Den Bep") == 0) { state_den_bep = s; sendDeviceCmd("DB", s); prefs.putBool("den_bep", s); }
    else if (strcmp(device_name, "Den WC") == 0) { state_den_wc = s; sendDeviceCmd("DW", s); prefs.putBool("den_wc", s); }
    else if (strcmp(device_name, "Quat Phong Khach") == 0) { state_quat_khach = s; sendDeviceCmd("QK", s); prefs.putBool("quat_khach", s); }
    else if (strcmp(device_name, "Quat Phong Ngu") == 0) { state_quat_ngu = s; sendDeviceCmd("QN", s); prefs.putBool("quat_ngu", s); }
    else if (strcmp(device_name, "Quat Bep") == 0) { state_quat_bep = s; sendDeviceCmd("QB", s); prefs.putBool("quat_bep", s); }
    else if (strcmp(device_name, "May Bom") == 0) { state_may_bom = s; sendDeviceCmd("MB", s); prefs.putBool("may_bom", s); } 
    else if (strcmp(device_name, "Rem Cua") == 0) { 
      state_rem_cua = s; prefs.putBool("rem_cua", s); 
      target_step_pos = s ? so_buoc_mo_rem : 0;             
    }
  }
  param->updateAndReport(val);
}

void stepperCode(void * pvParameters) {
  int step_idx = 0;
  for(;;) {
    if (current_step_pos < target_step_pos) {
      current_step_pos++; step_idx = (step_idx + 1) % 8;
      digitalWrite(STEP_IN1, stepMatrix[step_idx][0]); digitalWrite(STEP_IN2, stepMatrix[step_idx][1]);
      digitalWrite(STEP_IN3, stepMatrix[step_idx][2]); digitalWrite(STEP_IN4, stepMatrix[step_idx][3]);
      vTaskDelay(pdMS_TO_TICKS(stepDelay));
    } 
    else if (current_step_pos > target_step_pos) {
      current_step_pos--; step_idx = (step_idx - 1 + 8) % 8;
      digitalWrite(STEP_IN1, stepMatrix[step_idx][0]); digitalWrite(STEP_IN2, stepMatrix[step_idx][1]);
      digitalWrite(STEP_IN3, stepMatrix[step_idx][2]); digitalWrite(STEP_IN4, stepMatrix[step_idx][3]);
      vTaskDelay(pdMS_TO_TICKS(stepDelay));
    } 
    else {
      digitalWrite(STEP_IN1, LOW); digitalWrite(STEP_IN2, LOW);
      digitalWrite(STEP_IN3, LOW); digitalWrite(STEP_IN4, LOW);
      vTaskDelay(pdMS_TO_TICKS(100)); 
    }
  }
}

void sysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP: wifiProvDone = true; wifiJustConnected = true; break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: if (wifiProvDone) wifiDisconnected = true; break;
    default: break;
  }
}

void setupOTA() { 
  if (otaReady) return; 
  ArduinoOTA.setHostname(ota_hostname); ArduinoOTA.setPassword(ota_password); ArduinoOTA.begin(); otaReady = true;
}

void xuLyWiFiReconnect() {
  if (!wifiDisconnected || !wifiProvDone) return;
  static unsigned long lastRetryTime = 0;
  if (millis() - lastRetryTime < 10000UL) return;
  lastRetryTime = millis(); WiFi.reconnect();
  wifiDisconnected = false;
}

void reportInitialStates() {
  den_khach.updateAndReportParam ("Power", state_den_khach); den_ngu.updateAndReportParam   ("Power", state_den_ngu);
  den_bep.updateAndReportParam   ("Power", state_den_bep);   den_wc.updateAndReportParam    ("Power", state_den_wc);
  quat_khach.updateAndReportParam("Power", state_quat_khach);quat_ngu.updateAndReportParam  ("Power", state_quat_ngu);
  quat_bep.updateAndReportParam  ("Power", state_quat_bep);   may_bom.updateAndReportParam   ("Power", state_may_bom);
  rem_cua.updateAndReportParam   ("Power", state_rem_cua); 
  cam_bien_moi_truong.updateAndReportParam("Nguong Sang", target_lux);
  thiet_bi_an_ninh.updateAndReportParam("Che Do An Ninh", isArmed);
  syncAllDevices();
}

void setup() {
  Serial.begin(115200); 
  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RES); setServoAngle(SERVO_PIN, CLOSED_POS);
  ledcAttach(CLOTHES_SERVO_PIN, SERVO_FREQ, SERVO_RES); setServoAngle(CLOTHES_SERVO_PIN, CLOTHES_OUT_POS);
  
  Serial1.begin(SUB_BAUD, SERIAL_8N1, SUB_RX_PIN, SUB_TX_PIN);
  pinMode(STEP_IN1, OUTPUT); pinMode(STEP_IN2, OUTPUT); pinMode(STEP_IN3, OUTPUT); pinMode(STEP_IN4, OUTPUT);

  prefs.begin("smarthome", false);
  state_den_khach  = prefs.getBool("den_khach", false); state_den_ngu = prefs.getBool("den_ngu", false);
  state_den_bep = prefs.getBool("den_bep", false); state_den_wc = prefs.getBool("den_wc", false);
  state_quat_khach = prefs.getBool("quat_khach", false); state_quat_ngu = prefs.getBool("quat_ngu", false);
  state_quat_bep = prefs.getBool("quat_bep", false); state_may_bom = prefs.getBool("may_bom", false);
  state_rem_cua = prefs.getBool("rem_cua", false);
  matKhauDung = prefs.getString("mat_khau", "1234");
  target_lux = prefs.getInt("target_lux", 300);

  if (!prefs.isKey("num_cards")) {
    numCards = 1; byte defaultCard[4] = {0x85, 0xE2, 0x30, 0x07};
    memcpy(validCards[0], defaultCard, 4);
    prefs.putBytes("rfid_cards", validCards, sizeof(validCards));
    prefs.putInt("num_cards", numCards);
  } else {
    numCards = prefs.getInt("num_cards", 1);
    prefs.getBytes("rfid_cards", validCards, sizeof(validCards));
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP); dht.begin();
  pinMode(BUZZER_PIN, OUTPUT); digitalWrite(BUZZER_PIN, LOW);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP); pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP); 
  pinMode(GAS_SENSOR_PIN, INPUT_PULLUP); pinMode(GAS_ANALOG_PIN, INPUT);        
  pinMode(RAIN_SENSOR_PIN, INPUT_PULLUP); pinMode(TOUCH_SENSOR_PIN, INPUT); 
  
  current_step_pos = state_rem_cua ? so_buoc_mo_rem : 0;
  target_step_pos = current_step_pos;
  xTaskCreatePinnedToCore(stepperCode, "StepperTask", 4096, NULL, 1, &StepperTask, 0);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN); mfrc522.PCD_Init();
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN); kpd.begin();
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    oled.clearDisplay(); oled.setTextColor(SSD1306_WHITE); oled.setCursor(0,0); oled.print("Khoi dong..."); oled.display();
  }
  bh1750_trong.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  bh1750_ngoai.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire);
  
  Node my_node = RMaker.initNode("Mạng phân tán Main");
  den_khach.addCb(write_callback); den_ngu.addCb(write_callback); den_bep.addCb(write_callback); den_wc.addCb(write_callback);
  quat_khach.addCb(write_callback); quat_ngu.addCb(write_callback); quat_bep.addCb(write_callback); may_bom.addCb(write_callback); 
  rem_cua.addCb(write_callback); 

  my_node.addDevice(den_khach); my_node.addDevice(den_ngu); my_node.addDevice(den_bep); my_node.addDevice(den_wc);
  my_node.addDevice(quat_khach); my_node.addDevice(quat_ngu); my_node.addDevice(quat_bep); my_node.addDevice(may_bom);
  my_node.addDevice(rem_cua);
  
  Param tempParam("Nhiet Do", ESP_RMAKER_PARAM_TEMPERATURE, value(0.0f), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  tempParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(tempParam);
  Param humidParam("Do Am", "esp.param.humidity", value(0.0f), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  humidParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(humidParam);
  Param flameParam("Bao Chay", "esp.param.fire", value("An toan"), PROP_FLAG_READ);
  flameParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(flameParam);
  Param gasParam("Ro ri Gas", "esp.param.gas", value("An toan"), PROP_FLAG_READ);
  gasParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(gasParam);
  Param gasLevelParam("Nong do Gas", "esp.param.gas_level", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  gasLevelParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(gasLevelParam);
  Param soilParam("Do Am Dat", "esp.param.soil_moisture", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  soilParam.addUIType(ESP_RMAKER_UI_TEXT); cam_bien_moi_truong.addParam(soilParam);
  Param luxInParam("Anh Sang Trong", "esp.param.lux", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  cam_bien_moi_truong.addParam(luxInParam);
  Param luxOutParam("Anh Sang Ngoai", "esp.param.lux", value(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
  cam_bien_moi_truong.addParam(luxOutParam);
  Param targetLuxParam("Nguong Sang", "esp.param.target_lux", value(target_lux), PROP_FLAG_READ | PROP_FLAG_WRITE);
  targetLuxParam.addBounds(value(100), value(800), value(10)); targetLuxParam.addUIType(ESP_RMAKER_UI_SLIDER);
  cam_bien_moi_truong.addParam(targetLuxParam);
  my_node.addDevice(cam_bien_moi_truong);

  Param secParam("Trang Thai An Ninh", "esp.param.security", value("Binh thuong"), PROP_FLAG_READ);
  secParam.addUIType(ESP_RMAKER_UI_TEXT); thiet_bi_an_ninh.addParam(secParam); 
  Param armParam("Che Do An Ninh", ESP_RMAKER_PARAM_POWER, value(true), PROP_FLAG_READ | PROP_FLAG_WRITE);
  armParam.addUIType(ESP_RMAKER_UI_TOGGLE); thiet_bi_an_ninh.addParam(armParam);
  Param cardParam("Quan Ly The", ESP_RMAKER_PARAM_POWER, value(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
  cardParam.addUIType(ESP_RMAKER_UI_TOGGLE); thiet_bi_an_ninh.addParam(cardParam);
  Param passParam("Doi Mat Khau", ESP_RMAKER_PARAM_POWER, value(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
  passParam.addUIType(ESP_RMAKER_UI_TOGGLE); thiet_bi_an_ninh.addParam(passParam);
  Param simIntrusionParam("Gia Lap Dot Nhap", ESP_RMAKER_PARAM_POWER, value(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
  simIntrusionParam.addUIType(ESP_RMAKER_UI_TOGGLE); thiet_bi_an_ninh.addParam(simIntrusionParam);
  thiet_bi_an_ninh.addCb(write_callback);
  my_node.addDevice(thiet_bi_an_ninh);

  Param rainParam("Thoi Tiet", "esp.param.rain", value("Khong Mua - Dang phoi"), PROP_FLAG_READ);
  rainParam.addUIType(ESP_RMAKER_UI_TEXT); thiet_bi_gian_phoi.addParam(rainParam); my_node.addDevice(thiet_bi_gian_phoi);

  RMaker.enableTZService(); RMaker.enableSchedule(); RMaker.start();
  WiFi.onEvent(sysProvEvent);
  WiFiProv.beginProvision(NETWORK_PROV_SCHEME_SOFTAP, NETWORK_PROV_SCHEME_HANDLER_NONE, NETWORK_PROV_SECURITY_1, pop, service_name);
}

void loop() {
  handleSubSerial(); xuLyWiFiReconnect();
  setupOTA(); ArduinoOTA.handle();
  if (wifiJustConnected) { delay(1000); reportInitialStates(); wifiJustConnected = false; }

  if (digitalRead(BUTTON_PIN) == LOW) {
    unsigned long t = millis();
    while (digitalRead(BUTTON_PIN) == LOW) { delay(10); if (millis() - t > 3000) RMakerFactoryReset(2); }
  }

  if (dangNhapMatKhau && (millis() - lastKeyTime > KEYPAD_TIMEOUT_MS) && !doiMatKhauMode) { 
    dangNhapMatKhau = false; inputBuffer = ""; 
  }

  xuLyGuiSensorNonBlocking(); xuLyTouchTTP223(); 
  xuLyRFID(); xuLyKeypad(); 
  xuLyCacCamBienAnNinh(); xuLyThuDoTuDong(); 

  if (millis() - lastAutoLightTime > 3000UL) { lastAutoLightTime = millis(); xuLyTuDongAnhSang(); }

  static unsigned long lastLcdUpdate = 0;
  if (millis() - lastLcdUpdate > 2500UL) { lastLcdUpdate = millis(); docDHTVaHienThiLCD(); }

  static unsigned long lastSensorUpdate = 0;
  if (millis() - lastSensorUpdate > 60000UL) { lastSensorUpdate = millis(); guiSensorLenRainMaker(); }
}
