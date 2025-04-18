#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

SoftwareSerial mySerial(D5, D6); // RX, TX (GPIO14, GPIO12)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);  // WebSocket 81. portta


// bu bilgilerin silinmemesi gerek, sd carda kaydetmek gerekebilir
// yada bunları kullanmak yerine her seferde parmak izi sensorunden elde etmek gerek kullanıcı id lerini
// isimler her harükalde kaydedilmek zorunda
uint8_t users[128] = {0};          // user listesi
String name_list[128];            



unsigned long sonKontrolZamani = 0;
const unsigned long kontrolAraligi = 1000;     // parmak izi sensorü çalışma frekansı
const unsigned long kilit_suresi = 5000;       // kilidin açık kalma süresi


// WiFi ağ bilgilerini buraya yaz
const char* ssid = "es";
const char* password = "12345678";


// Basit HTML arayüz
// buraya nodemcu nun içine gömmek istediğimiz static html kodu yazılıcak
// ama şuanlık debug yaptığımız için gerek yok, projenin bitiminde koyucaz
const char htmlPage[] PROGMEM = R"rawliteral(

<h1> hello world </h1>

  )rawliteral";
  

int selonoid = D2;



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(selonoid, OUTPUT);
  digitalWrite(selonoid, LOW);

  Serial.begin(9600);
  
  delay(100);
  Serial.println("R307 parmak izi sensörü başlatılıyor...");
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


  // wifi
  WiFi.begin(ssid, password);
  delay(100);

  // Bağlantı sağlanana kadar bekle
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Bağlandı!");
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.localIP());


  // HTTP server
  server.on("/",[](){
    server.send_P(200, "text/html", htmlPage);  
    server.sendHeader("Access-Control-Allow-Origin", "*");
  });

  // userlari json şeklinde gosterir
  server.on("/users",[](){
    server.send(200, "application/json", fetch_users());  
    server.sendHeader("Access-Control-Allow-Origin", "*"); // CORS izni
  });

  server.on("/delete", HTTP_POST, []() {
    if (server.hasArg("id")) {
      String id = server.arg("id");
      Serial.println("Silinecek ID: " + id);
      
      String name = name_list[id.toInt()];

      delete_user(id.toInt());
      // Burada id'ye göre kayıt silme işlemi yapılabilir
      server.send(200, "text/plain", "ID " + id + " : " + name + " silindi.");
    } else {
      server.send(400, "text/plain", "ID parametresi eksik!");
    }
  });
  
  server.begin();
  Serial.println("HTTP sunucusu başladı.");
  
  
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
    
    Serial.print("Alınan veri: ");
    Serial.println(gelen_veri);

    // yeni kayit ekle
    if (gelen_veri == "kayit" || gelen_veri.equals("kayit")) {
      int size = Serial.println(finger.templateCount);
      Serial.print("size: ");
      Serial.println(size);
   //   enrollFingerprint();
    }
    // kayitli kullanıcı sayısını getir
    else if (gelen_veri.equals("sayi")) {
      int size = Serial.println(finger.templateCount);
      Serial.print("size: ");
      Serial.println(size);
    }

  }

  // -------------------------
  unsigned long simdi = millis();
  if (simdi - sonKontrolZamani >= kontrolAraligi) {
    sonKontrolZamani = simdi;
    
      int id = getFingerprintID();

      // kayit mevcut
      if (id != -1) {
        Serial.print("kayıt mevcut:  ");
        Serial.println(id);
        digitalWrite(selonoid, HIGH);
        delay(kilit_suresi);
        digitalWrite(selonoid, LOW);
      } 
      
  }



}


int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  int id = finger.fingerID;


  // parmak bulundu fakat users listesinde de olması gerek
  // hafıza kartı entegrasyonu yaptıktan sonra burayi silebilirsin
  if (users[id] == 0) return -1;

  return id;
}


/*  
  0 -> başarısız
  1 -> başarılı
  2 -> hafiza dolu
*/ 
uint8_t enrollFingerprint(String name) {
  int id = get_empty_index();
  Serial.print("size: ");
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
    Serial.println("✅ Parmak kaydedildi!");
    users[id] = 1;
    name_list[id] = name;
    return 1;
  } else {
    Serial.println("❌ Kaydetme başarısız.");
    return 0;
  }

}



// user eklemek için user dizsindeki ilk boş yeri return eder
int get_empty_index() {
  for (int i = 0; i < 128; i++) {
    if (users[i] == 0)
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
    //String data = doc["data"];     


    Serial.println("command: " + command);
    //Serial.println("data: " + data);




	
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
     //------------- yeni kayit ekle
     if (command.equals("kayit")) {
      String name = doc["name"];     
      int status = enrollFingerprint(name);
      // kayit başarılı
      if (status == 1) {
        webSocket.broadcastTXT("kayit başarili");
        delay(2000);  // 2 saniye bekle
      }
      // başarısız
      else if (status == 0) {
        webSocket.broadcastTXT("kayit başarısız oldu");
      }
      // hafiza dolu hatasi
      else if (status == 2) {
        webSocket.broadcastTXT("hafiza dolu");
      }
    }
    //-------------- kayitli kullanıcı sayısını getir
    else if (command.equals("sayi")) {
      int size = finger.templateCount;
      String mesaj = "size:" + String(size);
      webSocket.broadcastTXT(mesaj);
    }
    // ------------------ kullanıcıyı sil
    else if (command.equals("delete")) {
      int id  = doc["id"];

      uint8_t result = finger.deleteModel(id);
      if (result == FINGERPRINT_OK) {
        Serial.println("Kayıt silindi.");
      } else if (result == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("İletişim hatası.");
      } else if (result == FINGERPRINT_BADLOCATION) {
        Serial.println("ID bulunamadı.");
      } else if (result == FINGERPRINT_FLASHERR) {
        Serial.println("Silme hatası (flash).");
      } else {
        Serial.println("Bilinmeyen hata.");
      }
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
void delete_user(uint8_t id) {

  uint8_t result = finger.deleteModel(id);
  if (result == FINGERPRINT_OK) {
    Serial.println("Kayıt silindi.");
  } else if (result == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("İletişim hatası.");
  } else if (result == FINGERPRINT_BADLOCATION) {
    Serial.println("ID bulunamadı.");
  } else if (result == FINGERPRINT_FLASHERR) {
    Serial.println("Silme hatası (flash).");
  } else {
    Serial.println("Bilinmeyen hata.");
  }
  
  users[id] = 0;
 
}


String fetch_users() {


  // JSON oluştur
  StaticJsonDocument<200> doc;

  for (int i = 0; i < 128; i++) {
    if (users[i] != 0) {
      doc[String(i)] =  name_list[i];
    
    }
  }


  // JSON string olarak yaz
  String jsonString;
  serializeJson(doc, jsonString);
    
  Serial.println("JSON Gönderiliyor:");
  Serial.println(jsonString);

  return jsonString;


}