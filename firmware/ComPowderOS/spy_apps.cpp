#include "spy_apps.h"
#include "sprites.h"
#include "custom_sprites.h"
#include <Preferences.h>

/* =========================================================================
   DADOS: spies, roupas e paleta
   Os sprites sao indexados (16 cores). O corpo e desenhado em camadas:
   cabelo de tras + corpo (por estilo de roupa) + rosto/franja. A roupa so
   troca a camada do corpo e as cores da paleta.
   ========================================================================= */
struct SpyDef {
  const char *name;
  uint32_t hair, hairShade, skin, skinShade, eye, suit;
};

static const SpyDef SPIES[3] = {
  { "SAM",    0xEE6923, 0xBE4614, 0xFFDEBE, 0xF0B496, 0x28A050, 0x3CBE5A },
  { "CLOVER", 0xFFD750, 0xE1AA28, 0xFFDABC, 0xF0B296, 0x3C82E6, 0xE6283C },
  { "ALEX",   0x231E2D, 0x504B6E, 0xCD9669, 0xAF7350, 0x784B23, 0xFACD28 },
};

enum { ST_CATSUIT = 0, ST_DRESS, ST_PANTS, ST_SKIRT };

struct OutfitDef {
  const char *name;
  uint8_t style;
  uint32_t a, b;      // cores da roupa (ST_CATSUIT usa a cor da spy)
  bool glasses;
};

// Para criar roupas novas, e so adicionar linhas aqui (max. CUSTOM_MAX_OUTFITS).
static const OutfitDef OUTFITS[] = {
  { "SPY SUIT", ST_CATSUIT, 0, 0, false },
  { "PASSEIO",  ST_SKIRT,   0xFFFFFF, 0x5A8CE6, false },
  { "FESTA",    ST_DRESS,   0xFF3C96, 0xFFDC3C, false },
  { "DISFARCE", ST_PANTS,   0x3C3C4B, 0x282834, true  },
  { "PIJAMA",   ST_PANTS,   0xAADCFF, 0x78B4FA, false },
  { "PRAIA",    ST_SKIRT,   0xFF963C, 0x3CC8BE, false },
};
static const uint8_t N_OUTFITS = sizeof(OUTFITS) / sizeof(OUTFITS[0]);

static const uint8_t (*const HAIR_BACK[3])[SPR_W] = { SPR_HAIR_BACK_0, SPR_HAIR_BACK_1, SPR_HAIR_BACK_2 };
static const uint8_t (*const HEAD[3])[SPR_W]      = { SPR_HEAD_0, SPR_HEAD_1, SPR_HEAD_2 };
static const uint8_t (*const BODY[4])[SPR_W]      = { SPR_BODY_0, SPR_BODY_1, SPR_BODY_2, SPR_BODY_3 };

static uint8_t curSpy = 0;
static uint8_t spyOutfit[3] = { 0, 0, 0 };
static Preferences prefs;

void spyApps_init() {
  prefs.begin("spies", false);
  curSpy = prefs.getUChar("spy", 0) % 3;
  prefs.getBytes("fits", spyOutfit, 3);
  for (int i = 0; i < 3; i++) spyOutfit[i] %= N_OUTFITS;
}

static void saveLook() {
  prefs.putUChar("spy", curSpy);
  prefs.putBytes("fits", spyOutfit, 3);
}

/* =========================================================================
   RENDER DOS SPRITES (pixel art ampliado, sem suavizar)
   ========================================================================= */
#define OUTLINE_TMP 200

// Desenha uma grade indexada no buffer do canvas, ampliando cada pixel "scale" vezes
static void blitIndexed(lv_color_t *buf, int scale, const uint8_t *g, int w, int h, const lv_color_t *pal) {
  const lv_color_t key = LV_COLOR_CHROMA_KEY;
  int bw = w * scale;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      uint8_t idx = g[y * w + x];
      lv_color_t c = idx ? pal[idx & 15] : key;
      for (int sy = 0; sy < scale; sy++)
        for (int sx = 0; sx < scale; sx++)
          buf[(y * scale + sy) * bw + x * scale + sx] = c;
    }
  }
}

