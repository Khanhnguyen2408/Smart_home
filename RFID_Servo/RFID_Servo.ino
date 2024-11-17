#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#define SDA_PIN 5 
#define RST_PIN 4
#define LED_PIN 3
#define BUTTON_PIN 0

Servo myservo;
int pos = 0; // Góc quay của servo
MFRC522 rfid(SDA_PIN, RST_PIN); // Khởi tạo RFID

bool isRegisterMode = true; // Chế độ mặc định là Đăng ký
String registeredCards[100]; // Danh sách thẻ đã đăng ký
int cardCount = 0; // Số lượng thẻ đã đăng ký

unsigned long lastButtonPress = 0; // Lưu thời gian lần nhấn nút cuối
unsigned long debounceDelay = 200; // Thời gian debounce (200ms)
unsigned long doubleClickThreshold = 500; // Thời gian giữa 2 lần nhấn (500ms)

bool isCardRegistered(String cardUID) {
  for (int i = 0; i < cardCount; i++) {
    if (registeredCards[i] == cardUID) {
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  myservo.attach(25); // Gắn servo vào chân 25
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Hệ thống bắt đầu...");
  Serial.println("Nhấn nút 2 lần để chuyển đổi giữa chế độ Đăng ký và Truy cập.");
}

void loop() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);

  // Kiểm tra xem nút có được nhấn (với debounce)
  if (buttonState == LOW && lastButtonState == HIGH) {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress < doubleClickThreshold) {
      // Nếu khoảng thời gian giữa 2 lần nhấn nhỏ hơn 500ms, đổi chế độ
      isRegisterMode = !isRegisterMode;
      if (isRegisterMode) {
        Serial.println("Chế độ Đăng ký được bật!");
        digitalWrite(LED_PIN, HIGH); // Sáng LED khi ở chế độ đăng ký
      } else {
        Serial.println("Chế độ Truy cập được bật!");
        digitalWrite(LED_PIN, LOW); // Tắt LED khi ở chế độ truy cập
      }
    }
    lastButtonPress = currentTime;
  }
  lastButtonState = buttonState;

  // Kiểm tra thẻ RFID
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String cardUID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardUID += String(rfid.uid.uidByte[i], HEX); // Chuyển UID thành chuỗi Hex
  }
  cardUID.toUpperCase();

  Serial.print("Thẻ được quét UID: ");
  Serial.println(cardUID);

  if (isRegisterMode) {
    // Chế độ đăng ký
    if (!isCardRegistered(cardUID)) {
      if (cardCount < 100) {
        registeredCards[cardCount++] = cardUID;
        Serial.println("Thẻ đã được đăng ký thành công!");
      } else {
        Serial.println("Danh sách thẻ đầy, không thể đăng ký thêm.");
      }
    } else {
      Serial.println("Thẻ này đã được đăng ký trước đó.");
    }
  } else {
    // Chế độ truy cập
    if (isCardRegistered(cardUID)) {
      Serial.println("Truy cập thành công!");
      digitalWrite(LED_PIN, HIGH); // Sáng LED khi thẻ hợp lệ
      myservo.write(140);
      delay(5000);
      myservo.write(0);
      delay(500);
    } else {
      Serial.println("Truy cập bị từ chối. Thẻ không hợp lệ!");
    }
  }

  rfid.PICC_HaltA(); // Dừng hoạt động của thẻ
}