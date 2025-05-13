#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>
#include <FS.h>         // SPIFFS için
#include "config.h"


SoftwareSerial unoSerial(D0, D1); // RX, TX


SoftwareSerial mySerial(D2, D3); // RX, TX (GPIO14, GPIO12)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);


/*
ESP8266WebServerSecure server(443);
BearSSL::X509List cert(serverCert);
BearSSL::PrivateKey key(serverKey);
*/


ESP8266WebServer server(80);  // Port 80 (HTTP için yaygın port)


WiFiClientSecure client;
const char* host = "vahsikelebekler.pythonanywhere.com";
const int httpsPort = 443;
const String url = "/rapor";
const char fingerprint[] = "8F:89:21:9B:B6:AA:EC:5B:A7:20:9A:BC:F9:D0:A5:26:B6:E3:79:32"; // SHA-1 fingerprint



unsigned long sonKontrolZamani = 0;
const unsigned long kontrolAraligi = 1100;     // parmak izi sensorü çalışma frekansı
const unsigned long kilit_suresi = 5000;       // kilidin açık kalma süresi
uint8_t basarisiz_deneme = 0;                  // üstüste yapilan başarısız deneme sayisi
const uint8_t DENEME_HAKKI = 10;               //          


// Basit HTML arayüz
const char htmlPage[] PROGMEM = R"rawliteral(
<h1> hello world </h1>
)rawliteral";


const char html[] PROGMEM = R"rawliteral(<!DOCTYPE html><html lang="tr"><head><meta charset="UTF-8"><title>Kullanıcı Düzenleme Paneli</title><link rel="stylesheet" href="http://samsung/a.css"><style>#addUserForm{margin:20px 0}#addUserForm input{padding:5px;font-size:16px}#addUserForm button{padding:5px 10px;font-size:16px;cursor:pointer}</style></head><body><h1>Kullanıcı Özellikleri</h1><div id="batteryStatus" class="unknown" title="Şarj Durumu">❔</div><div id="addUserForm"><input type="text" id="nameInput" placeholder="İsim girin"><button onclick="addName()">Ekle</button></div><table id="userTable"><thead><tr><th>ID</th><th>İsim</th><th>İçeride mi?</th><th>Admin</th><th>Kapıyı Açabilir mi?</th><th>Sil</th></tr></thead><tbody></tbody></table><script src="http://fedora/a.js"></script></body></html>)rawliteral";

  



int selonoid = D4;
unsigned long unlockTime = 0;
bool isUnlocked = false;



struct User {
  uint8_t is_valid : 1;
  uint8_t is_admin : 1;
  uint8_t can_open_door : 1;
  uint8_t is_in : 1;
  uint8_t reserved : 4;
  char name[21];       
};

const int user_count = 50;
User users[user_count] = {};


File file;
String usersFileName = "/users.txt";
String logFileName = "/logs.txt";   // Log dosyasının adı
#define SD_CS D8  // Chip Select pini


// İstanbul için saat dilimi (UTC+3)
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0; // Yaz saati uygulaması yoksa 0