static void renderSpy(lv_color_t *buf, int scale, uint8_t spy, uint8_t outfit) {
  const SpyDef &s = SPIES[spy];
  const OutfitDef &o = OUTFITS[outfit];

  // Imagem propria cadastrada em custom_sprites.h?
  const uint16_t *custom = (outfit < CUSTOM_MAX_OUTFITS) ? CUSTOM_SPR[spy][outfit] : nullptr;
  if (custom) {
    const lv_color_t key = LV_COLOR_CHROMA_KEY;
    int bw = SPR_W * scale;
    for (int y = 0; y < SPR_H; y++)
      for (int x = 0; x < SPR_W; x++) {
        uint16_t v = pgm_read_word(&custom[y * SPR_W + x]);
        lv_color_t c = (v == 0x07E0) ? key
                       : lv_color_make((v >> 11) << 3, ((v >> 5) & 0x3F) << 2, (v & 0x1F) << 3);
        for (int sy = 0; sy < scale; sy++)
          for (int sx = 0; sx < scale; sx++)
            buf[(y * scale + sy) * bw + x * scale + sx] = c;
      }
    return;
  }

  // Junta as camadas
  static uint8_t g[SPR_H][SPR_W];
  for (int y = 0; y < SPR_H; y++)
    for (int x = 0; x < SPR_W; x++) {
      uint8_t c = pgm_read_byte(&HEAD[spy][y][x]);
      if (!c) c = pgm_read_byte(&BODY[o.style][y][x]);
      if (!c) c = pgm_read_byte(&HAIR_BACK[spy][y][x]);
      g[y][x] = c;
    }
  if (o.glasses) {   // oculos escuros do disfarce
    for (int y = 9; y <= 11; y++)
      for (int x = 9; x <= 12; x++) { g[y][x] = 1; g[y][SPR_W - 1 - x] = 1; }
    g[10][13] = g[10][14] = 1;
  }
  // Contorno escuro em volta do personagem
  for (int y = 0; y < SPR_H; y++)
    for (int x = 0; x < SPR_W; x++) {
      if (g[y][x]) continue;
      bool n = (x > 0 && g[y][x - 1] && g[y][x - 1] != OUTLINE_TMP) ||
               (x < SPR_W - 1 && g[y][x + 1] && g[y][x + 1] != OUTLINE_TMP) ||
               (y > 0 && g[y - 1][x] && g[y - 1][x] != OUTLINE_TMP) ||
               (y < SPR_H - 1 && g[y + 1][x] && g[y + 1][x] != OUTLINE_TMP);
      if (n) g[y][x] = OUTLINE_TMP;
    }
  for (int y = 0; y < SPR_H; y++)
    for (int x = 0; x < SPR_W; x++)
      if (g[y][x] == OUTLINE_TMP) g[y][x] = 1;

  lv_color_t a = lv_color_hex(o.style == ST_CATSUIT ? s.suit : o.a);
  lv_color_t b = lv_color_hex(o.style == ST_CATSUIT ? s.suit : o.b);
  lv_color_t pal[16] = {
    LV_COLOR_CHROMA_KEY, lv_color_hex(0x281423),
    lv_color_hex(s.hair), lv_color_hex(s.hairShade), lv_color_hex(s.skin), lv_color_hex(s.skinShade),
    lv_color_white(), lv_color_hex(s.eye), lv_color_hex(0xD73C5F),
    a, lv_color_darken(a, 70), b, lv_color_darken(b, 70),
    lv_color_hex(0xCDCDD7), lv_color_hex(0xFF5AAA), lv_color_hex(0xF5F5F5),
  };
  blitIndexed(buf, scale, &g[0][0], SPR_W, SPR_H, pal);
}

static lv_obj_t *makeCanvas(lv_obj_t *parent, lv_color_t **buf, int w, int h) {
  *buf = (lv_color_t *)malloc(sizeof(lv_color_t) * w * h);
  lv_obj_t *cv = lv_canvas_create(parent);
  if (*buf) {
    for (int i = 0; i < w * h; i++) (*buf)[i] = LV_COLOR_CHROMA_KEY;
    lv_canvas_set_buffer(cv, *buf, w, h, LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED);
  }
  return cv;
}

