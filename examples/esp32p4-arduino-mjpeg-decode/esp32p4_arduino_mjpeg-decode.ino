#define ROOT "/root"
#define MJPEG_FILENAME ROOT "/mjpeg_720_720_30fps.mjpeg"
#define MJPEG_OUTPUT_SIZE (720 * 720 * 2)          // memory for a output image frame
#define MJPEG_BUFFER_SIZE (MJPEG_OUTPUT_SIZE / 10) // memory for a single JPEG frame

#include <SD_MMC.h>
#include "pins_config.h"
#include "src/lcd/st7703_lcd.h"
st7703_lcd lcd = st7703_lcd(LCD_RST);

#include "MjpegClass.h"
static MjpegClass mjpeg;

/* variables */
static int total_frames = 0;
static unsigned long total_read_video = 0;
static unsigned long total_decode_video = 0;
static unsigned long total_show_video = 0;
static unsigned long start_ms, curr_ms;
static int16_t x = -1, y = -1, w = -1, h = -1;

void setup()
{
  Serial.begin(115200);
  lcd.begin();

  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0,SDMMC_D1,SDMMC_D2,SDMMC_D3);
  if (!SD_MMC.begin(ROOT, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    Serial.println(F("MJPEG start"));

    start_ms = millis();
    curr_ms = millis();
    if (!mjpeg.setup(MJPEG_FILENAME))
    {
      Serial.println(F("mjpeg.setup() failed!"));
    }
    else
    {
      while (mjpeg.readMjpegBuf())
      {
        // Read video
        total_read_video += millis() - curr_ms;
        curr_ms = millis();

        // Play video
        mjpeg.decodeJpg();
        total_decode_video += millis() - curr_ms;
        curr_ms = millis();

        if (x == -1)        
        {
          w = mjpeg.getWidth();
          h = mjpeg.getHeight();
          x = (w > lcd.width()) ? 0 : ((lcd.width() - w) / 2);
          y = (h > lcd.height()) ? 0 : ((lcd.height() - h) / 2);
        }
        lcd.draw16bitbergbbitmap(x, y, w, h, mjpeg.getOutBuf());
        total_show_video += millis() - curr_ms;

        curr_ms = millis();
        total_frames++;
      }
      int time_used = millis() - start_ms;
      Serial.println(F("MJPEG end"));

      float fps = 1000.0 * total_frames / time_used;
      Serial.printf("ESP32P4 MJPEG decoder\n\n");
      Serial.printf("Frame size: %d x %d\n", mjpeg.getWidth(), mjpeg.getHeight());
      Serial.printf("Total frames: %d\n", total_frames);
      Serial.printf("Time used: %d ms\n", time_used);
      Serial.printf("Average FPS: %0.1f\n", fps);
      Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", total_read_video, 100.0 * total_read_video / time_used);
      Serial.printf("Decode video: %lu ms (%0.1f %%)\n", total_decode_video, 100.0 * total_decode_video / time_used);
      Serial.printf("Show video: %lu ms (%0.1f %%)\n", total_show_video, 100.0 * total_show_video / time_used);
      
      mjpeg.close();
    }
  }
}

void loop()
{
}
