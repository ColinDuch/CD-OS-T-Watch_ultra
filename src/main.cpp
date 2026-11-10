#include <Arduino.h>   // le socle Arduino : setup(), loop(), Serial, delay()...

void setup() {
  Serial.begin(115200);   // ouvre le port série à 115200 bauds
  delay(2000);            // laisse le temps au PC de détecter le port USB
  Serial.println("Boot de MonOS...");
  Serial.printf("PSRAM : %lu octets\n", ESP.getPsramSize());  // vérifie la PSRAM
  // À TOI : ajoute une ligne qui affiche la RAM interne libre (indice : ESP.get...Heap())
}

void loop() {
  static uint32_t tick = 0;   // static = garde sa valeur d'un tour de loop à l'autre
  Serial.printf("Tick %lu\n", tick++);
  delay(1000);                // attend 1 seconde
}