#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include "DFRobotDFPlayerMini.h"
#include <SPI.h>
#include <MFRC522.h>
#include "FirebaseESP32.h"
#include "EEPROM.h"
#include "WiFi.h"

// WiFi và Firebase config
#define LENGTH(x) (strlen(x) + 1)  // length of char string
#define EEPROM_SIZE 200            // EEPROM size
#define WiFi_rst 0                 //WiFi credential reset pin (Boot button on ESP32)
#define FIREBASE_HOST "https://iot-btl-1e68d-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "GOo9nC24lYOr132BkvT9Imahst9ZsJi5fUgmVuiw"
#define DHT_pin 15
#define Servo1_pin 25
#define Servo2_pin 33
#define MQ2_pin 26
#define RX_pin 16
#define TX_pin 17
#define Quang_pin 34
#define SDA_PIN 5
#define RST_PIN 4
#define PK 13
#define T2 3
#define PN 12
#define H 32
#define BC 27
FirebaseData fb;
String ssid;  //string variable to store ssid
String pss;   //string variable to store password
unsigned long rst_millis;
typedef struct {
  float temperature;  // Nhiệt độ từ DHT11
  float humidity;     // Độ ẩm từ DHT11
  String cardID;      // id từ thẻ đăng kí
  int mq2;            // Trạng thái cảm biến MQ2
  int light;          // Trạng thái cảm biến quang
} data_t;

DFRobotDFPlayerMini myDFPlayer;
Servo servo1;
Servo servo2;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht2(DHT_pin, DHT11);
// RFID
MFRC522 rfid(SDA_PIN, RST_PIN);  // Khởi tạo RFID

volatile bool buttonPressed = false;  // Biến cờ báo hiệu nhấn nút
bool isRegisterMode = true;           // Chế độ mặc định là Đăng ký
String registeredCards[100];          // Danh sách thẻ đã đăng ký
int cardCount = 0;                    // Số lượng thẻ đã đăng ký

// Hàm ISR để xử lý ngắt
void IRAM_ATTR handleButtonInterrupt() {
  buttonPressed = true;  // Đặt cờ để xử lý trong loop()
}

// Kiểm tra xem thẻ đã đăng ký chưa
bool isCardRegistered(String cardUID) {
  for (int i = 0; i < cardCount; i++) {
    if (registeredCards[i] == cardUID) {
      return true;
    }
  }
  return false;
}

void servo_rem(int state) {
  if (state == 1) {
    servo2.write(180);
    delay(200);
  } else {
    servo2.write(0);
    delay(200);
  }
}

void sensor_quang(data_t *data) {
  data->light = digitalRead(Quang_pin);

  if (data->light == 0) {
    Serial.println("Phát hiện trời sáng!");
    servo_rem(1);
  } else {
    Serial.println("Phát hiện trời tối!");
    servo_rem(0);
  }

  // Gửi dữ liệu lên Firebase
  sent_data_fb(data, "/light");
}


void sensor_MQ2(data_t *data) {
  data->mq2 = digitalRead(MQ2_pin);

  Serial.println(data->mq2);
  if (data->mq2 == 0) {
    myDFPlayer.play(1);
    delay(30000);
  }

  // Gửi dữ liệu lên Firebase
  sent_data_fb(data, "/mq2");
}


void sensor_dht11(data_t *data) {
  data->temperature = dht2.readTemperature();
  data->humidity = dht2.readHumidity();

  lcd.setCursor(3, 0);
  lcd.print(round(data->temperature));
  lcd.setCursor(10, 0);
  lcd.print(round(data->humidity));
  delay(1000);

  // Gửi dữ liệu lên Firebase
  sent_data_fb(data, "/temperature");
  sent_data_fb(data, "/humidity");
}

void sendCardIDToFirebase(String cardID, String path) {
  if (Firebase.setString(fb, path, cardID)) {
    Serial.println("CardID đã được gửi thành công: " + cardID);
  }
}


bool isDoorOpen = false;  // Trạng thái cửa (đóng = false, mở = true)

