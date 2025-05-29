#include "Ui.h"
#include "SpiAPI.h"

SpiAPI spi_api;
#include <string>
std::string PluginsJSON;

void Ui::Init(){
    // SPI API init
    spi_api.Init();

    // UI STM32 communication
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);
    Wire1.setClock(400000);
    //Wire1.onFinishedAsync(i2c_async_done);
    Wire1.begin();

    // display init
    // TODO: Adafruit_SH1106G.cpp in Adafruit library, change l. 139, 140 _page_start_offset = 0 to avoid display line offset!
    softSPI = new SoftwareSPI(OLED_SCLK, OLED_DC, OLED_MOSI);
    display = new Adafruit_SH1106G(128, 64, softSPI, OLED_DC, OLED_RST, OLED_CS);
    display->begin(0, true);
    display->setRotation(2);
    display->clearDisplay();
    display->display();

    // NeoPixel init
    strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show(); // Turn OFF all pixels ASAP
    strip.setBrightness(10);
}

void Ui::WSSync(){
    ws_blink = 1; // set blink flag
}

void Ui::Update(){
    std::string response;
    spi_api.LoadPreset(0, 0); // load preset for channel 0, preset 0


    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SH110X_WHITE);
    display->setCursor(0, 0);
    display->printf("%s\n", response.c_str());

    display->display();

    delay(1000);
}

/*
void Ui::Update(){
  static unsigned long tick = 0;
  unsigned long delta = millis() - tick;
  tick = millis();
  static uint32_t bpm = 0;
  // get data from stm
  Wire1.readAsync(I2C_SLAVE_ADDR, &ui_data, sizeof(ui_data_t), true);
  while(!Wire1.finishedAsync()) delay(1);
  char buf[64];

  // ws indicator
  if(ws_blink){
    strip.setPixelColor(rgb_led_rp2350, strip.Color(255, 255, 255));
    ws_blink = 0;
  }else{
    strip.setPixelColor(rgb_led_rp2350, strip.Color(0, 0, 0));
  }

  // print pots
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->setCursor(0, 0);
  display->printf("%04d %04d %04d %04d\n", ui_data.pot_positions[0], ui_data.pot_positions[1], ui_data.pot_positions[2], ui_data.pot_positions[3]);

  for(int i=0, j=0;i<4;i++){
    if(ui_data.pot_states[i] & (1 << 0)) buf[j++] = '1'; else buf[j++] = '0';
    if(ui_data.pot_states[i] & (1 << 1)) buf[j++] = '1'; else buf[j++] = '0';
  }
  buf[8] = 0;
  display->printf("%s\n", buf);

  // print dbuttons
  for(int i=0;i<16;i++){
    if(ui_data.d_btns & (1 << i)){
      buf[i] = '1';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(0, 255, 0));
    }else{
      buf[i] = '0';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(64, 64, 64));
    }
    if(ui_data.d_btns_long_press & (1 << i)){
      buf[i] = 'L';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(255, 0, 0));
    }
  }
  buf[16] = 0;
  display->printf("%s\n", buf);

  // print fbuttons
  if (ui_data.f_btns & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(255, 0, 0));

  if (ui_data.f_btns & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(255, 0, 0));

  if (ui_data.f_btns & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(64, 64, 64));
  if (ui_data.f_btns_long_press & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(255, 0, 0));


  for(int i=0;i<5;i++){
    if(ui_data.f_btns & (1 << i)){
      buf[i] = '1';
    }
    else{
      buf[i] = '0';
    }
    if(ui_data.f_btns_long_press & (1 << i)){
      buf[i] = 'L';
    }
  }
  buf[5] = 0;
  display->printf("%s\n", buf);

  // print mcl buttons
  if (ui_data.mcl_btns & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_mcl, strip.Color(64, 64, 64));
  if (ui_data.mcl_btns_long_press & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 0, 0));

  for(int i=0;i<13;i++){
    if(ui_data.mcl_btns & (1 << i)){
      buf[i] = '1';
    }
    else{
      buf[i] = '0';
    }
    if(ui_data.mcl_btns_long_press & (1 << i)){
      buf[i] = 'L';
    }
  }
  buf[13] = 0;
  display->printf("%s\n", buf);


  display->printf("FPS %dHz, MSPF %dms\n", 1000 / delta, delta);

  // 120 bpm indicator approx.
  if(bpm > 71){
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 255, 255));
    bpm = 0;
  }
  bpm++;

  display->display();
  strip.show();
}
*/
