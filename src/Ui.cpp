#include "Ui.h"
#include "SpiAPI.h"
#include "DadaLogo.h"

SpiAPI spi_api;

#define STM32RESET_PIN 40

void Ui::Init(){
    // reset stm
    pinMode(STM32RESET_PIN, OUTPUT);
    digitalWrite(STM32RESET_PIN, false);
    delay(10);
    digitalWrite(STM32RESET_PIN, true);

    // SPI API init
    spi_api.Init();

    // UI STM32 communication
    Wire1.setSDA(I2C_SDA);
    Wire1.setSCL(I2C_SCL);
    Wire1.setClock(400000);
    //Wire1.onFinishedAsync(i2c_async_done);
    Wire1.begin();

    // display init
    softSPI = new SoftwareSPI(OLED_SCLK, OLED_DC, OLED_MOSI);
    display = new DaDa_SSD1309_MCL(128, 64, softSPI, OLED_DC, OLED_RST, OLED_CS); // spec is 10MHz
    display->begin(0, true);
    display->setRotation(0);
    display->clearDisplay();
    display->display();

    // NeoPixel init
    strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show(); // Turn OFF all pixels ASAP
    strip.setBrightness(10);
}

void Ui::displayString(const std::string &s){
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1309_WHITE);
    display->setCursor(0, 0);
    display->printf("%s\n", s.c_str());
    display->display();
}

void Ui::displayStringWait1s(const std::string &s){
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1309_WHITE);
  display->setCursor(0, 0);
  display->printf("%s\n", s.c_str());
  display->display();
  delay(1000);
}

void Ui::Update(){
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastTime < 10) return; // update every 10ms
  lastTime = currentTime;
  if (!p4Ready){
    //displayString("Waiting for P4...");
    display->clearDisplay();
    display->drawBitmap(0, 0, dada_bitmapx, 128, 64, SSD1309_WHITE);
    display->display();
    // assert reset for stm32
    digitalWrite(STM32RESET_PIN, false);
    resetRequested = true;
    delay(100);
    return;
  }
  // de-assert stm reset
  if (resetRequested){
    resetRequested = false;
    digitalWrite(STM32RESET_PIN, true);
    delay(1000);
  }

  //RunSpiAPITests();
  RunUITests();
}

void Ui::RunSpiAPITests(){
  std::string response;

  displayStringWait1s("GetPlugins...");
  spi_api.GetPlugins(response);
  displayStringWait1s(response);

  displayStringWait1s("SetActivePlugin...");
  spi_api.SetActivePlugin(0, "SineSrc");
  spi_api.SetActivePlugin(1, "SineSrc");
  std::string active_plugins;
  displayStringWait1s("GetActivePlugins...");
  spi_api.GetActivePlugin(0, response);
  active_plugins = "CH0: " + response;
  spi_api.GetActivePlugin(1, response);
  active_plugins += " CH1: " + response;
  displayStringWait1s(active_plugins);

  displayStringWait1s("SetActivePluginParam...");
  spi_api.SetActivePluginParam(0, "enableEG", 0);
  spi_api.SetActivePluginParam(1, "enableEG", 0);
  for (int i=440; i<1000; i+=20){
    spi_api.SetActivePluginParam(0, "frequency", i);
    spi_api.SetActivePluginParam(1, "frequency", i*2);
  }
  displayStringWait1s("Save Preset...");
  spi_api.SavePreset(0, "TestPreset0", 1);
  spi_api.SavePreset(1, "TestPreset1", 2);
  displayStringWait1s("Load Preset...");
  spi_api.LoadPreset(0, 0);
  spi_api.LoadPreset(1, 0);

  displayStringWait1s("Get CH0 Plugin Param...");
  spi_api.GetActivePluginParams(0, response);
  displayStringWait1s(response);
  displayStringWait1s("Get CH1 Plugin Param...");
  spi_api.GetActivePluginParams(1, response);
  displayStringWait1s(response);

  displayStringWait1s("Get Presets CH0...");
  spi_api.GetPresets(0, response);
  displayStringWait1s(response);
  displayStringWait1s("Get Presets CH1...");
  spi_api.GetPresets(1, response);
  displayStringWait1s(response);

  displayStringWait1s("Get/Set Preset Data...");
  spi_api.GetPresetData("TBDeep", response);
  displayStringWait1s(response);
  spi_api.SetPresetData("TBDeep", response);
  delay(1000);

  displayStringWait1s("Get/Set Configuration...");
  spi_api.GetConfiguration(response);
  spi_api.SetConfiguration(response);
  delay(1000);

  displayStringWait1s("Get All Favorites...");
  spi_api.GetAllFavorites(response);
  displayString(response);
  displayStringWait1s("Load Favorite 1...");
  spi_api.LoadFavorite(1);
  displayStringWait1s("Save Favorite 0...");
  response = "{\"name\":\"Test123\",\"plug_0\":\"SineSrc\",\"pre_0\":0,\"plug_1\":\"SineSrc\",\"pre_1\":0,\"ustring\":\"Test1234\"}";
  spi_api.SaveFavorite(0, response);
  delay(1000);

  displayStringWait1s("Get IO Caps...");
  spi_api.GetIOCapabilities(response);
  displayStringWait1s(response);

  displayStringWait1s("Reboot P4...");
  spi_api.Reboot();

}

