/*********************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *********************
  This example runs directly on ESP32 chip.

  NOTE: This requires ESP32 support package:
    https://github.com/espressif/arduino-esp32

  Please be sure to select the right ESP32 module
  in the Tools -> Board menu!

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *********************/


 /*********************
  Widget Blynk:

  V0 (Jarak): Jarak yang dihitung oleh HC-SR04.
  V1 (Kedalaman Air): Level air secara numerik.
  V2 (Status Kedalaman): Status level air, seperti "High", "Medium", atau "Low/Very Low".
  V3 widget button untuk override buzzer. -> switch
  V4 widget slider untuk override posisi bendungan. -> 0-180
  V5 widget button untuk mematikan override bendungan. -> switch
 *********************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL62b1Jr472"
#define BLYNK_TEMPLATE_NAME "ESP32 Wifi"
#define BLYNK_AUTH_TOKEN "VFoROZpoq0luAKm7gVGzZP-pvUYboSL-"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>


// Pin untuk Sistem Pengusiran Hama (HC-SR04 & 5V Buzzer)
const int echoPin = 27;    // GPIO 27
const int triggerPin = 14; // GPIO 14
const int buzzerPin = 23;  // GPIO 23

// Pin untuk Sistem Irigasi
const int kedalamanPin = 25; // pin ADC 25
Servo bendungan;             // servo control
const int servoPin = 4;     // GPIO 4


// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Projek";  // Input nama wifi.
char pass[] = "MM12345678";     // Input password wifi.


// Variabel untuk Sistem Pengusiran Hama
int durasi; // Waktu perjalanan gelombang ultrasonik dari sensor ke objek dan kembali lagi (pulsa pantulan) dalam mikrosekon.
int jarak; // Jarak antara sensor ultrasonik dan objek.

// Variabel untuk Sistem Irigasi
int MaxLevel = 4095;     // Maksimal nilai ADC (12-bit ESP32)
int PanjangSensor = 10;  // Panjang SEN 18 Water Level (cm)

int Level[5] = {
  (MaxLevel * 75) / 100, // 0 High level (75%)
  (MaxLevel * 65) / 100, // 1 Moderate high level (65%)
  (MaxLevel * 55) / 100, // 2 Medium level (55%)
  (MaxLevel * 45) / 100, // 3 Moderate low level (45%)
  (MaxLevel * 35) / 100  // 4 Low level (35%)
};

// Variabel untuk Override Sistem (dari otomatis ke manual)
bool buzzerOverride = false;    // False = mode otomatis, True = override
bool bendunganOverride = false; // False = mode otomatis, True = override
int manualBendunganPos = 0;     // Posisi manual bendungan (0–180)


void setup()
{
  // Debug console
  Serial.begin(115200); // komunikasi serial dengan baudrate 115200.

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // inisialisasi koneksi ke server blynk.

  // Setup Pin Sistem Pengusiran Hama
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Setup Servo Sistem Irigasi
  bendungan.attach(servoPin);
  bendungan.write(0); // Menutup bendungan.
}


// Override buzzer, user dapat menyalakan buzzer manual
BLYNK_WRITE(V3) 
{ 
  buzzerOverride = param.asInt(); // mengambil input dari widget,  0 = off, 1 = on
  if (buzzerOverride) 
  {
    digitalWrite(buzzerPin, HIGH); // Nyalakan buzzer
    Serial.println("Override: Buzzer ON");
  } 
  else 
  {
    digitalWrite(buzzerPin, LOW);  // Matikan buzzer
    Serial.println("Override: Buzzer OFF");
  }
}

// Override bendungan, user dapat mengatur servo bendungan
BLYNK_WRITE(V4) 
{ 
  if(bendunganOverride)
  {
    manualBendunganPos = param.asInt();   // Input nilai widget slider, 0-180*
    bendungan.write(manualBendunganPos);  // Atur posisi bendungan
    Serial.print("Override: Bendungan manual ke posisi ");
    Serial.println(manualBendunganPos);
  }           
}

// Switch override bendungan
BLYNK_WRITE(V5) 
{ 
  bendunganOverride = param.asInt();
}


void MengusirHama()
{
  // Membersihkan pin trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Kirim sinyal HIGH selama 10 mikrodetik.
  // HC-SR04 memerlukan pulsa trigger 10 mikrodetik agar dapat memulai pengiriman gelombang ultrasonik. (kata gpt)
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Menghitung durasi pantulan dalam mikrosekon.
  durasi = pulseIn(echoPin, HIGH); // Saat pin echo high, waktu mulai dihitung.
  jarak = durasi * 0.034 / 2; // 0.034 itu dari 340 m/s diubah jadi 0.034 cm/mikrosekon.

  // Kirim ke serial monitor arduino.
  Serial.print("Jarak: ");
  Serial.print(jarak);
  Serial.println(" cm");

  Blynk.virtualWrite(V0, jarak); // Mengirim jarak ke widget gauge blynk.

  if(!buzzerOverride)
  {
    // Logika menyalakan Buzzer
    // Kalo jarak > 50 cm, maka ada hama.
    if(jarak < 50)
    {
      digitalWrite(buzzerPin, HIGH);
    }
    else
    {
      digitalWrite(buzzerPin, LOW);
    }
  }
}


void CekKedalamanAir()
{
  int nilaiADC = analogRead(kedalamanPin); // ADC
  float kedalamanAir = (nilaiADC * PanjangSensor) / 4095; // Mengubah nilai ADC ke kedalaman air (cm)

  // Tampilkan nilai level air di serial monitor arduino dan blynk.
  Serial.print("Kedalaman air (ADC): ");
  Serial.println(kedalamanAir);
  Blynk.virtualWrite(V1, kedalamanAir); // Mengirim kedalaman air ke widget gauge blynk.

  if(!bendunganOverride)
  {
    if(kedalamanAir >= Level[0])
    {
      Serial.println("Kedalaman : High");
      bendungan.write(180); // bendungan terbuka
      Blynk.virtualWrite(V2, "High"); // Mengirim status kedalaman air ke widget label value blynk.
    }
    else if (kedalamanAir >= Level[1])
    {
      Serial.println("Kedalaman : Moderate High");
      bendungan.write(135); // bendungan lumayan terbuka 
      Blynk.virtualWrite(V2, "Moderate High"); // Mengirim status kedalaman air ke widget label value blynk.
    }
    else if (kedalamanAir >= Level[2])
    {
      Serial.println("Kedalaman : Medium");
      bendungan.write(90); // bendungan terbuka setengah
      Blynk.virtualWrite(V2, "Medium"); // Mengirim status kedalaman air ke widget label value blynk.
    }
    else if (kedalamanAir >= Level[3])
    {
      Serial.println("Kedalaman : Moderate Low");
      bendungan.write(45); // bendungan sedikit terbuka
      Blynk.virtualWrite(V2, "Moderate Low"); // Mengirim status kedalaman air ke widget label value blynk.
    }
    else if (kedalamanAir >= Level[4])
    {
      Serial.println("Kedalaman : Low");
      bendungan.write(0); // bendungan tutup
      Blynk.virtualWrite(V2, "Low"); // Mengirim status kedalaman air ke widget label value blynk.
    }
    else
    {
      Serial.println("Kedalaman : Very Low");
      bendungan.write(0); // bendungan tutup
      Blynk.virtualWrite(V2, "Very Low"); // Mengirim status kedalaman air ke widget label value blynk.
    } 
  }
}

void loop()
{
  Blynk.run(); // menjaga komunikasi dengan server blynk.
  MengusirHama();
  CekKedalamanAir();
  delay(50); // delay 50MS detik
}