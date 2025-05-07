#include <Arduino.h>
#include <HardwareSerial.h>
HardwareSerial RS485(1); // Use UART1
byte packet[] = {
    0x0b, // Sign address
    0xa2, // Always a2
    0xd0, // Display width
    0x70, // 112px
    0xd1, // Display height
    0x10, // 0px
    0xd2, // Horizontal offset
    0x00, // 0px right
    0xd3, // Vertical offset
    0x00, // 0px down
    0xD4, // Font
    0x60, // 13px font
    0x45, // E
    0x58, // X
    0x41, // A
    0x4d, // M
    0x50, // P
    0x4c, // L
    0x45  // E
};
int length = sizeof(packet) / sizeof(packet[0]);
byte calculateChecksum(byte byteArray[], int length)
{
  unsigned int sum = 0;
  for (int i = 0; i < length; i++)
  {
    sum += byteArray[i];
  }
  return (byte)(sum % 256);
}
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED off
  // Initialize UART1: 4800 baud, 8N1, RX=GPIO6, TX=GPIO7
  RS485.begin(4800, SERIAL_8N1, 6, 7);
  delay(3000);                 // Give sign time to reset
  RS485.write(0xff);           // Start byte
  RS485.write(packet, length); // Packet payload
  byte checksum = calculateChecksum(packet, length);
  // Handle special checksum cases
  if (checksum == 0xff)
  {
    RS485.write(0xfe);
    RS485.write(0x01);
  }
  else if (checksum == 0xfe)
  {
    RS485.write(0xfe);
    RS485.write(0x00);
  }
  else
  {
    RS485.write(checksum);
  }
  RS485.write(0xff); // Stop byte
}
void firstDotOne()
{
  RS485.write(0xff); // Start byte
  RS485.write(0x0b); // Sign address
  RS485.write(0x00); // Command
  RS485.write(0x01); // First dot on
  RS485.write(0xff); // Stop byte
}
void secondDotOne()
{
  RS485.write(0xff); // Start byte
  RS485.write(0x0b); // Sign address
  RS485.write(0x00); // Command
  RS485.write(0x02); // Second dot on
  RS485.write(0xff); // Stop byte
}
void loop()
{
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  secondDotOne();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  firstDotOne();
  delay(1000);
}