void setup() {
  Serial.begin(115200);
  unoSerial.begin(9600);


  pinMode(selonoid, OUTPUT);
  digitalWrite(selonoid, LOW);
 


  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS başlatılamadı!");
    return;
  }

  delay(1000);
  

  if (!SPIFFS.exists(logFileName)) {
    Serial.println("Dosya bulunamadı, oluşturuluyor...");
    File file = SPIFFS.open(logFileName, "w");
    if (file) {
      file.println();
      file.close();
      Serial.println("Boş log dosya oluşturuldu.");
    }
    else {
      Serial.println("Dosya log oluşturulamadı!");
    }
  } else {
    Serial.println("Dosya log bulundu, okunuyor...");
    get_logs();
  }



  if (!SPIFFS.exists(usersFileName)) {
    Serial.println("Dosya bulunamadı, oluşturuluyor...");
    File file = SPIFFS.open(usersFileName, "w");
    if (file) {
      for (int i = 0; i < user_count; i++) {
        file.println();
      }
      file.close();
      Serial.println("Boş dosya oluşturuldu.");
    } else {
      Serial.println("Dosya oluşturulamadı!");
    }
  } else {
    Serial.println("Dosya bulundu, okunuyor...");


    File file = SPIFFS.open(usersFileName, "r");
    int satirNumarasi = 0;

    while (file.available()) {
      String satir = file.readStringUntil('\n');
      satir.trim();

      if (satir.length() > 0) {
        int virgulIndex = satir.indexOf(',');
        if (virgulIndex != -1) {
          String id = satir.substring(0, virgulIndex);
          String isim = satir.substring(virgulIndex + 1);
          Serial.println("ID: " + id + ", İsim: " + isim);

          users[id.toInt()].is_valid = true;
          users[id.toInt()].can_open_door = true;
          snprintf(users[id.toInt()].name, sizeof(users[id.toInt()].name), "%s", isim.c_str());
        } else {
          Serial.println("Hatalı satır: " + satir);
        }
      }

      satirNumarasi++;
      if (satirNumarasi >= user_count) break;
    }

    file.close();
    Serial.println("Okuma tamamlandı.");
  }


  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("✓ Sensör bulundu!");
    int size = Serial.println(finger.templateCount);
    Serial.print("kayit sayisi: ");
    Serial.println(size);
  } 
  else {
    Serial.println("✗ Sensör bulunamadı!");
  }





  delay(1000);   // uno açılış mesajı için bekle

  WiFi.begin(ssid, password);
  Serial.println("Bağlanılıyor...");
  unoSerial.println("Wifi agi|taraniyor..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nIP adresi: ");
  Serial.println(WiFi.localIP());
  unoSerial.println("ip adresi:|" + WiFi.localIP().toString());

  // NTP sunucusunu ayarla
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  Serial.println("Zaman senkronize ediliyor...");
  
  //delay(5000);
  printLocalTime();













  client.setFingerprint(fingerprint); // Sertifika doğrulaması

  Serial.print("Sunucuya bağlanılıyor: ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Bağlantı başarısız!");
   // return;
  }

  

















 



  // ----------------------   HTTP API   --------------- 
 // server.getServer().setRSACert(&cert, &key);


  server.on("/",[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "text/html", html);  
  });




  server.on("/hi",[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "text/html", htmlPage);  
  });


  // userlari json şeklinde gosterir
  server.on("/users", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    server.send(200, "application/json", fetch_users());  
  });

  
  // giriş çıkış loglarını return eder
  server.on("/logs", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    server.send(200, "application/json", get_logs());  
  });
  

  // user erişim yetkileri burda düznelenir
  server.on("/user", HTTP_POST, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    server.send(200, "application/json", fetch_users());  
  });

  // kullanıcı silme işlemi
  server.on("/delete", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    if (server.hasArg("id")) {
      String id = server.arg("id");
      Serial.println("Silinecek ID: " + id);
      
      int key = delete_user(id.toInt());
      
      if (key == 1)
        server.send(200, "text/plain", "OK");
      else 
        server.send(200, "text/plain", "FAILED");
    }
  });

  server.on("/kayit", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    if (server.hasArg("name")) {
      String name = server.arg("name");

      uint8_t key = enrollFingerprint(name);

      if (key == 1) {
        server.send(200, "text/plain", "OK");
        delay(3000);
      }
      else {
        server.send(200, "text/plain", "FAILED");
        delay(3000);
      }

     // unoSerial.println("parmak okut");
      unoSerial.println("Giris modunda   |Parmak bekleniyor");
  
    }
  });
  

  // user erişim yetkileri burda düznelenir
  server.on("/lcd", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
      if (server.hasArg("msg")) {
        String msg = server.arg("msg");
        unoSerial.println(msg); 
        server.send(200, "text/plain", "OK");
      }
      else 
        server.send(200, "text/plain", "FAILED");
  });

  server.on("/format", HTTP_GET, []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      
      for (int i = 0; i < user_count; i++)
        users[i].is_valid = 0;
      
      if (SPIFFS.exists(usersFileName)) {
        SPIFFS.remove(usersFileName);
        Serial.println("users.txt silindi.");
      }
    
      if (SPIFFS.exists(logFileName)) {
        SPIFFS.remove(logFileName);
        Serial.println("logs.txt silindi.");
      }

      if (finger.emptyDatabase() == FINGERPRINT_OK) 
        server.send(200, "text/plain", "OK");
      else 
        server.send(200, "text/plain", "FAILED");
      
    });
  
  // durum bilgisini dondurur, şarj-durumu,giriş yapan_kişi
  server.on("/sarj", HTTP_GET, []() {

    int val = analogRead(A0);  // 0–1023 (0V–1V arası)
    Serial.print("Analog Değer: ");
    Serial.println(val);
  
    if (val > 200) {
      server.send(200, "text/plain", "YES");

    } 
    else {
      server.send(200, "text/plain", "NO");
    }
});




  server.begin();
  unoSerial.println("Giris modunda|Parmak okutunuz");

}