static lv_obj_t *makeScreen(uint32_t bg) {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(bg), 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  return scr;
}

/* =========================================================================
   APP — GUARDA-ROUPA
   Setas = troca a roupa | bolinhas coloridas = escolhe Sam / Clover / Alex
   ========================================================================= */
#define WARD_SCALE 3
static lv_obj_t *screenWardrobe, *wardName, *wardOutfit, *wardCanvas, *wardDots[3];
static lv_color_t *wardBuf;

static void wardRefresh() {
  if (wardBuf) {
    renderSpy(wardBuf, WARD_SCALE, curSpy, spyOutfit[curSpy]);
    lv_obj_invalidate(wardCanvas);
  }
  lv_label_set_text(wardName, SPIES[curSpy].name);
  lv_obj_set_style_text_color(wardName, lv_color_darken(lv_color_hex(SPIES[curSpy].suit), 60), 0);
  lv_label_set_text(wardOutfit, OUTFITS[spyOutfit[curSpy]].name);
  for (int i = 0; i < 3; i++)
    lv_obj_set_style_border_width(wardDots[i], i == curSpy ? 3 : 0, 0);
}

static void wardArrowCb(lv_event_t *e) {
  if (compowder_wasSwipe()) return;
  int dir = (int)(intptr_t)lv_event_get_user_data(e);
  spyOutfit[curSpy] = (spyOutfit[curSpy] + N_OUTFITS + dir) % N_OUTFITS;
  saveLook();
  wardRefresh();
}

static void wardDotCb(lv_event_t *e) {
  if (compowder_wasSwipe()) return;
  curSpy = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  saveLook();
  wardRefresh();
}

static lv_obj_t *wardArrow(const char *symbol, lv_align_t align, int x, int dir) {
  lv_obj_t *btn = lv_btn_create(screenWardrobe);
  lv_obj_set_size(btn, 40, 40);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF50AA), 0);
  lv_obj_align(btn, align, x, 0);
  lv_obj_add_event_cb(btn, wardArrowCb, LV_EVENT_CLICKED, (void *)(intptr_t)dir);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, symbol);
  lv_obj_center(lbl);
  return btn;
}

