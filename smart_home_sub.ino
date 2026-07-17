/*
 * ============================================================
 * SMART HOME - SUB 
 * ============================================================
 */

// ==============================================================================
// 1. KHAI BÁO CHÂN THIẾT BỊ VÀ ĐÈN LED 
// ==============================================================================
#define MAIN_TX_PIN         17  // Nối vào RX(18) của Main
#define MAIN_RX_PIN         16  // Nối vào TX(17) của Main

// -- Cảm biến --
#define SOIL_MOISTURE_PIN   34  // Chân Analog đọc Cảm biến độ ẩm đất

// -- Hệ thống LED Đèn --
#define PIN_LED_KHACH       4   // LED Phòng Khách (Hỗ trợ PWM tự động điều chỉnh độ sáng)
#define PIN_LED_NGU         5   
#define PIN_LED_BEP         18  
#define PIN_LED_WC          19  

// -- Hệ thống Relay --
#define RELAY_FAN_KHACH     21  
#define RELAY_FAN_NGU       22  
#define RELAY_FAN_BEP       23  
#define RELAY_MAY_BOM       25  

// ==============================================================================
// 2. BIẾN TOÀN CỤC VÀ TRẠNG THÁI
// ==============================================================================
bool state_dk = false;
bool state_dn = false; bool state_db = false; bool state_dw = false;
bool state_qk = false; bool state_qn = false;
bool state_qb = false; bool state_mb = false;

int phanTramDat = 0;       
unsigned long lastSoilSampleTime = 0;

// ==============================================================================
// 3. CẤU TRÚC LỌC NHIỄU CHO NHIỀU NÚT NHẤN (DEBOUNCE ARRAY)
// ==============================================================================
struct NutNhan {
  int pin;                      
  String deviceCode;           
  int lastFlickerState;           
  int confirmedState;             
  unsigned long lastDebounceTime; 
};

NutNhan danhSachNutNhan[] = {
  {13, "DK", HIGH, HIGH, 0}, 
  {14, "DN", HIGH, HIGH, 0}, 
  {27, "DB", HIGH, HIGH, 0}, 
  {32, "DW", HIGH, HIGH, 0}, 
  {33, "QK", HIGH, HIGH, 0}, 
  {26, "QN", HIGH, HIGH, 0}, 
  {2,  "QB", HIGH, HIGH, 0}, 
  {15, "MB", HIGH, HIGH, 0}  
};
const int TONG_SO_NUT = sizeof(danhSachNutNhan) / sizeof(NutNhan);

// ==============================================================================
// 4. HÀM KHỞI TẠO VÀ ĐIỀU KHIỂN THIẾT BỊ
// ==============================================================================
void initAllDevices() {
  ledcAttach(PIN_LED_KHACH, 5000, 8);
  ledcWrite(PIN_LED_KHACH, 0); 
  Serial.println(F("[SUB-LED] Da khoi tao PWM cho LED Phong Khach (GPIO 4)"));

  pinMode(PIN_LED_NGU, OUTPUT); digitalWrite(PIN_LED_NGU, LOW);
  pinMode(PIN_LED_BEP, OUTPUT); digitalWrite(PIN_LED_BEP, LOW);
  pinMode(PIN_LED_WC,  OUTPUT); digitalWrite(PIN_LED_WC,  LOW);
  Serial.println(F("[SUB-LED] Da khoi tao cac LED con lai (GPIO 5, 18, 19) o muc LOW"));

  pinMode(RELAY_FAN_KHACH,  OUTPUT); digitalWrite(RELAY_FAN_KHACH,  HIGH); 
  pinMode(RELAY_FAN_NGU,    OUTPUT); digitalWrite(RELAY_FAN_NGU,    HIGH);
  pinMode(RELAY_FAN_BEP,    OUTPUT); digitalWrite(RELAY_FAN_BEP,    HIGH);
  pinMode(RELAY_MAY_BOM,    OUTPUT); digitalWrite(RELAY_MAY_BOM,    LOW);  
  Serial.println(F("[SUB-RELAY] Da khoi tao cac Relay Quat & May Bom"));
}

