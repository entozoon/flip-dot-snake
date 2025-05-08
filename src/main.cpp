#include <Arduino.h>
// https://github.com/Nosen92/maskin-flipdot/blob/main/protocol.md
byte packet1[] = {
    // 0xff,  // Start byte - Sent separately as not included in checksum
    0x00, // Sign address
    0xa2, // Always a2
    0xd0, // Display width:
    28,   // 28px (but doesn't seem to matter)
    0xd1, // Display height:
    13,   // 13px
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
    // A column of 5 pixels is represented by a single byte
    // 1st - 1, 2nd - 2, 3rd - 4, 4th - 8, 5th - 16.
    // Add these up binary style then plus 32 to get the character code
    // all 5 pixels on =  0b11111 + 32
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    32 + 0b00000,
    // 0xcd,   // Checksum - sent separately as not included in checksum
    // 0xff    // Stop byte - Sent separately as not included in checksum
};
byte packet2[] = {
    // 0xff,  // Start byte - Sent separately as not included in checksum
    0x00, // Sign address
    0xa2, // Always a2
    0xd0, // Display width:
    28,   // 28px (but doesn't seem to matter)
    0xd1, // Display height:
    13,   // 13px
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
    // A column of 5 pixels is represented by a single byte
    // 1st - 1, 2nd - 2, 3rd - 4, 4th - 8, 5th - 16.
    // Add these up binary style then plus 32 to get the character code
    // all 5 pixels on =  0b11111 + 32
    32 + 0b10000,
    32 + 0b01000,
    32 + 0b00100,
    32 + 0b00010,
    32 + 0b00001,
    32 + 0b00010,
    32 + 0b00100,
    32 + 0b01000,
    32 + 0b10000,
    // 0xcd,   // Checksum - sent separately as not included in checksum
    // 0xff    // Stop byte - Sent separately as not included in checksum
};
// function to get length above

byte calculateChecksum(byte byteArray[], int length)
{
  unsigned int sum = 0;
  for (int i = 0; i < length; i++)
  {
    sum += byteArray[i];
  }
  return (byte)(sum % 256); // Return checksum as byte (mod 256)
}
void sendDisplayPacket(byte packet[])
{
  int packetLength = sizeof(packet) / sizeof(packet[0]);
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
  while (!Serial1)
  {
    ; // wait for serial port to connect.
  }
  delay(4000); // Makes communication much more robust
}
void loop()
{
  Serial.println("Packet 1");
  sendDisplayPacket(packet1);
  delay(4000);
  Serial.println("Packet 2");
  sendDisplayPacket(packet2);
  delay(4000);
}