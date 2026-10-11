/*******************************************************************************
  ComPowder OS + Totally Spies — shell LVGL (GC9A01 1.28" + CST816S)

  Baseado no ComPowderOS de esha-ahmed/Totally-Spies-ComPowder: mesma estrutura
  (cada app e uma tela LVGL, swipe para a esquerda/direita troca de app), com
  tres apps novos em spy_apps.cpp:
    1. GUARDA-ROUPA  escolhe Sam / Clover / Alex e troca as roupas (pixel art 8-bit)
    2. JERRY         liga para a WOOHP; o Jerry passa a missao
    3. MISSAO        mini game: desvie dos lasers e pegue 10 gadgets

  Bibliotecas (as mesmas do projeto original):
    Arduino_GFX_Library   1.4.7
    lvgl                  8.3.x   <-- usar a 8.3, nao a 9
  lv_conf.h: LV_COLOR_DEPTH 16. Funciona com LV_COLOR_16_SWAP 0 ou 1 e com
  LV_TICK_CUSTOM 0 ou 1. Precisa de LV_USE_CANVAS, LV_USE_SPINNER e
  LV_FONT_MONTSERRAT_14 (ja sao o padrao).

  Arduino IDE: Board "ESP32S3 Dev Module", USB CDC On Boot: Enabled.
  Waveshare ESP32-S3-Touch-LCD-1.28: Flash 16MB, PSRAM "QSPI PSRAM".
*******************************************************************************/

#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <lvgl.h>
#include "spy_apps.h"

// 1 = Waveshare ESP32-S3-Touch-LCD-1.28 | 0 = placa do repo original (ESP32S3-128SPIT)
#define BOARD_WAVESHARE_128 1

// 1 = inclui tambem Radar / Spotify / Net Scanner do repo original.
// Para isso, copie apps.h e apps.cpp do repo para esta mesma pasta.
#define USE_REPO_APPS 0
#if USE_REPO_APPS
#include "apps.h"
#endif

#if BOARD_WAVESHARE_128
  #define TFT_SCK    10
  #define TFT_MOSI   11
  #define TFT_CS     9
  #define TFT_DC     8
  #define TFT_RST    14
  #define TFT_BLK    2
  #define TOUCH_SDA  6
  #define TOUCH_SCL  7
  #define TOUCH_RST  13
  #define TOUCH_INT  5
#else
  #define TFT_SCK    3
  #define TFT_MOSI   10
  #define TFT_CS     2
  #define TFT_DC     18
  #define TFT_RST    21
  #define TFT_BLK    42
  #define TOUCH_SDA  8
  #define TOUCH_SCL  9
  #define TOUCH_RST  0
  #define TOUCH_INT  11
#endif
#define TOUCH_ADDR 0x15

// ================== HARDWARE ==================
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, -1, HSPI);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);

// ================== LVGL BUFFER ==================
static const uint32_t SCREEN_W = 240;
static const uint32_t SCREEN_H = 240;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// ================== APP MANAGER ==================
// Ordem de navegacao (swipe esquerda = proximo, direita = anterior)
enum AppId {
  APP_WARDROBE = 0, APP_JERRY, APP_MISSION,
#if USE_REPO_APPS
  APP_RADAR, APP_MUSIC, APP_NETSCAN,
#endif
  APP_COUNT
};
static lv_obj_t *screens[APP_COUNT];
static int currentApp = APP_WARDROBE;

static void loadApp(int idx, lv_scr_load_anim_t anim) {
  currentApp = (idx + APP_COUNT) % APP_COUNT;
  lv_scr_load_anim(screens[currentApp], anim, 180, 0, false);
}
static void nextApp() { loadApp(currentApp + 1, LV_SCR_LOAD_ANIM_MOVE_LEFT); }
static void prevApp() { loadApp(currentApp - 1, LV_SCR_LOAD_ANIM_MOVE_RIGHT); }

// Usado pelos apps das spies (Jerry -> Missao -> Jerry)
void compowder_gotoScreen(lv_obj_t *scr) {
  for (int i = 0; i < APP_COUNT; i++) {
    if (screens[i] == scr) {
      if (i != currentApp) loadApp(i, LV_SCR_LOAD_ANIM_FADE_ON);
      return;
    }
  }
}

// ================== DISPLAY FLUSH CALLBACK ==================
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if LV_COLOR_16_SWAP
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp);
}

