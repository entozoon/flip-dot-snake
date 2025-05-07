#include <Arduino.h>
// Use UART1 with TX on GPIO10, RX unused (-1)
HardwareSerial RS485(1); // UART1
const uint8_t RS485_TX_PIN = 7;
const uint32_t BAUD_RATE = 4800;
// Mobitec flip-dot RS-485 header
const uint8_t sign_hdr[] = {0xFF, 0x06, 0xA2};
void add_checksum(uint8_t *data, int *length)
{
  int csum = 0;
  for (int i = 1; i < *length; i++)
  {
    csum += data[i];
  }
  data[(*length)++] = csum & 0xFF;
  if (data[*length - 1] == 0xFE)
  {
    data[(*length)++] = 0x00;
  }
  else if (data[*length - 1] == 0xFF)
  {
    data[*length - 1] = 0xFE;
    data[(*length)++] = 0x01;
  }
}
void write_raw(uint8_t *msg, int len)
{
  RS485.write(msg, len);
  RS485.flush();
}
// Sends command to set just the first pixel (1,1) on or off
void set_first_dot(bool on)
{
  uint8_t msg[64];
  int len = 0;
  memcpy(msg, sign_hdr, sizeof(sign_hdr));
  len += sizeof(sign_hdr);
  // Set X = 1
  msg[len++] = 0xD2;
  msg[len++] = 1;
  // Set Y = 1
  msg[len++] = 0xD3;
  msg[len++] = 1;
  // Select bitmap font
  msg[len++] = 0xD4;
  msg[len++] = 0x77;
  // Set dot (bitmap font row of 0x01 = 1 pixel lit, 0x00 = off)
  msg[len++] = on ? 0x01 : 0x00;
  add_checksum(msg, &len);
  msg[len++] = 0xFF; // End byte
  write_raw(msg, len);
}
static void write_all_white(void)
{
  uint8_t buffer[1024];
  int len;
  len = 0;
  buffer[len++] = 0xd2; // Set x coordinate to 1
  buffer[len++] = 1;
  buffer[len++] = 0xd3; // Set y coordinate to 4
  buffer[len++] = 4;
  buffer[len++] = 0xd4; // Select bitmap font
  buffer[len++] = 0x77;
  // 0x20 - 0x3f is used with the bitmap font
  // 0x20 is no pixels set and 0x3f is all pixels set
  memset(buffer + len, 0x3f, 112);
  len += 112;
  buffer[len++] = 0xd2;
  buffer[len++] = 1;
  buffer[len++] = 0xd3;
  buffer[len++] = 9;
  buffer[len++] = 0xd4;
  buffer[len++] = 0x77;
  memset(buffer + len, 0x3f, 112);
  len += 112;
  buffer[len++] = 0xd2;
  buffer[len++] = 1;
  buffer[len++] = 0xd3;
  buffer[len++] = 14;
  buffer[len++] = 0xd4;
  buffer[len++] = 0x77;
  memset(buffer + len, 0x3f, 112);
  len += 112;
  buffer[len++] = 0xd2;
  buffer[len++] = 1;
  buffer[len++] = 0xd3;
  buffer[len++] = 19;
  buffer[len++] = 0xd4;
  buffer[len++] = 0x77;
  memset(buffer + len, 0x3f, 112);
  len += 112;
  buffer[len] = '\0';
  // write_text(buffer);
  write_raw(buffer, len);
}
void setup()
{
  RS485.begin(BAUD_RATE, SERIAL_8N1, -1, RS485_TX_PIN); // RX = -1
  delay(1000);                                          // Optional: allow device boot time
  set_first_dot(true);                                  // Turn it on at startup
}
void loop()
{
  delay(4000);
  set_first_dot(false);
  delay(4000);
  set_first_dot(true);
  delay(4000);
  write_all_white();
  delay(4000);
}