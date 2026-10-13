// ESP8266 (NodeMCU/D1 mini) I2C adres tarayici.
// SDA = D2 (GPIO4), SCL = D1 (GPIO5). Serial Monitor: 115200.
// Adres tespitinden sonra asil Space Invaders sketch'ini yeniden yukleyin.
#include <Arduino.h>
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(4, 5);
  delay(500);
  Serial.println("I2C adres tarayici basladi (SDA=D2, SCL=D1)");
}
void loop() {
  uint8_t count = 0;
  for (uint8_t addr=1; addr<127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission()==0) {
      Serial.print("Cihaz bulundu: 0x");
      if (addr<16) Serial.print('0');
      Serial.println(addr, HEX);
      ++count;
    }
  }
  if (count==0) Serial.println("Cihaz yok. Kablo/besleme kontrol edin.");
  else Serial.println("Tarama tamamlandi.");
  delay(3000);
}
