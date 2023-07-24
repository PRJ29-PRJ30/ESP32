#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "******************************************"
#define FIREBASE_AUTH "**************************"
#define WIFI_SSID "********"
#define WIFI_PASSWORD "********"

Servo myservo;  // Servo nesnesi oluşturun
int servoPin = 14;  // Servo motorunun bağlı olduğu pin numarası

FirebaseData firebaseData;
int pos = 0;    // Servo pozisyonunu saklamak için değişken
bool initialized = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  myservo.setPeriodHertz(50);    // Standart 50 Hz servo
  myservo.attach(servoPin); // Servo motorunun pinine bağlanın
  timeClient.begin();
  timeClient.setTimeOffset(3 * 3600);  // Saat dilimini ayarlayabilirsiniz (örnekte 3 saat ileri ayarlandı)

}

void loop() {
  if (!initialized) {
    myservo.write(pos);
    delay(1000);
    initialized = true;
  }
  
  if (Firebase.getString(firebaseData, "/Plaka/A1")) {
    if (firebaseData.dataType() == "string") {
      String stringValue = firebaseData.stringData();
      Serial.print("Received string value: ");
      Serial.println(stringValue);
      if (Firebase.getString(firebaseData, "/Rezervasyon/" + stringValue + "/Giriş Saati")) {
        if (firebaseData.dataType() == "string") {
          String stringValue2 = firebaseData.stringData();
          Serial.println(stringValue2);
          timeClient.update();

          int hour = timeClient.getHours();
          int minute = timeClient.getMinutes();

          String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
          String minuteStr = (minute < 10) ? "0" + String(minute) : String(minute);

          String timeString1 = hourStr + ":" + minuteStr;
          if (stringValue2 == timeString1 || stringValue2 <= timeString1) {
            if (Firebase.getString(firebaseData, "/Rezervasyon/" + stringValue + "/Durum")) {
              if (firebaseData.dataType() == "string") {
                String stringValue1 = firebaseData.stringData();
                Serial.println(stringValue1);
                if (stringValue1 == "F") {
                  if (pos < 130) {
                    pos += 20;
                    if (pos > 130) {
                      pos = 130;
                    }
                  }
                } else if (stringValue1 == "T") {
                  if (pos > 20) {
                    pos -= 20;
                    if (pos < 20) {
                      pos = 20;
                    }
                  }
                }
                myservo.write(pos);
                delay(10);
              }
            }
          }
        }
      }
      if (Firebase.getString(firebaseData, "/Rezervasyon/" + stringValue + "/Çıkış Saati")) {
        if (firebaseData.dataType() == "string") {
          String stringValue1 = firebaseData.stringData();
          Serial.println(stringValue1);
          timeClient.update();

          int hour = timeClient.getHours();
          int minute = timeClient.getMinutes();

          String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
          String minuteStr = (minute < 10) ? "0" + String(minute) : String(minute);

          String timeString = hourStr + ":" + minuteStr;
          if (stringValue1 == timeString) {
            Firebase.setString(firebaseData, "/Rezervasyon/" + stringValue + "/Durum", "F");
          }
        }
      }
    }
  }
  delay(1000);
}
