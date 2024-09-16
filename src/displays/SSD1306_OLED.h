#ifndef SSD1306_OLED_H
#define SSD1306_OLED_H

#include "displayDriver.h"
#include "panel.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef DISPLAY_SSD1306_OLED
    #define SCREEN_WIDTH 128  // OLED display width, in pixels
    #define SCREEN_HEIGHT 32  // OLED display height, in pixels
    #define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)
    #define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#endif

class SSD1306_OLED : public DisplayDriver {
private:
    Adafruit_SSD1306 display;
    uint8_t activeBuffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];  // Active framebuffer
    uint8_t workingBuffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8]; // Working framebuffer
    unsigned long lastUpdateTime = 0;  // Time tracker for refresh
    const unsigned long refreshInterval = 33;  // 30 Hz = ~33ms per frame

    void SwapBuffers();  // Swap active and working buffers
    void CopyFrameBuffer();  // Copy activeBuffer to workingBuffer

public:
    SSD1306_OLED();
    virtual void DrawPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) override;
    virtual void DrawPixel(uint16_t x, uint16_t y, uint16_t color) override;
    virtual void ClearScreen() override;
    virtual void SetBrightness(uint8_t level) override;
    virtual void FillScreen(uint8_t r, uint8_t g, uint8_t b) override;
    virtual void DisplayText(const char *text, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, bool transparent = false, bool inverted = false) override;
    virtual void FillZoneRaw(uint8_t idx, uint8_t *pBuffer) override;
    virtual void FillZoneRaw565(uint8_t idx, uint8_t *pBuffer) override;
    virtual void FillPanelRaw(uint8_t *pBuffer) override;
    virtual void FillPanelUsingPalette(uint8_t *pBuffer, uint8_t *palette) override;
#if !defined(ZEDMD_WIFI)
    virtual void FillPanelUsingChangedPalette(uint8_t *pBuffer, uint8_t *palette, bool *paletteAffected) override;
#endif

    bool ConvertToBlackAndWhite(uint8_t r, uint8_t g, uint8_t b);
    bool ConvertToBlackAndWhite(uint16_t color565);
    uint8_t RgbToGrayscale(uint8_t r, uint8_t g, uint8_t b);
    void DrawDitheredPixel(int x, int y, uint8_t *pBuffer);  // Dithered drawing function
    void UpdateDisplay();  // Refresh the display at 30 Hz
    ~SSD1306_OLED();
};

#endif // SSD1306_OLED_H
