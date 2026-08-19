
#define RS485_TX 16
#define RS485_RX 15

void setup()
{
  Serial.begin(115200);

  Serial2.begin(
      115200,
      SERIAL_8N1,
      RS485_RX,
      RS485_TX);

  Serial.println("====================");
  Serial.println("RS485 READY");
  Serial.println("====================");
}

void loop()
{
  // kirim ke ATmega
  Serial2.println("{\"cmd\":\"get_pzem\"}");

  // tampilkan di USB
  Serial.println("TX -> PZEM");

  // kalau ATmega balas
  while (Serial2.available())
  {
    char c = Serial2.read();
    Serial.write(c);
  }

  delay(1000);
}