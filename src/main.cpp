#include <Arduino.h>
#include <cstring> // For memcpy

// Display dimensions
const int DISPLAY_WIDTH = 28;
const int DISPLAY_HEIGHT = 13; // 13 pixels high

// Sign address
const byte DEFAULT_SIGN_ADDRESS = 0x00;

// Function to calculate Modulo 256 Checksum
byte calculateChecksum(byte byteArray[], int length)
{
  unsigned int sum = 0;
  for (int i = 0; i < length; i++)
  {
    sum += byteArray[i];
  }
  return (byte)(sum % 256); // Return checksum as byte (mod 256)
}

// Function to send a complete packet (including start/stop bytes and checksum)
void sendDisplayPacket(byte packetData[], int packetLength)
{
  Serial1.write(0xff); // Starting byte
  Serial1.write(packetData, packetLength);
  byte checksum = calculateChecksum(packetData, packetLength);

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
  Serial1.flush();     // Wait for transmission to complete
}

// Function to draw a full screen buffer to the flip-dot display
void drawFullScreen(byte screenBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH], byte signAddress)
{
  const int numBands = (DISPLAY_HEIGHT + 4) / 5; // Each band is 5 pixels high

  // Calculate total packet size
  // Header: signAddr, A2, D0, W, D1, H (6 bytes)
  // Per Band Data Section: D2, HOffset, D3, VOffset, D4, Font (6 bytes) + PixelData (DISPLAY_WIDTH bytes)
  const int commonHeaderSize = 6;
  const int bandControlSizeBytes = 6;
  const int bandPixelDataSizeBytes = DISPLAY_WIDTH;
  const int singleBandDataSectionSize = bandControlSizeBytes + bandPixelDataSizeBytes;
  const int totalPacketContentLength = commonHeaderSize + (numBands * singleBandDataSectionSize);

  byte fullPacket[totalPacketContentLength];
  int currentPacketIndex = 0;

  // 1. Common Header
  fullPacket[currentPacketIndex++] = signAddress;
  fullPacket[currentPacketIndex++] = 0xA2;
  fullPacket[currentPacketIndex++] = 0xD0;           // Display width label
  fullPacket[currentPacketIndex++] = DISPLAY_WIDTH;  // Actual width value (0x1C for 28)
  fullPacket[currentPacketIndex++] = 0xD1;           // Display height label
  fullPacket[currentPacketIndex++] = DISPLAY_HEIGHT; // Actual height value (0x0D for 13)

  // 2. Data Sections (one for each band)
  for (int band = 0; band < numBands; ++band)
  {
    int bandStartRow = band * 5;
    byte verticalOffset = bandStartRow + 4; // weird offset

    // 2a. Band Control Bytes
    fullPacket[currentPacketIndex++] = 0xD2; // Horizontal offset label
    fullPacket[currentPacketIndex++] = 0x00; // Horizontal offset value (0)
    fullPacket[currentPacketIndex++] = 0xD3; // Vertical offset label
    fullPacket[currentPacketIndex++] = verticalOffset;
    fullPacket[currentPacketIndex++] = 0xD4; // Font label
    fullPacket[currentPacketIndex++] = 0x77; // Pixel control font

    // 2b. Band Pixel Data
    byte subcolumnData[DISPLAY_WIDTH];
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      byte subcol_value = 0;
      for (int r = 0; r < 5; ++r) // 5 pixels in a subcolumn
      {
        int currentDisplayRow = bandStartRow + r;
        if (currentDisplayRow < DISPLAY_HEIGHT)
        {
          if (screenBuffer[currentDisplayRow][x] == 1)
          {
            subcol_value |= (1 << r); // Bit 0 for top-most pixel in band, up to bit 4
          }
        }
      }
      subcolumnData[x] = subcol_value + 32; // Add 32 as per protocol
    }
    memcpy(&fullPacket[currentPacketIndex], subcolumnData, DISPLAY_WIDTH);
    currentPacketIndex += DISPLAY_WIDTH;
  }

  // 3. Send the single, complete packet
  sendDisplayPacket(fullPacket, totalPacketContentLength);
  // A small delay after a full screen update might be good if updates are very frequent,
  // but the main loop already has delays.
}

// Global screen buffer
byte mainScreenBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

void setup()
{
  Serial.begin(115200);                  // For debugging
  Serial1.begin(4800, SERIAL_8N1, 6, 7); // Pins D6 (TX1), D7 (RX1) for ESP32-C6 SuperMini

  while (!Serial1)
  {
    ; // wait for serial port to connect.
  }
  Serial.println("Serial1 connected. Waiting 2s for display to be ready...");
  delay(2000); // Delay for robust communication
  Serial.println("Setup complete. Starting loop.");

  // Initialize screen buffer to all off
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      mainScreenBuffer[y][x] = 0;
    }
  }
  drawFullScreen(mainScreenBuffer, DEFAULT_SIGN_ADDRESS); // Clear screen initially
}

void loop()
{
  // Example 1: Fill screen with ON
  Serial.println("Drawing full screen ON");
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      mainScreenBuffer[y][x] = 1;
    }
  }
  drawFullScreen(mainScreenBuffer, DEFAULT_SIGN_ADDRESS);
  delay(2000); // Display for 2 seconds

  // Example 2: Fill screen with OFF
  Serial.println("Drawing full screen OFF");
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      mainScreenBuffer[y][x] = 0;
    }
  }
  drawFullScreen(mainScreenBuffer, DEFAULT_SIGN_ADDRESS);
  delay(2000); // Display for 2 seconds

  // Example 3: Draw a border
  Serial.println("Drawing a border");
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      if (y == 0 || y == DISPLAY_HEIGHT - 1 || x == 0 || x == DISPLAY_WIDTH - 1)
      {
        mainScreenBuffer[y][x] = 1;
      }
      else
      {
        mainScreenBuffer[y][x] = 0;
      }
    }
  }
  drawFullScreen(mainScreenBuffer, DEFAULT_SIGN_ADDRESS);
  delay(2000);

  // Example 4: Checkerboard pattern
  Serial.println("Drawing checkerboard");
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      mainScreenBuffer[y][x] = (x % 2 == y % 2) ? 1 : 0;
    }
  }
  drawFullScreen(mainScreenBuffer, DEFAULT_SIGN_ADDRESS);
  delay(2000);
}
