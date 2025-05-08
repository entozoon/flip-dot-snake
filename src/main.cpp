#include <Arduino.h>
#include <cstring>
const int DISPLAY_WIDTH = 28;
const int DISPLAY_HEIGHT = 13;
const byte DEFAULT_SIGN_ADDRESS = 0x00;
byte screenBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
byte screenActual[DISPLAY_HEIGHT][DISPLAY_WIDTH];
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
  sendDisplayPacket(fullPacket, totalPacketContentLength);
  memcpy(screenActual, screenBuffer, DISPLAY_HEIGHT * DISPLAY_WIDTH); // Copy screenBuffer to screenActual
}
void clearScreen()
{
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      screenBuffer[y][x] = 0;
    }
  }
  // Send the cleared buffer to the display
  drawFullScreen(screenBuffer, DEFAULT_SIGN_ADDRESS);
}
void fillScreen()
{
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      screenBuffer[y][x] = 1;
    }
  }
  // Send the filled buffer to the display
  drawFullScreen(screenBuffer, DEFAULT_SIGN_ADDRESS);
}
void drawFiveColumnsWorthOfPixels(int x, int y)
{
  // Draw a 5-column wide section of the screenBuffer to the display
  // This is a simplified version for demonstration purposes.
  byte packet[13]; // Packet for a single 1-column, 5-pixel high update
  packet[0] = DEFAULT_SIGN_ADDRESS;
  packet[1] = 0xA2; // Command for display data
  packet[2] = 0xD0; // Display width label for data:
  packet[3] = 28;
  packet[4] = 0xD1; // Height label for data
  packet[5] = 13;
  packet[6] = 0xD2;          // Horizontal offset label on display
  packet[7] = (byte)x;       // Horizontal offset value
  packet[8] = 0xD3;          // Vertical offset label on display
  packet[9] = (byte)(y + 4); // Vertical offset value (protocol specific: y + 4)
  packet[10] = 0xD4;         // Font label
  packet[11] = 0x77;         // Pixel control font
  byte current_col_data_byte = screenBuffer[y][x];
  packet[12] = current_col_data_byte + 32; // Pixel data byte for the column segment (+32 offset)
  sendDisplayPacket(packet, sizeof(packet));
}
// void drawBuffer()
// {
//   // Compare screenBuffer to screenActual, going in 5px high columns.
//   // Where there are differences, draw to the screen those 5px columns
//   // using the vertical and horizontal offsets, then copy screenBuffer to screenActual.
//   const int numBands = (DISPLAY_HEIGHT + 4) / 5; // Each band is 5 pixels high
//   for (int band_idx = 0; band_idx < numBands; ++band_idx)
//   {
//     int band_start_row = band_idx * 5;
//     // Determine the actual number of rows in this band segment for packet height
//     // This can be less than 5 for the last band if DISPLAY_HEIGHT is not a multiple of 5.
//     int actual_rows_in_this_band_segment = 5;
//     if (band_start_row + 5 > DISPLAY_HEIGHT)
//     {
//       actual_rows_in_this_band_segment = DISPLAY_HEIGHT - band_start_row;
//     }
//     for (int col_x = 0; col_x < DISPLAY_WIDTH; ++col_x)
//     {
//       byte current_col_data_byte = 0;
//       byte actual_col_data_byte = 0;
//       // Calculate the 5-bit representation for the current column in this band
//       // for both screenBuffer and screenActual.
//       // Iterate 5 times for the 5 potential bits in a column byte.
//       for (int r = 0; r < 5; ++r)
//       {
//         int current_display_row = band_start_row + r;
//         if (current_display_row < DISPLAY_HEIGHT) // Only consider pixels within display bounds
//         {
//           if (screenBuffer[current_display_row][col_x] == 1)
//           {
//             current_col_data_byte |= (1 << r);
//           }
//           if (screenActual[current_display_row][col_x] == 1)
//           {
//             actual_col_data_byte |= (1 << r);
//           }
//         }
//       }
//       // If the column segment data differs, send an update packet
//       if (current_col_data_byte != actual_col_data_byte)
//       {
//         byte packet[13]; // Packet for a single 1-column, 5-pixel high update
//         packet[0] = DEFAULT_SIGN_ADDRESS;
//         packet[1] = 0xA2; // Command for display data
//         // Define the dimensions of the data being sent (1 column wide)
//         packet[2] = 0xD0;                                   // Width label for data
//         packet[3] = 0x01;                                   // Width of data = 1 column
//         packet[4] = 0xD1;                                   // Height label for data
//         packet[5] = (byte)actual_rows_in_this_band_segment; // Height of data for this specific segment
//         // Define the position on the display where this data goes
//         packet[6] = 0xD2;                        // Horizontal offset label on display
//         packet[7] = (byte)col_x;                 // Horizontal offset value
//         packet[8] = 0xD3;                        // Vertical offset label on display
//         packet[9] = (byte)(band_start_row + 4);  // Vertical offset value (protocol specific: band_start_row + 4)
//         packet[10] = 0xD4;                       // Font label
//         packet[11] = 0x77;                       // Pixel control font
//         packet[12] = current_col_data_byte + 32; // Pixel data byte for the column segment (+32 offset)
//         sendDisplayPacket(packet, 13);
//         // A small delay might be needed if updates are very frequent,
//         // but relying on main loop delay for now as per drawFullScreen comments.
//         delay(10); // Adding delay to allow display to process
//       }
//     }
//   }
//   // After all differing columns have been sent, copy screenBuffer to screenActual
//   for (int y = 0; y < DISPLAY_HEIGHT; ++y)
//   {
//     for (int x = 0; x < DISPLAY_WIDTH; ++x)
//     {
//       screenActual[y][x] = screenBuffer[y][x];
//     }
//   }
// }
void setup()
{
  Serial.begin(115200);                  // For debugging
  Serial1.begin(4800, SERIAL_8N1, 6, 7); // Pins D6 (TX1), D7 (RX1) for ESP32-C6 SuperMini
  while (!Serial1)
  {
    ; // wait for serial port to connect.
  }
  // Initialize the screen bufferz
  for (int y = 0; y < DISPLAY_HEIGHT; ++y)
  {
    for (int x = 0; x < DISPLAY_WIDTH; ++x)
    {
      screenBuffer[y][x] = 1;
      screenActual[y][x] = 0;
    }
  }
  delay(4000);
  Serial.println("Let's go");
  Serial.println("FILL");
  fillScreen();
  delay(4000);
}
void loop()
{
  Serial.println("CLEAR");
  clearScreen();
  delay(4000);
  // screenBuffer[0][0] = 1;
  // screenBuffer[1][1] = 1;
  // screenBuffer[2][2] = 1;
  // screenBuffer[3][3] = 1;
  // screenBuffer[4][4] = 1;
  // screenBuffer[5][5] = 1;
  // screenBuffer[6][6] = 1;
  // screenBuffer[7][7] = 1;
  // screenBuffer[8][8] = 1;
  // screenBuffer[9][9] = 1;
  // screenBuffer[10][10] = 1;
  Serial.println("STUFF 1");
  drawFiveColumnsWorthOfPixels(0, 0);
  delay(4000);
  Serial.println("STUFF 2");
  drawFiveColumnsWorthOfPixels(1, 0);
  delay(4000);
}