const int buttonPin = 2;// nút nhấn với GPIO2
const int ledPin = 4; // kết nối led với GPIO 
int ledState = LOW; // mặc định đèn tắt 
int buttonState;
int lastButtonState = HIGH;
usigned long lastDebounceTime = 0;
usigned long debounceDelay = 50;
void setup() {
  pinMode(buttonPin, IUPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin,ledState);
}

void loop() {
  int reading = digitalRead(buttonPin);
  if(reading != lastButtonState){
    lastDebounceTime = millis();
  }
  if((millis()-lastDebounceTime) > debounceDelay){
    if(reading != buttonState){
      buttonState = reading;
      if(buttonState==HIGH){
        ledState = !ledState;
      }
    }
  }
  digitalWrite(ledPin, ledState);
  lastButtonState = reading;
}
