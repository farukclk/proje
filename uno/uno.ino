#include <LiquidCrystal.h>
#include <SoftwareSerial.h>


SoftwareSerial mySerial(8, 9); // RX, TX


int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;   //LCD'nin pin değişkenlerini tanımlıyoruz.
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);              //LCD'nin pin bağlantılarını ayarlıyoruz.

void setup() {
  mySerial.begin(9600);

  lcd.begin(16, 2);                                     //LCD ekranımızın en-boy oranını ayarlıyoruz.                               
  delay(1000);
  lcd.clear();                                          //LCD'deki eski yazılar temizlenir.
  lcd.setCursor(0, 0);                                  //LCD'nin 1. satır 1. sütunundan yazmaya başlıyoruz.      
  lcd.print("vahsikeleblekler");                                
  lcd.setCursor(0, 1);                                  //LCD'nin 2. satır 1. sütunundan yazmaya başlıyoruz.                               //Uzaklık değerini LCD'ye yazdırıyoruz.
  lcd.print("welcome");
}

void loop() {
  //  delay(100000);
  if(mySerial.available() > 0) { 
   
    //webSocket.broadcastTXT(c, sizeof(c));

    String gelen_veri = mySerial.readStringUntil('\n'); // Enter ile biten veriyi oku
    gelen_veri.trim(); // Baştaki ve sondaki boşlukları temizle
    
    lcd.clear();
     Serial.println(gelen_veri);

    
    int ayrac_index = gelen_veri.indexOf('|');
    
    if (ayrac_index != -1) { // Eğer '|' bulunduysa
      String satir1 = gelen_veri.substring(0, ayrac_index);
      String satir2 = gelen_veri.substring(ayrac_index + 1);

      lcd.setCursor(0, 0);
      lcd.print(satir1);
      
      lcd.setCursor(0, 1);
      lcd.print(satir2);
    } 
    else {
      // Eğer | yoksa, her ihtimale karşı sadece birinci satıra yaz
      lcd.setCursor(0, 0);
      lcd.print(gelen_veri);
    }
  }
}