void loop() {
  

  server.handleClient();

  
  if (isUnlocked) {
    if (millis() - unlockTime >= kilit_suresi) {
      digitalWrite(selonoid, LOW); // kilidi artık kapat
      unoSerial.println("S_OFF");
      isUnlocked = false;
      unoSerial.println("Giris modunda|Parmak bekleniyor");
    }
  }
  else {
   
    unsigned long simdi = millis();

    if (simdi - sonKontrolZamani >= kontrolAraligi) {
      sonKontrolZamani = simdi;
      
      int id = getFingerprintID();


      // kayıtsız kullanıcı
      if (id == -1) {
        basarisiz_deneme++;
        log(-1);
     
        //3.a
        // alarmı devreye sok
        if (basarisiz_deneme == DENEME_HAKKI) {
          unoSerial.println("ALARM!!!");
        }
        // alarm çoktan devrede
        else if (basarisiz_deneme > DENEME_HAKKI) {
        }
        else {

            unoSerial.print("Kisi taninmadi!|Deneme: ");
            unoSerial.println(basarisiz_deneme);

            delay(3000);

            unoSerial.println("Giris modunda|Parmak okutun");
          }
      }
      // kayıtlı
      else if (id >= 0) {
          
        log(id);
        
        //3.a
        // acil durum protokolu aktif durumda, yalnızca yöneticiye izin var
        if (basarisiz_deneme > DENEME_HAKKI) {
          
            // sistemi normala dondur
            // alarmı sustur
            if (users[id].is_admin) {
              basarisiz_deneme = 0;
            }
        }
        else if (users[id].can_open_door) {
          basarisiz_deneme = 0;

          Serial.println("kapıyı aç");
          unoSerial.println("S_ON");      // kilidi aç
          unoSerial.print("Hosgeldin|");
          unoSerial.println(users[id].name);

          unlockTime = millis();       // açılma zamanını kaydet
          isUnlocked = true;
        }
      }
    }
  }
}



// 1 giriş, 0 çıkış
void log(int id) {
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeString[64];
  strftime(timeString, sizeof(timeString), "Tarih: %Y-%m-%d  Saat: %H:%M:%S", &timeinfo);

    
  // Log dosyasına yaz
  File logFile = SPIFFS.open(logFileName, "a");
  if (logFile) {
    if (id == -1) {
      logFile.print("-1,");
      logFile.println(timeString);
      logFile.close();
      Serial.println("Log dosyasına yazıldı.");
    }
    else {
      if (users[id].is_in)
        logFile.print("0,");
      else
        logFile.print("1,");
      
      logFile.print(id);
      logFile.print(",");
      logFile.println(timeString);
      logFile.close();
      Serial.println("Log dosyasına yazıldı.");
    } 
  } 
  else {
    Serial.println("Log dosyası açılamadı.");
  }
  
  users[id].is_in = ! users[id].is_in;
  
}



void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Zaman alınamadı");
    return;
  }

  char timeString[64];
  strftime(timeString, sizeof(timeString), "Tarih: %Y-%m-%d  Saat: %H:%M:%S", &timeinfo);
  Serial.println(timeString);
}




/*
  -2 -> hata
  -1  -> kayıtli değil
  or user_id
*/

int getFingerprintID() {
  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK) {
    //Serial.println("Parmak algılanmadı.");
    return -2;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    Serial.println("Parmak izi şablona çevrilemedi.");
    return -2;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("Eşleşen parmak izi bulunamadı.");
    return -1;
  }

  int id = finger.fingerID;


  int confidence = finger.confidence;

  Serial.print("Kayıt bulundu! ID: ");
  Serial.print(id);
  Serial.print(" Güven: ");
  Serial.println(confidence);

  return id;
}


/*  
  0 -> başarısız
  1 -> başarılı
  2 -> hafiza dolu
*/ 
uint8_t enrollFingerprint(String name) {
  int id = get_empty_index();
  Serial.print("index: ");
  Serial.println(id);
  if (id == -1) {
    Serial.println("kapasite dolu [!]");
    return 2;
  }
  int p = -1;
  
  unoSerial.print("Kayit modunda|Parmak okut");


  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) { Serial.println("Görüntü dönüştürülemedi."); return 0; }

  Serial.println("Parmaği kaldir...");
  unoSerial.println("Parmagi kaldir");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Ayni parmagi|tekrar yerlestir");
  unoSerial.println("Ayni parmagi|tekrar yerlestir");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("2. görüntü dönüştürülemedi.");
    unoSerial.println("2. görüntü|dönüstürülemedi");
    return 0; 
    }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Model oluşturulamadı.");
    unoSerial.println("Model |olusturulamadi");
    return 0;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
   
    users[id].is_valid = true;
    users[id].is_admin = false;
    users[id].can_open_door = true;
    snprintf(users[id].name, sizeof(users[id].name), "%s", name);
  
    
    Serial.println("✅ Parmak kaydedildi!");
    unoSerial.println("Parmak|kaydedildi");
    kaydet_id_ve_isim(id, String(id) +"," +  name);                // hafiza kartına da kaydet


    return 1;
  } 
  else {
    Serial.println("❌ Kaydetme başarısız.");
    unoSerial.println("Kaydetme|basarisiz");
    return 0;
  }
}



