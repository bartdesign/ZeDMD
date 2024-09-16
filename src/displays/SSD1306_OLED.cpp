#ifdef DISPLAY_SSD1306_OLED
#include "SSD1306_OLED.h"
#include "displayConfig.h"
#include "fonts/tiny4x6.h"
#include <Arduino.h>

template <typename T>
T clamp(T value, T min, T max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/// @brief Initialization stuff
SSD1306_OLED::SSD1306_OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)  {

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        // Handle initialization error
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);  // Infinite loop to halt further execution
    }

    // Initialize buffers
    memset(activeBuffer, 0, sizeof(activeBuffer));
    memset(workingBuffer, 0, sizeof(workingBuffer));
    display.clearDisplay();
    display.display();
}

/// @brief Swap active and working buffers and update the display
void SSD1306_OLED::SwapBuffers() {
    memcpy(activeBuffer, workingBuffer, sizeof(workingBuffer));  // Copy working buffer to active
    display.clearDisplay();
    display.drawBitmap(0, 0, activeBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
}

/// @brief Copy active buffer to working buffer
void SSD1306_OLED::CopyFrameBuffer() {
    memcpy(workingBuffer, activeBuffer, sizeof(activeBuffer));  // Copy active buffer to working buffer
}

/// @brief Refresh the display at 30 Hz
void SSD1306_OLED::UpdateDisplay() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= refreshInterval) {
        SwapBuffers();  // Swap buffers and update the display
        CopyFrameBuffer();  // Prepare the working buffer for the next frame
        lastUpdateTime = currentTime;  // Update the timestamp
    }
}

/// @brief Convert RGB888 to black and white
bool SSD1306_OLED::ConvertToBlackAndWhite(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t luminance = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    return luminance >= 128;
}

/// @brief Convert RGB565 to black and white
bool SSD1306_OLED::ConvertToBlackAndWhite(uint16_t color565) {
    uint8_t r = ((color565 >> 11) & 0x1F) << 3;
    uint8_t g = ((color565 >> 5) & 0x3F) << 2;
    uint8_t b = (color565 & 0x1F) << 3;
    return ConvertToBlackAndWhite(r, g, b);
}

/// @brief Convert RGB to grayscale
uint8_t SSD1306_OLED::RgbToGrayscale(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
}

/// @brief Draw a pixel in the working buffer using RGB888
void SSD1306_OLED::DrawPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    bool color = ConvertToBlackAndWhite(r, g, b);
    if (color) {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y % 8));  // Set pixel to white
    } else {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y % 8));  // Set pixel to black
    }
}

/// @brief Draw a pixel in the working buffer using RGB565
void SSD1306_OLED::DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    bool bw = ConvertToBlackAndWhite(color);
    if (bw) {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y % 8));  // Set pixel to white
    } else {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y % 8));  // Set pixel to black
    }
}

/// @brief Function to apply dithering and draw a monochrome pixel
void SSD1306_OLED::DrawDitheredPixel(int x, int y, uint8_t *pBuffer) {
    size_t pos = 3 * (y * SCREEN_WIDTH + x);

    // Get grayscale value
    uint8_t oldPixel = RgbToGrayscale(pBuffer[pos], pBuffer[pos + 1], pBuffer[pos + 2]);

    // Apply threshold to decide the color
    uint8_t newPixel = (oldPixel > 128) ? 255 : 0;
    uint8_t white = (newPixel == 255) ? 1 : 0;

    // Draw the pixel in the working buffer
    if (white) {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] |= (1 << (y % 8));  // Set pixel to white
    } else {
        workingBuffer[x + (y / 8) * SCREEN_WIDTH] &= ~(1 << (y % 8));  // Set pixel to black
    }

    // Calculate the error
    int error = oldPixel - newPixel;

    // Distribute the error to neighboring pixels (Floyd-Steinberg Dithering)
    if (x + 1 < SCREEN_WIDTH) {
        pBuffer[pos + 3] = clamp(pBuffer[pos + 3] + (error * 7) / 16, 0, 255);
        pBuffer[pos + 4] = clamp(pBuffer[pos + 4] + (error * 7) / 16, 0, 255);
        pBuffer[pos + 5] = clamp(pBuffer[pos + 5] + (error * 7) / 16, 0, 255);
    }
    if (y + 1 < SCREEN_HEIGHT) {
        if (x > 0) {
            pBuffer[pos + SCREEN_WIDTH * 3 - 3] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 - 3] + (error * 3) / 16, 0, 255);
            pBuffer[pos + SCREEN_WIDTH * 3 - 2] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 - 2] + (error * 3) / 16, 0, 255);
            pBuffer[pos + SCREEN_WIDTH * 3 - 1] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 - 1] + (error * 3) / 16, 0, 255);
        }

        pBuffer[pos + SCREEN_WIDTH * 3] = clamp(pBuffer[pos + SCREEN_WIDTH * 3] + (error * 5) / 16, 0, 255);
        pBuffer[pos + SCREEN_WIDTH * 3 + 1] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 + 1] + (error * 5) / 16, 0, 255);
        pBuffer[pos + SCREEN_WIDTH * 3 + 2] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 + 2] + (error * 5) / 16, 0, 255);

        if (x + 1 < SCREEN_WIDTH) {
            pBuffer[pos + SCREEN_WIDTH * 3 + 3] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 + 3] + (error * 1) / 16, 0, 255);
            pBuffer[pos + SCREEN_WIDTH * 3 + 4] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 + 4] + (error * 1) / 16, 0, 255);
            pBuffer[pos + SCREEN_WIDTH * 3 + 5] = clamp(pBuffer[pos + SCREEN_WIDTH * 3 + 5] + (error * 1) / 16, 0, 255);
        }
    }
}

