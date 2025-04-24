#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServerSecure.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "config.h"


SoftwareSerial mySerial(D5, D6); // RX, TX (GPIO14, GPIO12)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

ESP8266WebServerSecure server(443);
BearSSL::X509List cert(serverCert);
BearSSL::PrivateKey key(serverKey);
WebSocketsServer webSocket = WebSocketsServer(81);  // WebSocket 81. portta



unsigned long sonKontrolZamani = 0;
const unsigned long kontrolAraligi = 1000;     // parmak izi sensorü çalışma frekansı
const unsigned long kilit_suresi = 5000;       // kilidin açık kalma süresi
uint8_t basarisiz_deneme = 0;                  // üstüste yapilan başarısız deneme sayisi
const uint8_t DENEME_HAKKI = 100;               //          


// Basit HTML arayüz
const char htmlPage[] PROGMEM = R"rawliteral(
<h1> hello world </h1>
)rawliteral";
  

int selonoid = D2;


// bu bilgilerin silinmemesi gerek, sd carda kaydetmek gerekebilir
// yada bunları kullanmak yerine her seferde parmak izi sensorunden elde etmek gerek kullanıcı id lerini
// isimler her harükalde kaydedilmek zorunda

struct User {
  uint8_t is_valid : 1;
  uint8_t is_admin : 1;
  uint8_t can_open_door : 1;
  uint8_t is_in : 1;
  uint8_t reserved : 4;
  char name[20];       // Max 30 karakter + null terminator
};

User users[100];




void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(selonoid, OUTPUT);
  digitalWrite(selonoid, LOW);

  WiFi.begin(ssid, password);


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



  Serial.println("Bağlanılıyor...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nIP adresi: ");
  Serial.println(WiFi.localIP());

  server.getServer().setRSACert(&cert, &key);




  // ----------------------   HTTP API   --------------- 

  server.on("/",[](){
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send_P(200, "text/html", htmlPage);  
  });

  // userlari json şeklinde gosterir
  server.on("/users", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
    server.send(200, "application/json", fetch_users());  
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
        delay(2000);
      }
      else
        server.send(200, "text/plain", "FAILED");
    }
  });



  server.begin();


  // WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket sunucusu başladı.");
  
}


void loop() {

  webSocket.loop();
  server.handleClient();
  
  if(Serial.available() > 0) { 
    //webSocket.broadcastTXT(c, sizeof(c));

    String gelen_veri = Serial.readStringUntil('\n'); // Enter ile biten veriyi oku
    gelen_veri.trim(); // Baştaki ve sondaki boşlukları temizle
    
  }

  // -------------------------
  unsigned long simdi = millis();

  if (simdi - sonKontrolZamani >= kontrolAraligi) {
    sonKontrolZamani = simdi;
    
    int id = getFingerprintID();

    // hatalı okuma
    if (id == -2) {

    }
    // kayıtsız kullanıcı
    else if (id == -1) {
        basarisiz_deneme++;

        //3.a
        // alarmı devreye sok
        if (basarisiz_deneme == DENEME_HAKKI) {
          basarisiz_deneme++;
        }
        else if (basarisiz_deneme > DENEME_HAKKI) {
        }

        delay(2000);

    }
    // kayıtlı
    else if (id >= 0) {
        
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
          Serial.println("kilit açılıyor");
          digitalWrite(selonoid, HIGH);
          delay(kilit_suresi);
          digitalWrite(selonoid, LOW);
      }
    }
  }
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
  Serial.println("Parmak yerleştir...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) { Serial.println("Görüntü dönüştürülemedi."); return 0; }

  Serial.println("Parmağı kaldır...");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Aynı parmağı tekrar yerleştir...");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) { Serial.println("2. görüntü dönüştürülemedi."); return 0; }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) { Serial.println("Model oluşturulamadı."); return 0; }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
   
    users[id].is_valid = true;
    users[id].is_admin = false;
    users[id].can_open_door = true;
    snprintf(users[id].name, sizeof(users[id].name), "%s", name);
  
    
    Serial.println("✅ Parmak kaydedildi!");
    return 1;
  } 
  else {
    Serial.println("❌ Kaydetme başarısız.");
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



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
	if (type == WStype_TEXT) {
		Serial.println("----");
  

    // JSON metnini string olarak al
    String gelen_veri = String((char*)payload);
    Serial.printf("Gelen mesaj: %s\n", gelen_veri.c_str());


    // JSON belgesini oluşturmak için yer aç (statik veya dinamik kullanılabilir)
    StaticJsonDocument<200> doc;

    // parse et (metni JSON'a çevir)
    DeserializationError error = deserializeJson(doc, gelen_veri);

    if (error) {
      Serial.print("JSON HATA: ");
      Serial.println(error.f_str());
      return;
    }

    // Artık JSON içeriğini kullanabilirsin
    String command = doc["command"]; 
    Serial.println("command: " + command);



	
    if(command.equals("led1")) {
      Serial.println("ledi yak");
      digitalWrite(LED_BUILTIN, LOW);
      webSocket.broadcastTXT("ledi açtım");
    }

    else if(command.equals("led0")) {
      Serial.println("ledi sodur");
      digitalWrite(LED_BUILTIN, HIGH);  
      webSocket.broadcastTXT("led kapali artık"); 
    }

    //-------------- kayitli kullanıcı sayısını getir
    else if (command.equals("sayi")) {
      int size = finger.templateCount;
      String mesaj = "size:" + String(size);
      webSocket.broadcastTXT(mesaj);
    }

    // ---------------- parmak izini formatla, tüm kayıtları sil
    else if (command.equals("format")) {
      if (finger.emptyDatabase() == FINGERPRINT_OK) {
        Serial.println("Tüm kayıtlar silindi!");
        webSocket.broadcastTXT("Tüm kayıtlar silindi!");
      } else {
        Serial.println("format işlemi başarısız.  [!]");
        webSocket.broadcastTXT("format işlemi başarısız [!]");
      }
    }
  
  }
}


// direkt sil geç, yoksa da yok
int delete_user(uint8_t id) {

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
String fetch_users() {

  // JSON oluştur
  StaticJsonDocument<200> doc;

  for (int id = 0; id < 128; id++) {
    if (users[id].is_valid == 1) {
      doc[String(id)] =  users[id].name;
    }
  }


  // JSON string olarak yaz
  String jsonString;
  serializeJson(doc, jsonString);
  
  return jsonString;


}