lv_obj_t *wardrobeApp_create() {
  screenWardrobe = makeScreen(0xE1C8FA);

  wardName = lv_label_create(screenWardrobe);
  lv_obj_align(wardName, LV_ALIGN_TOP_MID, 0, 16);

  wardCanvas = makeCanvas(screenWardrobe, &wardBuf, SPR_W * WARD_SCALE, SPR_H * WARD_SCALE);
  lv_obj_align(wardCanvas, LV_ALIGN_CENTER, 0, -8);

  wardArrow(LV_SYMBOL_LEFT,  LV_ALIGN_LEFT_MID,  12, -1);
  wardArrow(LV_SYMBOL_RIGHT, LV_ALIGN_RIGHT_MID, -12, +1);

  wardOutfit = lv_label_create(screenWardrobe);
  lv_obj_set_style_text_color(wardOutfit, lv_color_hex(0x7A3CA8), 0);
  lv_obj_align(wardOutfit, LV_ALIGN_BOTTOM_MID, 0, -42);

  for (int i = 0; i < 3; i++) {
    lv_obj_t *d = lv_btn_create(screenWardrobe);
    lv_obj_set_size(d, 24, 24);
    lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(d, lv_color_hex(SPIES[i].suit), 0);
    lv_obj_set_style_border_color(d, lv_color_white(), 0);
    lv_obj_set_style_shadow_width(d, 0, 0);
    lv_obj_align(d, LV_ALIGN_BOTTOM_MID, (i - 1) * 32, -12);
    lv_obj_add_event_cb(d, wardDotCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    wardDots[i] = d;
  }

  wardRefresh();
  return screenWardrobe;
}

/* =========================================================================
   APP — JERRY (chamada da WOOHP)
   Botao de ligar -> "chamando" -> Jerry aparece no monitor e passa a missao.
   Toque na tela para avancar a fala. Tambem mostra o resultado da missao.
   ========================================================================= */
#define JERRY_SCALE 3
enum { JS_IDLE, JS_RINGING, JS_TALK };
enum { JM_BRIEF, JM_WIN, JM_LOSE };

static lv_obj_t *screenJerry, *jerryCallBtn, *jerryTitle, *jerrySpinner, *jerryRingLbl;
static lv_obj_t *jerryPanel, *jerryCanvas, *jerryText, *jerryHint;
static lv_color_t *jerryBuf;
static uint8_t jerryState = JS_IDLE, jerryMode = JM_BRIEF, jerryLine = 0, jerryFrame = 0;
static unsigned long jerryT0 = 0, jerryLastFrame = 0;

// %s = nome da spy escolhida (a fonte padrao do LVGL nao tem acentos)
static const char *const J_BRIEF[] = {
  "Ola, spies! Aqui e o Jerry.",
  "Temos lasers a solta no shopping de Beverly Hills!",
  "%s, desvie dos lasers e recolha 10 gadgets.",
  "Boa sorte, spies!",
};
static const char *const J_WIN[]  = { "Missao cumprida! Otimo trabalho, %s!", "A WOOHP agradece." };
static const char *const J_LOSE[] = { "Que pena, foi por pouco...", "Tente de novo, %s!" };

static const lv_color_t *jerryPalette() {
  static lv_color_t pal[16];
  static const uint32_t hex[11] = { 0, 0x19191E, 0xAAAAB2, 0x82828C, 0xF0C8AA, 0xCDA082,
                                    0xFFFFFF, 0x000000, 0x8C1E28, 0x282832, 0x464655 };
  for (int i = 0; i < 16; i++) pal[i] = lv_color_hex(i < 11 ? hex[i] : 0);
  return pal;
}

static void jerryDraw(uint8_t frame) {
  if (!jerryBuf) return;
  blitIndexed(jerryBuf, JERRY_SCALE, frame ? &SPR_JERRY_1[0][0] : &SPR_JERRY_0[0][0],
              JERRY_W, JERRY_H, jerryPalette());
  lv_obj_invalidate(jerryCanvas);
}

static void setHidden(lv_obj_t *o, bool hidden) {
  if (hidden) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static const char *const *jerryLines(uint8_t &n) {
  if (jerryMode == JM_BRIEF) { n = sizeof(J_BRIEF) / sizeof(J_BRIEF[0]); return J_BRIEF; }
  if (jerryMode == JM_WIN)   { n = sizeof(J_WIN) / sizeof(J_WIN[0]);     return J_WIN; }
  n = sizeof(J_LOSE) / sizeof(J_LOSE[0]);
  return J_LOSE;
}

static void jerrySetState(uint8_t st) {
  jerryState = st;
  setHidden(jerryCallBtn, st != JS_IDLE);
  setHidden(jerryTitle,   st != JS_IDLE);
  setHidden(jerrySpinner, st != JS_RINGING);
  setHidden(jerryRingLbl, st != JS_RINGING);
  setHidden(jerryPanel,   st != JS_TALK);
  setHidden(jerryText,    st != JS_TALK);
  setHidden(jerryHint,    st != JS_TALK);
  if (st == JS_RINGING) jerryT0 = millis();
  if (st == JS_TALK) {
    uint8_t n;
    const char *const *lines = jerryLines(n);
    lv_label_set_text_fmt(jerryText, lines[jerryLine], SPIES[curSpy].name);
    jerryDraw(0);
  }
}

static void missionStart();
static lv_obj_t *screenMission;

static void jerryCallCb(lv_event_t *e) {
  if (compowder_wasSwipe()) return;
  jerryMode = JM_BRIEF;
  jerryLine = 0;
  jerrySetState(JS_RINGING);
}

static void jerryScreenCb(lv_event_t *e) {
  if (compowder_wasSwipe() || jerryState != JS_TALK) return;
  if (millis() - jerryT0 < 600) return;   // evita pular a fala ao soltar o dedo do jogo
  uint8_t n;
  jerryLines(n);
  if (++jerryLine < n) { jerrySetState(JS_TALK); return; }
  jerryLine = 0;
  jerrySetState(JS_IDLE);
  if (jerryMode == JM_BRIEF) {          // aceitou a missao
    missionStart();
    compowder_gotoScreen(screenMission);
  }
}

static void jerryShowResult(bool win) {
  jerryMode = win ? JM_WIN : JM_LOSE;
  jerryLine = 0;
  jerryT0 = millis();
  jerrySetState(JS_TALK);
  compowder_gotoScreen(screenJerry);
}

lv_obj_t *jerryApp_create() {
  screenJerry = makeScreen(0x230C2D);
  lv_obj_add_event_cb(screenJerry, jerryScreenCb, LV_EVENT_CLICKED, NULL);

  // --- parado: botao de ligar ---
  jerryTitle = lv_label_create(screenJerry);
  lv_label_set_text(jerryTitle, "WOOHP");
  lv_obj_set_style_text_color(jerryTitle, lv_color_hex(0xFF50AA), 0);
  lv_obj_align(jerryTitle, LV_ALIGN_TOP_MID, 0, 28);

  jerryCallBtn = lv_btn_create(screenJerry);
  lv_obj_set_size(jerryCallBtn, 110, 110);
  lv_obj_set_style_radius(jerryCallBtn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(jerryCallBtn, lv_color_hex(0xFF50AA), 0);
  lv_obj_align(jerryCallBtn, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_event_cb(jerryCallBtn, jerryCallCb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *callLbl = lv_label_create(jerryCallBtn);
  lv_label_set_text(callLbl, LV_SYMBOL_CALL "\nLIGAR\nJERRY");
  lv_obj_set_style_text_align(callLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(callLbl);

  // --- chamando ---
  jerrySpinner = lv_spinner_create(screenJerry, 1000, 60);
  lv_obj_set_size(jerrySpinner, 150, 150);
  lv_obj_center(jerrySpinner);
  lv_obj_set_style_arc_color(jerrySpinner, lv_color_hex(0xFF50AA), LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(jerrySpinner, lv_color_hex(0x4A2060), LV_PART_MAIN);
  lv_obj_clear_flag(jerrySpinner, LV_OBJ_FLAG_CLICKABLE);
  jerryRingLbl = lv_label_create(screenJerry);
  lv_label_set_text(jerryRingLbl, "CHAMANDO\nJERRY...");
  lv_obj_set_style_text_align(jerryRingLbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(jerryRingLbl, lv_color_white(), 0);
  lv_obj_center(jerryRingLbl);

  // --- conversa: monitor azul com o Jerry + legenda ---
  jerryPanel = lv_obj_create(screenJerry);
  lv_obj_remove_style_all(jerryPanel);
  lv_obj_set_size(jerryPanel, 116, 104);
  lv_obj_set_style_radius(jerryPanel, 14, 0);
  lv_obj_set_style_bg_color(jerryPanel, lv_color_hex(0x466EAA), 0);
  lv_obj_set_style_bg_opa(jerryPanel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(jerryPanel, lv_color_hex(0xFF50AA), 0);
  lv_obj_set_style_border_width(jerryPanel, 3, 0);
  lv_obj_set_style_clip_corner(jerryPanel, true, 0);
  lv_obj_align(jerryPanel, LV_ALIGN_TOP_MID, 0, 18);
  lv_obj_clear_flag(jerryPanel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  jerryCanvas = makeCanvas(jerryPanel, &jerryBuf, JERRY_W * JERRY_SCALE, JERRY_H * JERRY_SCALE);
  lv_obj_align(jerryCanvas, LV_ALIGN_BOTTOM_MID, 0, 0);

  jerryText = lv_label_create(screenJerry);
  lv_label_set_long_mode(jerryText, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(jerryText, 176);
  lv_obj_set_style_text_align(jerryText, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(jerryText, lv_color_white(), 0);
  lv_obj_align(jerryText, LV_ALIGN_TOP_MID, 0, 128);

  jerryHint = lv_label_create(screenJerry);
  lv_label_set_text(jerryHint, LV_SYMBOL_DOWN);
  lv_obj_set_style_text_color(jerryHint, lv_color_hex(0xFF50AA), 0);
  lv_obj_align(jerryHint, LV_ALIGN_BOTTOM_MID, 0, -14);

  jerrySetState(JS_IDLE);
  return screenJerry;
}

void jerryApp_update(unsigned long now) {
  if (lv_scr_act() != screenJerry) return;
  if (jerryState == JS_RINGING && now - jerryT0 > 2200) jerrySetState(JS_TALK);
  if (jerryState == JS_TALK && now - jerryLastFrame > 220) {   // boca mexendo
    jerryLastFrame = now;
    jerryFrame ^= 1;
    jerryDraw(jerryFrame);
  }
}

/* =========================================================================
   APP — MISSAO (mini game)
   Arraste o dedo para mover a spy (com a roupa escolhida no guarda-roupa).
   Desvie dos lasers vermelhos e pegue 10 gadgets (mini compowders).
   ========================================================================= */
#define PLAYER_SCALE 2
#define PLAYER_W (SPR_W * PLAYER_SCALE)
#define PLAYER_H (SPR_H * PLAYER_SCALE)
#define PLAYER_Y 120
#define N_FALL 6
#define GOAL 10

struct Fall { lv_obj_t *obj; bool on, gadget; float x, y, v; int w, h; };

static lv_obj_t *missionHud, *missionMsg, *playerCanvas;
static lv_color_t *playerBuf;
static Fall falls[N_FALL];
static bool missionPlaying = false;
static float playerX = 120, targetX = 120;
static int score = 0, lives = 3;
static unsigned long lastStep = 0, lastSpawn = 0, hitUntil = 0;

bool missionApp_capturesTouch() { return missionPlaying && lv_scr_act() == screenMission; }

static void missionHudRefresh() {
  lv_label_set_text_fmt(missionHud, "%d/%d  x%d", score, GOAL, lives);
}

static void missionDrawPlayer() {
  if (!playerBuf) return;
  renderSpy(playerBuf, PLAYER_SCALE, curSpy, spyOutfit[curSpy]);
  lv_obj_invalidate(playerCanvas);
}

static void missionReset() {
  for (int i = 0; i < N_FALL; i++) { falls[i].on = false; lv_obj_add_flag(falls[i].obj, LV_OBJ_FLAG_HIDDEN); }
  score = 0; lives = 3; playerX = targetX = 120; hitUntil = 0;
  lv_obj_set_pos(playerCanvas, (int)playerX - PLAYER_W / 2, PLAYER_Y);
  lv_obj_set_style_bg_color(screenMission, lv_color_hex(0x230C2D), 0);
  missionHudRefresh();
  missionDrawPlayer();
}

static void missionStart() {
  missionReset();
  missionPlaying = true;
  lastSpawn = lastStep = millis();
  lv_obj_add_flag(missionMsg, LV_OBJ_FLAG_HIDDEN);
}

static void missionEnd(bool win) {
  missionPlaying = false;
  missionReset();
  lv_obj_clear_flag(missionMsg, LV_OBJ_FLAG_HIDDEN);
  jerryShowResult(win);
}

static void missionTouchCb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SCREEN_LOAD_START) {         // roupa pode ter mudado no guarda-roupa
    if (!missionPlaying) missionReset();
  } else if (code == LV_EVENT_PRESSING && missionPlaying) {
    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);
    targetX = constrain(p.x, 62, 178);
  } else if (code == LV_EVENT_CLICKED && !missionPlaying && !compowder_wasSwipe()) {
    missionStart();
  }
}

static void spawnFall() {
  for (int i = 0; i < N_FALL; i++) {
    Fall &f = falls[i];
    if (f.on) continue;
    f.on = true;
    f.gadget = random(100) < 45;
    f.x = random(62, 179);
    f.y = 44;
    f.v = 1.6f + random(0, 14) / 10.0f + score * 0.08f;
    if (f.gadget) {
      f.w = 16; f.h = 16;
      lv_obj_set_style_radius(f.obj, LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_color(f.obj, lv_color_hex(0xFF50AA), 0);
      lv_obj_set_style_border_width(f.obj, 3, 0);
    } else {
      f.w = 6; f.h = 22;
      lv_obj_set_style_radius(f.obj, 3, 0);
      lv_obj_set_style_bg_color(f.obj, lv_color_hex(0xFF2828), 0);
      lv_obj_set_style_border_width(f.obj, 0, 0);
    }
    lv_obj_set_size(f.obj, f.w, f.h);
    lv_obj_set_pos(f.obj, (int)f.x - f.w / 2, (int)f.y - f.h / 2);
    lv_obj_clear_flag(f.obj, LV_OBJ_FLAG_HIDDEN);
    return;
  }
}

lv_obj_t *missionApp_create() {
  screenMission = makeScreen(0x230C2D);
  lv_obj_add_event_cb(screenMission, missionTouchCb, LV_EVENT_ALL, NULL);

  lv_obj_t *floor = lv_obj_create(screenMission);
  lv_obj_remove_style_all(floor);
  lv_obj_set_size(floor, 240, 40);
  lv_obj_set_pos(floor, 0, 200);
  lv_obj_set_style_bg_color(floor, lv_color_hex(0x46235A), 0);
  lv_obj_set_style_bg_opa(floor, LV_OPA_COVER, 0);

  missionHud = lv_label_create(screenMission);
  lv_obj_set_style_text_color(missionHud, lv_color_white(), 0);
  lv_obj_align(missionHud, LV_ALIGN_TOP_MID, 0, 16);

  for (int i = 0; i < N_FALL; i++) {
    lv_obj_t *o = lv_obj_create(screenMission);
    lv_obj_remove_style_all(o);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_white(), 0);
    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    falls[i].obj = o;
    falls[i].on = false;
  }

  playerCanvas = makeCanvas(screenMission, &playerBuf, PLAYER_W, PLAYER_H);

  missionMsg = lv_label_create(screenMission);
  lv_label_set_text(missionMsg, "MISSAO\ntoque para iniciar");
  lv_obj_set_style_text_align(missionMsg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(missionMsg, lv_color_hex(0xFF50AA), 0);
  lv_obj_align(missionMsg, LV_ALIGN_TOP_MID, 0, 56);

  // nada alem da tela recebe o toque
  for (uint32_t i = 0; i < lv_obj_get_child_cnt(screenMission); i++)
    lv_obj_clear_flag(lv_obj_get_child(screenMission, i), LV_OBJ_FLAG_CLICKABLE);

  missionReset();
  return screenMission;
}

void missionApp_update(unsigned long now) {
  if (lv_scr_act() != screenMission || !missionPlaying) return;
  if (now - lastStep < 30) return;
  lastStep = now;

  playerX += (targetX - playerX) * 0.35f;
  lv_obj_set_pos(playerCanvas, (int)playerX - PLAYER_W / 2, PLAYER_Y);

  unsigned long interval = (unsigned long)max(380, 850 - score * 40);
  if (now - lastSpawn > interval) { lastSpawn = now; spawnFall(); }

  for (int i = 0; i < N_FALL; i++) {
    Fall &f = falls[i];
    if (!f.on) continue;
    f.y += f.v;
    bool caught = f.y > PLAYER_Y + 4 && f.y < PLAYER_Y + 70 && fabsf(f.x - playerX) < 20;
    if (caught || f.y > 215) {
      f.on = false;
      lv_obj_add_flag(f.obj, LV_OBJ_FLAG_HIDDEN);
      if (caught) {
        if (f.gadget) score++;
        else {
          lives--;
          hitUntil = now + 200;
          lv_obj_set_style_bg_color(screenMission, lv_color_hex(0x7A1428), 0);
        }
        missionHudRefresh();
      }
      continue;
    }
    lv_obj_set_pos(f.obj, (int)f.x - f.w / 2, (int)f.y - f.h / 2);
  }

  if (hitUntil && now > hitUntil) {
    hitUntil = 0;
    lv_obj_set_style_bg_color(screenMission, lv_color_hex(0x230C2D), 0);
  }

  if (score >= GOAL) missionEnd(true);
  else if (lives <= 0) missionEnd(false);
}
