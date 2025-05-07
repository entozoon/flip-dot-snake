#include <Arduino.h>
// https://github.com/Nosen92/maskin-flipdot/blob/main/protocol.md
byte packet[] = {
    // 0xff,  // Starting byte, sent separately below to not include in checksum
    0x00, // Sign address
    0xa2, // Always a2
    0xd0, // Display width:
    0x1c, // 28px
    0xd1, // Display height:
    0xd0, // 13px
    0xd2, // Horizontal offset:
    0x00, // Xpx right
    0xd3, // Vertical offset:
    0x03, // Ypx down (3 is top for pixel font)
    0xD4, // Font:
    0x77, // 0x60 = 7px text, 0x62 = 7px wide, etc etc, 0x77 = pixel control
          // 0x48, // H
          // 0x45, // E
          // 0x4c, // L
          // 0x4c, // L
          // 0x4f, // O
    0x2a,
    0x20,
    0x31,
    0x2e,
    //
    0x2a,
    0x20,
    0x31,
    0x2e,
    // 0xcd,  // Checksum
    // 0xff   // Stop byte
};
int length = sizeof(packet) / sizeof(packet[0]);
byte calculateChecksum(byte byteArray[], int length)
{
  unsigned int sum = 0;
  for (int i = 0; i < length; i++)
  {
    sum += byteArray[i];
  }
  return (byte)(sum % 256); // Return checksum as byte (mod 256)
}
void sendDisplayPacket(byte packet[], int packetLength)
{
  Serial1.write(0xff); // Starting byte
  Serial1.write(packet, packetLength);
  byte checksum = calculateChecksum(packet, packetLength);
  if (checksum == 0xff)
  {
    Serial1.write(0xfe);
    Serial1.write(0x01);
  }
  else if (checksum == 0xfe)
  {
    Serial1.write(0xfe);
    Serial1.write(0x00);
  }
  else
  {
    Serial1.write(checksum);
  }
  Serial1.write(0xff); // Stop byte
  Serial1.flush();
}
void setup()
{
  Serial.begin(115200); // For debugging
  Serial1.begin(4800, SERIAL_8N1, 6, 7);
  // Using default UART1 pins for esp32-c6-supermini for now.
  while (!Serial1)
  {
    ; // wait for serial port to connect.
  }
  delay(2000); // Makes communication much more robust
  sendDisplayPacket(packet, length);
}
bool sendpacketState = false; // packet is sent in setup, so loop starts with packet2
void loop()
{
  // for vertical offset 0x00 through 0x0a
  for (int i = 0; i < 14; i++)
  {
    packet[9] = i;
    sendDisplayPacket(packet, length);
    delay(1000);
    Serial.println("vertical offset: " + String(i));
  }
}