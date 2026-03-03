#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>
#include "config.h"

// ============================================================
//  HARDWARE CONFIGURATION (LovyanGFX)
//  GC9A01 240x240 round display — ESP32-2424S012
// ============================================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI      _bus;

public:
  LGFX() {
    {
      auto cfg       = _bus.config();
      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 80000000;
      cfg.freq_read  = 20000000;
      cfg.spi_3wire  = true;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk   = SPI_SCLK;
      cfg.pin_mosi   = SPI_MOSI;
      cfg.pin_miso   = -1;
      cfg.pin_dc     = SPI_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg             = _panel.config();
      cfg.pin_cs           = SPI_CS;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.memory_width     = SCREEN_WIDTH;
      cfg.memory_height    = SCREEN_HEIGHT;
      cfg.panel_width      = SCREEN_WIDTH;
      cfg.panel_height     = SCREEN_HEIGHT;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

#endif
