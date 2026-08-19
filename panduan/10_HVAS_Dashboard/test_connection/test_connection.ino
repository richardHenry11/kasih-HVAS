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

    Serial.println();
    Serial.println("====================");
    Serial.println("RS485 READY");
    Serial.println("====================");
}

void loop()
{
    Serial2.println("test");

    Serial.println("TX -> test");

    delay(1000);
}