// ================== TOUCH ==================
// Mesmo burst-read do original (registro 0x01, 6 bytes), mas lido a cada ciclo
// para o LVGL enxergar o dedo arrastando (o mini game precisa disso). O swipe
// de troca de app e calculado aqui pelo deslocamento entre encostar e soltar,
// em vez do registro de gesto do chip.
static bool tpDown = false;
static int16_t tpX = 120, tpY = 120, tpStartX = 120, tpStartY = 120;
static unsigned long tpLastSeen = 0;
static bool lastWasSwipe = false;
static int pendingNav = 0;

bool compowder_wasSwipe() { return lastWasSwipe; }

static bool readTouchController(int16_t &x, int16_t &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x01);
  if (Wire.endTransmission(false) != 0) return false;   // chip dormindo = sem toque
  Wire.requestFrom(TOUCH_ADDR, 6, true);
  if (Wire.available() < 6) return false;

  Wire.read();                 // gesture (nao usado)
  byte points  = Wire.read();
  byte x_high  = Wire.read();
  byte x_low   = Wire.read();
  byte y_high  = Wire.read();
  byte y_low   = Wire.read();
  if (points == 0 || points > 2) return false;

  x = constrain(((x_high & 0x0F) << 8) | x_low, 0, (int)SCREEN_W - 1);
  y = constrain(((y_high & 0x0F) << 8) | y_low, 0, (int)SCREEN_H - 1);
  return true;
}

void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  int16_t x, y;
  unsigned long now = millis();
  if (readTouchController(x, y)) {
    if (!tpDown) { tpDown = true; tpStartX = x; tpStartY = y; lastWasSwipe = false; }
    tpX = x; tpY = y; tpLastSeen = now;
  } else if (tpDown && now - tpLastSeen > 50) {
    tpDown = false;
    int dx = tpX - tpStartX, dy = tpY - tpStartY;
    if (abs(dx) > 50 && abs(dx) > 2 * abs(dy)) {
      lastWasSwipe = true;                              // os apps ignoram esse "clique"
      if (!missionApp_capturesTouch()) pendingNav = (dx < 0) ? +1 : -1;
    }
  }
  data->point.x = tpX;
  data->point.y = tpY;
  data->state = tpDown ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // Touch I2C
  pinMode(TOUCH_INT, INPUT_PULLUP);
  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(10);
  digitalWrite(TOUCH_RST, HIGH);
  delay(50);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);
  Wire.beginTransmission(TOUCH_ADDR);   // desliga o auto-sleep do CST816S
  Wire.write(0xFE);
  Wire.write(0x01);
  Wire.endTransmission();

  // Display
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  if (!gfx->begin()) Serial.println("Erro no display!");
  gfx->fillScreen(0x0000);

  // LVGL init
  lv_init();
  buf1 = (lv_color_t *)heap_caps_malloc(SCREEN_W * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
  if (!buf1) {
    Serial.println("ERRO: falha ao alocar o buffer do LVGL");
    while (1) delay(1000);
  }
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_W * 20);

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_W;
  disp_drv.ver_res = SCREEN_H;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  // Apps como telas LVGL independentes
  randomSeed(esp_random());
  spyApps_init();
  screens[APP_WARDROBE] = wardrobeApp_create();
  screens[APP_JERRY]    = jerryApp_create();
  screens[APP_MISSION]  = missionApp_create();
#if USE_REPO_APPS
  screens[APP_RADAR]    = radarApp_create();
  screens[APP_MUSIC]    = musicApp_create();
  screens[APP_NETSCAN]  = netscanApp_create();
#endif

  lv_scr_load(screens[APP_WARDROBE]);
  Serial.println("ComPowder OS: boot completo!");
}

// ================== LOOP ==================
void loop() {
  unsigned long now = millis();

#if !LV_TICK_CUSTOM
  static unsigned long lastTick = 0;
  lv_tick_inc(now - lastTick);
  lastTick = now;
#endif

  jerryApp_update(now);
  missionApp_update(now);
#if USE_REPO_APPS
  radarApp_update(now);
  musicApp_update(now);
  netscanApp_update(now);
#endif

  lv_timer_handler();

  if (pendingNav) {            // swipe detectado no my_touch_read
    if (pendingNav > 0) nextApp(); else prevApp();
    pendingNav = 0;
  }
  delay(5);
}