void setDeviceState(String name, bool state) {
  if (name == "DK") { 
    state_dk = state;
    ledcWrite(PIN_LED_KHACH, state ? 255 : 0); 
    Serial.printf("[SUB-LOG] LED Phong Khach -> %s (PWM: %d)\n", state ? "BAT" : "TAT", state ? 255 : 0);
  }
  else if (name == "DN") { 
    state_dn = state;
    digitalWrite(PIN_LED_NGU,   state ? HIGH : LOW); 
    Serial.printf("[SUB-LOG] LED Phong Ngu -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "DB") { 
    state_db = state;
    digitalWrite(PIN_LED_BEP,   state ? HIGH : LOW); 
    Serial.printf("[SUB-LOG] LED Bep -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "DW") { 
    state_dw = state;
    digitalWrite(PIN_LED_WC,    state ? HIGH : LOW); 
    Serial.printf("[SUB-LOG] LED WC -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "QK") { 
    state_qk = state;
    digitalWrite(RELAY_FAN_KHACH,  state ? LOW : HIGH); 
    Serial.printf("[SUB-LOG] Quat Phong Khach -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "QN") { 
    state_qn = state;
    digitalWrite(RELAY_FAN_NGU,    state ? LOW : HIGH); 
    Serial.printf("[SUB-LOG] Quat Phong Ngu -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "QB") { 
    state_qb = state;
    digitalWrite(RELAY_FAN_BEP,    state ? LOW : HIGH); 
    Serial.printf("[SUB-LOG] Quat Bep -> %s\n", state ? "BAT" : "TAT");
  }
  else if (name == "MB") { 
    state_mb = state;
    digitalWrite(RELAY_MAY_BOM,    state ? HIGH : LOW); 
    Serial.printf("[SUB-LOG] May Bom -> %s\n", state ? "BAT" : "TAT");
  } 
}

bool getDeviceState(String name) {
  if (name == "DK") return state_dk;
  if (name == "DN") return state_dn;
  if (name == "DB") return state_db; 
  if (name == "DW") return state_dw;
  if (name == "QK") return state_qk; 
  if (name == "QN") return state_qn;
  if (name == "QB") return state_qb;
  if (name == "MB") return state_mb;
  return false;
}

// ==============================================================================
// 5. XỬ LÝ NÚT NHẤN 
// ==============================================================================
void xuLyCacNutNhan() {
  for (int i = 0; i < TONG_SO_NUT; i++) {
    int reading = digitalRead(danhSachNutNhan[i].pin);
    if (reading != danhSachNutNhan[i].lastFlickerState) {
      danhSachNutNhan[i].lastDebounceTime = millis();
    }

    if ((millis() - danhSachNutNhan[i].lastDebounceTime) > 50) {
      if (reading != danhSachNutNhan[i].confirmedState) {
        danhSachNutNhan[i].confirmedState = reading;
        if (danhSachNutNhan[i].confirmedState == LOW) {
          String code = danhSachNutNhan[i].deviceCode;
          bool newState = !getDeviceState(code); 
          
          setDeviceState(code, newState); 
          
          Serial2.printf("EVT:VAL:%s:%d\n", code.c_str(), newState ? 1 : 0);
          Serial.printf("[BUTTON] Bam nut vat ly -> [%s] chuyen thanh [%s]\n", code.c_str(), newState ? "BAT" : "TAT");
        }
      }
    }
    danhSachNutNhan[i].lastFlickerState = reading;
  }
}

// ==============================================================================
// 6. XỬ LÝ ĐỘ ẨM ĐẤT & TỰ ĐỘNG TƯỚI
// ==============================================================================
void xuLyHeThongTuoiNuoc() {
  if (millis() - lastSoilSampleTime > 2000) { 
    lastSoilSampleTime = millis();
    int rawAnalog = analogRead(SOIL_MOISTURE_PIN);
  
    int pct = map(rawAnalog, 4095, 1200, 0, 100);
    pct = constrain(pct, 0, 100);
    
    Serial.printf("[GARDEN] Analog tho: %d => Do am quy doi: %d%%\n", rawAnalog, pct);

    static unsigned long lastSyncSoil = 0;
    if (millis() - lastSyncSoil > 60000) {
      lastSyncSoil = millis();
      Serial2.printf("EVT:VAL:SOIL:%d\n", phanTramDat);
      Serial.println(F("[UART] Da gui cap nhat Do Am Dat sang mach Main."));
    }
  }
}

// ==============================================================================
// 7. NHẬN LỆNH TỪ BOARD TRUNG TÂM QUA UART
// ==============================================================================
void handleMainSerial() {
  while (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    
    if (line.length() > 0) {
      if (line.startsWith("CMD:DK_PWM:")) {
        int pwmVal = line.substring(11).toInt();
        pwmVal = constrain(pwmVal, 0, 255);
        state_dk = (pwmVal > 0);
        ledcWrite(PIN_LED_KHACH, pwmVal);
        Serial.printf("[UART NHAN] Main dieu chinh Dimming LED Khach -> PWM: %d\n", pwmVal);
      }
      else if (line.startsWith("CMD:")) {
        String phanSau = line.substring(4);
        int colonPos   = phanSau.indexOf(':');
        if (colonPos > 0) {
          String deviceName = phanSau.substring(0, colonPos);
          bool valState     = (phanSau.substring(colonPos + 1).toInt() == 1);
          setDeviceState(deviceName, valState);
          Serial.printf("[UART NHAN] Main yeu cau [%s] -> Trang thai: [%s]\n", deviceName.c_str(), valState ? "ON" : "OFF");
        }
      } else {
        Serial.printf("[UART NHAN CHƯA RÕ] %s\n", line.c_str());
      }
    }
  }
}

// ==============================================================================
// 8. KHỞI TẠO CẤU HÌNH SUB BOARD
// ==============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=================================================="));
  Serial.println(F("   MACH SUB ESP32 DANG KHOI DONG... (LED GPIO VER)"));
  Serial.println(F("=================================================="));

  Serial2.begin(115200, SERIAL_8N1, MAIN_TX_PIN, MAIN_RX_PIN);
  for (int i = 0; i < TONG_SO_NUT; i++) {
    pinMode(danhSachNutNhan[i].pin, INPUT_PULLUP);
  }

  pinMode(SOIL_MOISTURE_PIN, INPUT);

  initAllDevices();
  Serial2.println("EVT:READY");
  Serial.println(F("[SETUP] Da gui tin hieu chao hoi EVT:READY cho mach Main"));
}

// ==============================================================================
// 9. VÒNG LẶP CHÍNH
// ==============================================================================
void loop() {
  handleMainSerial();      
  xuLyCacNutNhan();        
  xuLyHeThongTuoiNuoc();    
  delay(10);
}