void handleRFID() {
  // Kiểm tra xem có thẻ mới và đọc thẻ không
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Lấy UID của thẻ và chuyển thành chuỗi HEX
  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i], HEX);
  }
  cardUID.toUpperCase();  // Chuyển thành chữ in hoa

  Serial.print("Thẻ được quét UID: ");
  Serial.println(cardUID);

  // Kiểm tra xem thẻ có trong "path kiểm tra" hay không
  String checkPath = "/allowedCards";       // Path chứa các thẻ hợp lệ
  String newCardPath = "/newCards";         // Path lưu các thẻ mới
  if (Firebase.getString(fb, checkPath)) {  // Lấy danh sách thẻ từ Firebase
    String allowedCards = fb.stringData();  // Danh sách thẻ hợp lệ

    if (allowedCards.indexOf(cardUID) != -1) {  // Thẻ có tồn tại
      if (!isDoorOpen) {
        // Mở cửa
        Serial.println("Thẻ hợp lệ. Mở cửa.");
        servo1.write(180);  // Mở khóa
        isDoorOpen = true;  // Cập nhật trạng thái cửa
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Cua da mo!");
      } else {
        // Đóng cửa
        Serial.println("Thẻ hợp lệ. Đóng cửa.");
        servo1.write(0);     // Đóng khóa
        isDoorOpen = false;  // Cập nhật trạng thái cửa
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Cua da dong!");
      }
    } else {
      // Thẻ không hợp lệ
      Serial.println("Thẻ không hợp lệ.");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("The khong hop le");
      // Lưu thẻ mới vào "newCards" trên Firebase
      sendCardIDToFirebase(cardUID, newCardPath);
    }
  } else {
    Serial.println("Không thể lấy danh sách thẻ hợp lệ từ Firebase.");
  }

  rfid.PICC_HaltA();  // Dừng hoạt động của thẻ
}


//tải dữ liệu từ firebase
void loadRegisteredCards() {
  if (Firebase.getString(fb, "/rfid")) {        // Thực hiện yêu cầu lấy dữ liệu từ Firebase
    String dataFromFirebase = fb.stringData();  // Lấy dữ liệu từ đối tượng FirebaseData
    Serial.println("Data from Firebase: " + dataFromFirebase);

    // Loại bỏ dấu ngoặc vuông đầu và cuối nếu có
    dataFromFirebase.replace("[", "");  // Loại bỏ dấu [
    dataFromFirebase.replace("]", "");  // Loại bỏ dấu ]

    // Chuyển dữ liệu thành mảng thẻ đã đăng ký
    int startIndex = 0;
    int endIndex = dataFromFirebase.indexOf('"');  // Tìm dấu ngoặc kép đầu tiên

    // Duyệt qua các thẻ trong mảng
    while (endIndex != -1) {
      String cardID = dataFromFirebase.substring(startIndex, endIndex);  // Lấy thẻ
      registeredCards[cardCount++] = cardID;                             // Lưu vào mảng
      startIndex = endIndex + 1;                                         // Cập nhật chỉ số bắt đầu
      endIndex = dataFromFirebase.indexOf('"', startIndex);              // Tìm dấu ngoặc kép tiếp theo
    }
  } else {
    Serial.println("Failed to get data from Firebase");
  }
}

// gửi data lên firebase
void sent_data_fb(data_t *data, String path) {
  // Ví dụ: Firebase.setFloat() để gửi số thực
  if (path == "/temperature") {
    Firebase.setFloat(fb, path, data->temperature);
  } else if (path == "/humidity") {
    Firebase.setFloat(fb, path, data->humidity);
  } else if (path == "/mq2") {
    Firebase.setInt(fb, path, data->mq2);
  } else if (path == "/light") {
    Firebase.setInt(fb, path, data->light);
  }
}

//lấy data từ firebase
void receive_data_fb() {
  bool pk, t2, pn, h, bc;

  // Lấy giá trị từ Firebase
  if (Firebase.getBool(fb, "/v1")) {
    pk = fb.boolData();
  } else {
    Serial.println("Lỗi khi lấy dữ liệu từ /v1:");
    Serial.println(fb.errorReason());
    pk = false;  // Gán giá trị mặc định
  }

  if (Firebase.getBool(fb, "/v3")) {
    t2 = fb.boolData();
  } else {
    Serial.println("Lỗi khi lấy dữ liệu từ /v3:");
    Serial.println(fb.errorReason());
    t2 = false;
  }

  if (Firebase.getBool(fb, "/v0")) {
    pn = fb.boolData();
  } else {
    Serial.println("Lỗi khi lấy dữ liệu từ /v0:");
    Serial.println(fb.errorReason());
    pn = false;
  }

  if (Firebase.getBool(fb, "/v2")) {
    h = fb.boolData();
  } else {
    Serial.println("Lỗi khi lấy dữ liệu từ /v2:");
    Serial.println(fb.errorReason());
    h = false;
  }

  if (Firebase.getBool(fb, "/v5")) {
    bc = fb.boolData();
  } else {
    Serial.println("Lỗi khi lấy dữ liệu từ /v5:");
    Serial.println(fb.errorReason());
    bc = false;
  }

  // Điều khiển chân digital
  digitalWrite(PK, pk);
  digitalWrite(T2, t2);
  digitalWrite(PN, pn);
  digitalWrite(h, h);
  digitalWrite(bc, bc);

  // In giá trị ra serial
  Serial.print("PK: ");
  Serial.println(pk);
  Serial.print("T2: ");
  Serial.println(t2);
  Serial.print("PN: ");
  Serial.println(pn);
  Serial.print("H: ");
  Serial.println(h);
  Serial.print("BC: ");
  Serial.println(bc);
}

