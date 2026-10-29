#include "AudioBoard.h"  //https://github.com/pschatzmann/arduino-audio-driver
#include "Audio.h"       //https://github.com/schreibfaul1/ESP32-audioI2S
#include "SD_MMC.h"

Audio audio;
DriverPins my_pins;
AudioBoard board(AudioDriverES8311, my_pins);  

//SD_MMC 
#define SD_MMC_D0    39
#define SD_MMC_D1    40
#define SD_MMC_D2    41
#define SD_MMC_D3    42
#define SD_MMC_CMD   44
#define SD_MMC_CLK   43

//ES8311 I2S
#define I2S_DSDIN 11
#define I2S_BCLK 12 //I2S_SCLK
#define I2S_LRC 10
#define I2S_DOUT 9
#define I2S_MCLK 13

//NS4150
#define PA_CTRL 53 //音频输出状态

// ES8311 I2C 
#define I2C_SDA 7  
#define I2C_SCL 8 
#define ES8311_ADDRESS 0x18

void setup() {
  Serial.begin(115200);

  //NS4150输出状态
  pinMode(PA_CTRL, OUTPUT);
  digitalWrite(PA_CTRL, HIGH);
 
  // add i2c codec pins: scl, sda, port
  my_pins.addI2C(PinFunction::CODEC, I2C_SCL, I2C_SDA, ES8311_ADDRESS);

  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0,SD_MMC_D1,SD_MMC_D2,SD_MMC_D3); //四线SD_MMC
  if (!SD_MMC.begin("/root")) {    
    Serial.println("Card Mount Failed");
    return;
  }
  
  // configure codec 
  CodecConfig cfg;
  cfg.input_device = ADC_INPUT_ALL;//ADC_INPUT_LINE1; ADC_INPUT_ALL
  cfg.output_device = DAC_OUTPUT_ALL; 
  cfg.i2s.bits = BIT_LENGTH_16BITS;
  cfg.i2s.rate = RATE_44K;
  cfg.i2s.fmt = I2S_NORMAL;  
    
  //初始化ES8311
  board.begin(cfg);

  //调用audio库实现MP3输出
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT,I2S_MCLK);
  audio.setVolume(10); // 0...21
  audio.connecttoFS(SD_MMC, "01.mp3");
  
}

void loop() {
audio.loop();
}