// user eklemek için user dizsindeki ilk boş yeri return eder
int get_empty_index() {
  for (int i = 0; i < 128; i++) {
    if (users[i].is_valid == false)
      return i;
  }

  return -1; // dizi tam dolu
}


/*
user dosyasında ilgili satira kaydeder
satir = id+1
text = id,isim
*/
void kaydet_id_ve_isim(int id, String text) {
  String lines[user_count];  // maksimum 500 satır destekliyor
  int lineCount = 0;

  // 1. aşama: mevcut dosyayı oku
  File file = SPIFFS.open(usersFileName, "r");
  if (file) {
    while (file.available()) {
      lines[lineCount] = file.readStringUntil('\n');
      lines[lineCount].trim();
      lineCount++;
    }
    file.close();
  } else {
    Serial.println("Dosya açılamadı (okuma aşaması).");
    return;
  }

  // 2. aşama: boş satırları doldur
  int satir_numarasi = id + 1; // çünkü 0. id -> 1. satır demek
  if (lineCount < satir_numarasi) {
    for (int i = lineCount; i < satir_numarasi; i++) {
      lines[i] = "";
    }
    lineCount = satir_numarasi;
  }

  // 3. aşama: ilgili satırı değiştir
  int index = satir_numarasi - 1; // dizide 0 tabanlı
  lines[index] = text;

  // 4. aşama: dosyayı baştan yaz
  file = SPIFFS.open(usersFileName, "w");
  if (file) {
    file.seek(0);
    file.truncate(0); // eski içeriği sil

    for (int i = 0; i < lineCount; i++) {
      file.println(lines[i]);
    }

    file.close();
    Serial.println("Yazma tamam.");
  } else {
    Serial.println("Dosya açılamadı (yazma aşaması).");
  }
}





// direkt sil geç, yoksa da yok
int delete_user(uint8_t id) {

  kaydet_id_ve_isim(id, "");

  uint8_t result = finger.deleteModel(id);
  if (result == FINGERPRINT_OK) {
    
     // isim verisini de kalıcı sil
    users[id].is_valid = false;
    snprintf(users[id].name, sizeof(users[id].name), "%s", "");
    Serial.println("Kayıt silindi.");
    return 1;
  } else if (result == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("İletişim hatası.");
  } else if (result == FINGERPRINT_BADLOCATION) {
    Serial.println("ID bulunamadı.");
  } else if (result == FINGERPRINT_FLASHERR) {
    Serial.println("Silme hatası (flash).");
  } else {
    Serial.println("Bilinmeyen hata.");
  }
  
  return 0;
}



/*
  kayıtlı tüm kullanıcıları JSON şeklinde return eder
*/
String fetch_users_eski() {

  // JSON oluştur
  StaticJsonDocument<200> doc;

  for (int id = 0; id < user_count; id++) 
    if (users[id].is_valid == 1) 
      doc[String(id)] =  users[id].name;
    
  // JSON string olarak yaz
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;
}


String fetch_users() {
  StaticJsonDocument<1024> doc;  // Gerekirse artır

  JsonArray root = doc.to<JsonArray>();

  for (int i = 0; i < user_count; i++) {
    if (users[i].is_valid == 1) {
      JsonArray userData = root.createNestedArray();
      userData.add(i);                       // [0]
      userData.add(users[i].name);           // [1]
      userData.add(users[i].is_admin);       // [2]
      userData.add(users[i].can_open_door);  // [3]
      userData.add(users[i].is_in);          // [4]
    }
  }

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}












/*
logs.txt dosyasının return eder 
giriş çıkış loglarını return eder
*/
String get_logs() {
  File file = SPIFFS.open(logFileName, "r");
  if (!file) {
    Serial.println("Dosya açılamadı: " + String(logFileName));
    return "";
  }

  String content = "";
  while (file.available()) {
    content += file.readStringUntil('\n') + "\n"; // Satır satır okuyup ekliyor
  }
  file.close();
  Serial.println("logs:");
  Serial.println(content);
  return content;
}
