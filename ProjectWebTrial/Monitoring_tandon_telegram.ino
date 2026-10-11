#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// --- PENGATURAN WIFI & TELEGRAM ---
const char* ssid = "TBV_Backup";
const char* password = "nasigorengrempah";
#define BOT_TOKEN "8905356458:AAHYixcHfmYKqGQP_QrWlpf5UfGdUWk3WIw"
#define CHAT_ID "5229511290" // Ganti dengan ID angka Anda

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// --- PIN & KALIBRASI ---
#define TRIG_PIN D1
#define ECHO_PIN D2
const int TINGGI_TANDON_TOTAL = 30; 
const int BATAS_AIR_PENUH = 5;      
const int BATAS_KRITIS_PERSEN = 20; 

bool notifikasiTerkirim = false;
int persentaseAirSaatIni = 0;
int tinggiAirSaatIni = 0;

int botRequestDelay = 1000;
unsigned long lastTimeBotRan = 0;

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "/cek" || text == "cek" || text == "Cek") {
      String pesan = "💧 *STATUS TANDON SAAT INI*\n\n";
      pesan += "Ketinggian Air: " + String(tinggiAirSaatIni) + " cm\n";
      pesan += "Kapasitas: *" + String(persentaseAirSaatIni) + "%*";
      bot.sendMessage(chat_id, pesan, "Markdown");
    }
    
    if (text == "/start") {
      bot.sendMessage(chat_id, "Halo! Saya Bot Tandon Air Anda.\nKetik /cek untuk status air.", "");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  client.setInsecure(); // Matikan cek sertifikat agar memori lega
  
  Serial.print("\nMenghubungkan ke WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println("\nWiFi Terhubung!");
  Serial.println("Mencoba menghubungi Server Telegram...");
  
  // Tes kirim pesan dan tangkap hasilnya
  bool terkirim = bot.sendMessage(CHAT_ID, "✅ Sistem Monitoring Tandon Aktif!", "");
  
  if (terkirim) {
    Serial.println("SUKSES! Pesan terkirim ke Telegram.");
  } else {
    Serial.println("ERROR: Gagal mengirim pesan ke Telegram!");
  }
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // SOLUSI HANG: Batas waktu 100ms agar program tidak terkunci
  long duration = pulseIn(ECHO_PIN, HIGH, 100000); 
  int distance = duration * 0.034 / 2;

  if (distance > 0 && distance <= 400) {
    tinggiAirSaatIni = TINGGI_TANDON_TOTAL - distance;
    if (tinggiAirSaatIni < 0) tinggiAirSaatIni = 0;

    persentaseAirSaatIni = map(distance, TINGGI_TANDON_TOTAL, BATAS_AIR_PENUH, 0, 100);
    if (persentaseAirSaatIni < 0) persentaseAirSaatIni = 0;
    if (persentaseAirSaatIni > 100) persentaseAirSaatIni = 100;

    Serial.print("Jarak: "); Serial.print(distance);
    Serial.print(" cm | Kapasitas: "); Serial.print(persentaseAirSaatIni); Serial.println("%");

    // Kirim notifikasi air habis
    if (persentaseAirSaatIni <= BATAS_KRITIS_PERSEN && !notifikasiTerkirim) {
      if (bot.sendMessage(CHAT_ID, "⚠️ *PERINGATAN!* Air tandon hampir habis: " + String(persentaseAirSaatIni) + "%", "Markdown")) {
        notifikasiTerkirim = true;
      }
    } 
    else if (persentaseAirSaatIni > BATAS_KRITIS_PERSEN + 5) { 
      notifikasiTerkirim = false; 
    }
  } else {
    // Jika sensor gagal membaca, program tidak akan crash
    Serial.println("Sensor tidak terbaca / di luar jangkauan.");
  }

  // Cek pesan Telegram masuk setiap 1 detik
  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  delay(100); // Beri waktu istirahat sejenak untuk proses WiFi
}