// hàm đọc và ghi vào eeprom
void writeStringToFlash(const char *toStore, int startAddr) {
  int i = 0;
  for (; i < LENGTH(toStore); i++) {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}
String readStringFromFlash(int startAddr) {
  char in[128];  // char array of size 128 for reading the stored data
  int i = 0;
  for (; i < 128; i++) {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}
void setup() {
  Serial.begin(9600);
  pinMode(PK, OUTPUT);
  pinMode(T2, OUTPUT);
  pinMode(PN, OUTPUT);
  pinMode(H, OUTPUT);
  pinMode(BC, OUTPUT);
  // Khởi tạo màn hình
  lcd.init();
  lcd.backlight();
  lcd.print("Waiting for Wifi.");
  //Init WiFi as Station, start SmartConfig
  pinMode(WiFi_rst, INPUT);
  if (!EEPROM.begin(EEPROM_SIZE)) {  //Init EEPROM
    Serial.println("failed to init EEPROM");
    delay(1000);
  } else {
    ssid = readStringFromFlash(0);  // Read SSID stored at address 0
    Serial.print("SSID = ");
    Serial.println(ssid);
    pss = readStringFromFlash(40);  // Read Password stored at address 40
    Serial.print("psss = ");
    Serial.println(pss);
  }
  WiFi.begin(ssid.c_str(), pss.c_str());
  delay(3500);                        // Wait for a while till ESP connects to WiFi
  if (WiFi.status() != WL_CONNECTED)  // if WiFi is not connected
  {
    //Init WiFi as Station, start SmartConfig
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig();
    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("SmartConfig received.");
    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      lcd.print(".");
    }
    Serial.println("WiFi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    // read the connected WiFi SSID and password
    ssid = WiFi.SSID();
    pss = WiFi.psk();
    Serial.print("SSID:");
    Serial.println(ssid);
    Serial.print("PSS:");
    Serial.println(pss);
    Serial.println("Store SSID & PSS in Flash");
    writeStringToFlash(ssid.c_str(), 0);  // storing ssid at address 0
    writeStringToFlash(pss.c_str(), 40);  // storing pss at address 40
  } else {
    lcd.print("WiFi Connected");
    lcd.clear();
  }

  // Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  // Khởi tạo Servo
  servo1.attach(Servo1_pin);
  servo2.attach(Servo2_pin);

  // Khởi tạo MQ-02
  Serial1.begin(9600, SERIAL_8N1, RX_pin, TX_pin);
  if (!myDFPlayer.begin(Serial1)) {
    Serial.println("Không thể khởi động");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("DFPlayer đang hoạt động");
  myDFPlayer.volume(25);
  pinMode(26, INPUT);

  // Khởi tạo cảm biến quang
  pinMode(Quang_pin, INPUT);

  // Tải danh sách thẻ từ Firebase
  loadRegisteredCards();

  // Khởi tạo RFID
  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Hệ thống bắt đầu...");
}

void loop() {
  static data_t data = { 0 };  // Khởi tạo struct
  //lay du lieu tu firebase
  receive_data_fb();
  // nhấn nút reset cấu hình wifi
  rst_millis = millis();
  while (digitalRead(WiFi_rst) == LOW) {
    // Wait till boot button is pressed
  }
  // check the button press time if it is greater than 0.5sec clear wifi cred and restart ESP
  if (millis() - rst_millis >= 500) {
    Serial.println("Reseting the WiFi credentials");
    writeStringToFlash("", 0);   // Reset the SSID
    writeStringToFlash("", 40);  // Reset the Password
    Serial.println("Wifi credentials erased");
    Serial.println("Restarting the ESP");
    delay(500);
    ESP.restart();  // Restart ESP
  }
  sensor_dht11(&data);
  sensor_quang(&data);
  sensor_MQ2(&data);
  handleRFID();
}