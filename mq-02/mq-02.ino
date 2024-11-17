int cambien = 26;
int giatri;
void setup() {
  Serial.begin(9600);
  pinMode(cambien,INPUT);
}

void loop() {
  giatri = digitalRead(cambien);
  Serial.print("Giá trị cảm biến: ");
  Serial.println(giatri);
  delay(200);
}
