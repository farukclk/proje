#include <LiquidCrystal.h>
#include <SoftwareSerial.h>


SoftwareSerial mySerial(8, 9); // RX, TX


int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;   //LCD'nin pin değişkenlerini tanımlıyoruz.
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);              //LCD'nin pin bağlantılarını ayarlıyoruz.

int selonoid = 13;


void setup() {

  Serial.begin(115200);
  Serial.println("başladı uno");
  pinMode(selonoid, OUTPUT);
  digitalWrite(selonoid, LOW);
  
  delay(500);
 
  lcd.begin(16, 2);                                     //LCD ekranımızın en-boy oranını ayarlıyoruz.                               
  lcd.clear();                                          //LCD'deki eski yazılar temizlenir.
  lcd.setCursor(0, 0);                                  //LCD'nin 1. satır 1. sütunundan yazmaya başlıyoruz.      
  lcd.print("vahsikeleblekler");                                
  lcd.setCursor(0, 1);                                  //LCD'nin 2. satır 1. sütunundan yazmaya başlıyoruz.                               //Uzaklık değerini LCD'ye yazdırıyoruz.
  lcd.print("welcome");
  mySerial.begin(9600);

}

void loop() {
  if(mySerial.available() > 0) { 
    //webSocket.broadcastTXT(c, sizeof(c));

    String gelen_veri = mySerial.readStringUntil('\n'); // Enter ile biten veriyi oku
    gelen_veri.trim(); // Baştaki ve sondaki boşlukları temizle
    

     
    
    int ayrac_index = gelen_veri.indexOf('|');
    
    if (ayrac_index != -1) { // Eğer '|' bulunduysa
      lcd.clear();
      String satir1 = gelen_veri.substring(0, ayrac_index);
      String satir2 = gelen_veri.substring(ayrac_index + 1);

      lcd.setCursor(0, 0);
      lcd.print(satir1);
      
      lcd.setCursor(0, 1);
      lcd.print(satir2);
    } 
    else {

      if (gelen_veri == "S_ON") 
        digitalWrite(selonoid, HIGH);
      else if (gelen_veri == "S_OFF")
        digitalWrite(selonoid, LOW);
      
      else {
        lcd.clear();
        // Eğer | yoksa, her ihtimale karşı sadece birinci satıra yaz
        lcd.setCursor(0, 0);
        lcd.print(gelen_veri);
      }
    }
  }
}

