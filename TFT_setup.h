// This is a configuration setup file example for an RP2040 processor
// Adapt as required for your own hardware configuration

#define ILI9488_DRIVER

//#define TFT_WIDTH 320
//#define TFT_HEIGHT 240

#define TFT_SPI_PORT 1
#define TFT_MISO  12 // DO NOT CONNECT TO THE TFT!!!, ONLY CONNECT TO TOUCH MISO
#define TFT_SCLK  10 //PICO pin 14 
#define TFT_MOSI  11 //hw pin 15
#define TFT_DC   4  // Data Command control pin, hw pin 6
#define TFT_RST  5  // Reset pin (could connect to Arduino RESET pin). hw pin 7
#define TFT_CS   13  // Chip select control pin, hw pin 17
#define TOUCH_CS 9 // hw 12

// MOSI also goes to T_DIN, MISO goes to T_DOUT

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

//#define SMOOTH_FONT
//#define LOAD_GFXFF

#define SPI_FREQUENCY  66000000

#define SPI_READ_FREQUENCY  20000000

#define SPI_TOUCH_FREQUENCY  2500000
