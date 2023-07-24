#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NewPing.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "********************************"
#define FIREBASE_AUTH "********************************"
#define WIFI_SSID "**********"
#define WIFI_PASSWORD "**********"

FirebaseData firebaseData;

#define TRIGGER_PIN 17 // HCSR04 sensörünün trigger pini
#define ECHO_PIN 16    // HCSR04 sensörünün echo pini
#define MAX_DISTANCE 50 // HCSR04 sensörünün algılama mesafesi (cm)
#define MIN_DISTANCE 1  // Minimum algılama mesafesi (cm)
#define DURATION 10000  // Durumun "T" olarak ayarlı kalacağı süre (ms) (10 saniye = 10000 milisaniye)

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  timeClient.begin();
  timeClient.setTimeOffset(3 * 3600);  // Saat dilimini ayarlayabilirsiniz (örnekte 3 saat ileri ayarlandı)
  
}

void loop()
{
  if (Firebase.getString(firebaseData, "/Plaka/A1"))
  {
    if (firebaseData.dataType() == "string")
    {
      String stringValue = firebaseData.stringData();

      timeClient.update();
      
      if (Firebase.getString(firebaseData, "/Rezervasyon/" + stringValue + "/Çıkış Saati"))
      {
        if (firebaseData.dataType() == "string")
        {
          String cikissaati = firebaseData.stringData();
          Serial.print("ÇIKIŞ SAATİ: ");
          Serial.println(cikissaati);
          timeClient.update();
          int hour = timeClient.getHours();
          int minute = timeClient.getMinutes();

          String hourStr = (hour < 10) ? "0" + String(hour) : String(hour);
          String minuteStr = (minute < 10) ? "0" + String(minute) : String(minute);

          String timeString1 = hourStr + ":" + minuteStr;

          if (cikissaati == timeString1 )    // || cikissaati >= timeString1) BUNU DENİCEZ
          {
            if (Firebase.getString(firebaseData, "/Rezervasyon/" + stringValue + "/Durum"))
            {
              if (firebaseData.dataType() == "string")
              {
                String status = firebaseData.stringData();
                Serial.print("DURUM: ");
                Serial.println(status);

                unsigned int distance = sonar.ping_cm(); // HCSR04 sensörü ile mesafeyi ölç

                Serial.print("SENSOR MESAFE:");
                Serial.println(distance);

                if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE)
                {
                  Serial.print("SENSOR BARİYER VERİ:");
                  Serial.println("DİKKAT ARAÇ VAR"); // Cisim algılandığında "MERT" yazdır
                  Firebase.setString(firebaseData, "/Rezervasyon/" + stringValue + "/Sensör Bariyer", "DİKKAT ARAÇ VAR");

                  if (status != "T") // Eğer Durum "T" değilse
                  {
                    Firebase.setString(firebaseData, "/Rezervasyon/" + stringValue + "/Durum", "T");
                  }
                }
                else
                {
                  Serial.print("SENSOR BARİYER VERİ:");
                  Serial.println("F"); // Cisim algılanmadığında "F" yazdır
                  Firebase.setString(firebaseData, "/Rezervasyon/" + stringValue + "/Sensör Bariyer", "F");

                  if (status == "T") // Eğer Durum "T" ise
                  {
                    unsigned long currentTime = millis();
                    unsigned long previousTime = currentTime;
                    while (currentTime - previousTime < DURATION) // Belirtilen süre (10 saniye) boyunca beklet
                    {
                      currentTime = millis();
                      // Gerekirse diğer işlemleri burada yapabilirsiniz
                    }
                    Firebase.setString(firebaseData, "/Rezervasyon/" + stringValue + "/Durum", "F");
                  }
                }
              }

              delay(100); // Ölçümler arasında bir gecikme vermek için
            }
          }
        }
      }
    }
  }
}
