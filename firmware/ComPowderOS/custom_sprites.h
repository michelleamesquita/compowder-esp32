// Sprites proprios (opcional). Para usar uma imagem sua no lugar do pixel art embutido:
//   python3 ../../tools/png2sprite.py sam_festa.png SAM_FESTA >> custom_sprites_data.h
// depois inclua o arquivo gerado aqui e aponte a posicao [spy][roupa] para o array.
// Formato: 28x44 pixels RGB565, 0x07E0 = transparente. nullptr = usa o sprite embutido.
#ifndef CUSTOM_SPRITES_H
#define CUSTOM_SPRITES_H
#include <Arduino.h>

#define CUSTOM_MAX_OUTFITS 12

// #include "custom_sprites_data.h"

// linhas: 0 = Sam, 1 = Clover, 2 = Alex | colunas: mesma ordem da tabela OUTFITS
static const uint16_t *const CUSTOM_SPR[3][CUSTOM_MAX_OUTFITS] = {
  { nullptr },   // ex.: { nullptr, nullptr, SAM_FESTA }
  { nullptr },
  { nullptr },
};

#endif
