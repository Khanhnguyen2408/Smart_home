//#define LED_PIN 25
#define BUTTON_PIN 12

bool isOff = true; // Biến để quản lý chế độ

void setup() {
//  pinMode(LED_PIN, OUTPUT);          // Cấu hình chân LED là OUTPUT
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Cấu hình nút là INPUT_PULLUP
  //digitalWrite(LED_PIN, LOW);        // LED bắt đầu ở trạng thái tắt
  Serial.begin(9600);               // Khởi động Serial để in thông báo
}

void loop() {
  static bool lastButtonState = HIGH; // Trạng thái nút trước đó
  bool buttonState = digitalRead(BUTTON_PIN); // Đọc trạng thái nút

  // Kiểm tra nếu nút được nhấn (chuyển từ HIGH sang LOW)
  if (lastButtonState == HIGH && buttonState == LOW) {
    isOff = !isOff; // Đổi trạng thái chế độ

    if (isOff) {
      Serial.println("Chế độ Đăng ký được bật!");
      //digitalWrite(LED_PIN, HIGH); // Bật LED
    } else {
      Serial.println("Chế độ Truy cập được bật!");
      //digitalWrite(LED_PIN, LOW); // Tắt LED
    }
    
    delay(500); // Tránh trạng thái lặp lại do nút bị rung
  }

  lastButtonState = buttonState; // Cập nhật trạng thái nút trước đó
}