void Ui::updateUIInputs(){
  // get data from stm
  while(!Wire1.finishedAsync());
  Wire1.readAsync(I2C_SLAVE_ADDR, &ui_data, sizeof(ui_data_t), true);
}


void Ui::RunUITests(){
  static unsigned long tick = 0;
  unsigned long delta = millis() - tick;
  tick = millis();
  static uint32_t bpm = 0;

  ui_data_t ui_data_current = ui_data; // copy current ui data
  // start background DMA ui_data update
  updateUIInputs();

  char buf[64];

  // print pots
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1309_WHITE);
  display->setCursor(0, 0);
  display->printf("%04d %04d %04d %04d\n", ui_data_current.pot_positions[0], ui_data_current.pot_positions[1], ui_data_current.pot_positions[2], ui_data_current.pot_positions[3]);

  for(int i=0, j=0;i<4;i++){
    if(ui_data_current.pot_states[i] & (1 << 0)) buf[j++] = '1'; else buf[j++] = '0';
    if(ui_data_current.pot_states[i] & (1 << 1)) buf[j++] = '1'; else buf[j++] = '0';
  }
  buf[8] = 0;
  display->printf("%s\n", buf);

  // print dbuttons
  for(int i=0;i<16;i++){
    if(ui_data_current.d_btns & (1 << i)){
      buf[i] = '1';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(0, 255, 0));
    }else{
      buf[i] = '0';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(64, 64, 64));
    }
    if(ui_data_current.d_btns_long_press & (1 << i)){
      buf[i] = 'L';
      strip.setPixelColor(rgb_led_btn_map[i], strip.Color(255, 0, 0));
    }
  }
  buf[16] = 0;
  display->printf("%s\n", buf);

  // print fbuttons
  /*
  if (ui_data_current.f_btns & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(64, 64, 64));
  if (ui_data_current.f_btns_long_press & (1 << 4))
    strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(255, 0, 0));
  */
  if (ui_data_current.f_btns & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(64, 64, 64));
  if (ui_data_current.f_btns_long_press & (1 << 2))
    strip.setPixelColor(rgb_led_fbtn_map[1], strip.Color(255, 0, 0));

  if (ui_data_current.f_btns & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(64, 64, 64));
  if (ui_data_current.f_btns_long_press & (1 << 0))
    strip.setPixelColor(rgb_led_fbtn_map[2], strip.Color(255, 0, 0));

  uint8_t b = ledStatus & 0xff;
  uint8_t g = (ledStatus >> 8) & 0xff;
  uint8_t r = (ledStatus >> 16) & 0xff;
  strip.setPixelColor(rgb_led_fbtn_map[0], strip.Color(r, g, b));
  //display->printf("%d\n", ledStatus);

  for(int i=0;i<5;i++){
    if(ui_data_current.f_btns & (1 << i)){
      buf[i] = '1';
    }
    else{
      buf[i] = '0';
    }
    if(ui_data_current.f_btns_long_press & (1 << i)){
      buf[i] = 'L';
    }
  }
  buf[5] = 0;
  display->printf("%s\n", buf);

  // print mcl buttons
  if (ui_data_current.mcl_btns & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(0, 255, 0));
  else
    strip.setPixelColor(rgb_led_mcl, strip.Color(64, 64, 64));
  if (ui_data_current.mcl_btns_long_press & (1 << 1))
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 0, 0));

  for(int i=0;i<13;i++){
    if(ui_data_current.mcl_btns & (1 << i)){
      buf[i] = '1';
    }
    else{
      buf[i] = '0';
    }
    if(ui_data_current.mcl_btns_long_press & (1 << i)){
      buf[i] = 'L';
    }
  }
  buf[13] = 0;
  display->printf("%s\n", buf);


  display->printf("FPS %dHz, MSPF %dms\n", 1000 / delta, delta);

  // in level bar
  uint16_t cy = display->getCursorY();
  display->fillRect(0, cy, r>>1, 4, SSD1309_WHITE);
  display->fillRect(0, cy+5, g>>1, 4, SSD1309_WHITE);
  if (b) display->fillCircle(128-4, 3, 2, SSD1309_WHITE);
  else display->drawCircle(128-4, 3, 2, SSD1309_WHITE);

  // 120 bpm indicator approx.
  if(bpm > 71){
    strip.setPixelColor(rgb_led_mcl, strip.Color(255, 255, 255));
    bpm = 0;
  }
  bpm++;

  display->display();
  strip.show();
}