/// @brief Clear the working buffer and the display
void SSD1306_OLED::ClearScreen() {
    memset(workingBuffer, 0, sizeof(workingBuffer));  // Clear working buffer
    display.clearDisplay();  // Clear display
    display.display();
}

/// @brief Not supported: Set brightness
void SSD1306_OLED::SetBrightness(uint8_t level) {
    // Not supported for SSD1306
}

/// @brief Fill the screen with a solid color (RGB)
void SSD1306_OLED::FillScreen(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
            DrawPixel(x, y, r, g, b);
        }
    }
}

/// @brief Display text on the screen
void SSD1306_OLED::DisplayText(const char *text, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, bool transparent, bool inverted) {
    for (uint8_t ti = 0; ti < strlen(text); ti++) {
        for (uint8_t tj = 0; tj <= 5; tj++) {
            uint8_t fourPixels = getFontLine(text[ti], tj);  // Assuming getFontLine returns 4 pixels from font
            for (uint8_t pixel = 0; pixel < 4; pixel++) {
                bool p = (fourPixels >> (3 - pixel)) & 0x1;  // Extract the bit for the pixel
                if (inverted) {
                    p = !p;
                }
                if (transparent && !p) {
                    continue;  // Skip if transparent and pixel is off
                }
                DrawPixel(x + pixel + (ti * 4), y + tj, r * p, g * p, b * p);  // Draw each pixel in the character
            }
        }
    }
    display.display();  // Push the updated content to the display
}

/// @brief Fill a zone using RGB888 pixel data
void IRAM_ATTR SSD1306_OLED::FillZoneRaw(uint8_t idx, uint8_t *pBuffer) {
    uint16_t yOffset = (idx / ZONES_PER_ROW) * ZONE_HEIGHT;
    uint16_t xOffset = (idx % ZONES_PER_ROW) * ZONE_WIDTH;

    for (uint16_t y = 0; y < ZONE_HEIGHT; y++) {
        for (uint16_t x = 0; x < ZONE_WIDTH; x++) {
            uint16_t pos = (y * ZONE_WIDTH + x) * 3;
            DrawPixel(x + xOffset, y + yOffset, pBuffer[pos], pBuffer[pos + 1], pBuffer[pos + 2]);
        }
    }
}

/// @brief Fill a zone using RGB565 pixel data
void IRAM_ATTR SSD1306_OLED::FillZoneRaw565(uint8_t idx, uint8_t *pBuffer) {
    uint16_t yOffset = (idx / ZONES_PER_ROW) * ZONE_HEIGHT;
    uint16_t xOffset = (idx % ZONES_PER_ROW) * ZONE_WIDTH;

    for (uint16_t y = 0; y < ZONE_HEIGHT; y++) {
        for (uint16_t x = 0; x < ZONE_WIDTH; x++) {
            uint16_t pos = (y * ZONE_WIDTH + x) * 2;
            uint16_t color565 = pBuffer[pos] | (pBuffer[pos + 1] << 8);
            DrawPixel(x + xOffset, y + yOffset, color565);
        }
    }
}

/// @brief Fill entire screen using raw RGB888 buffer
void IRAM_ATTR SSD1306_OLED::FillPanelRaw(uint8_t *pBuffer) {
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
            uint16_t pos = (y * SCREEN_WIDTH + x) * 3;
            DrawDitheredPixel(x, y, pBuffer);
        }
    }
}

/// @brief Fill entire screen using palette data
void SSD1306_OLED::FillPanelUsingPalette(uint8_t *pBuffer, uint8_t *palette) {
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
            uint16_t pos = pBuffer[y * SCREEN_WIDTH + x] * 3;
            DrawPixel(x, y, palette[pos], palette[pos + 1], palette[pos + 2]);
        }
    }
}

#if !defined(ZEDMD_WIFI)
/// @brief Fill panel using palette with affected tracking
void SSD1306_OLED::FillPanelUsingChangedPalette(uint8_t *pBuffer, uint8_t *palette, bool *paletteAffected) {
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
            uint16_t pos = pBuffer[y * SCREEN_WIDTH + x];
            if (paletteAffected[pos]) {
                pos *= 3;
                DrawPixel(x, y, palette[pos], palette[pos + 1], palette[pos + 2]);
            }
        }
    }
}
#endif

/// Destructor
SSD1306_OLED::~SSD1306_OLED() {
    // Clean up if necessary
}

#endif
