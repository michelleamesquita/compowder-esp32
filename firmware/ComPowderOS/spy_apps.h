#ifndef SPY_APPS_H
#define SPY_APPS_H

#include <Arduino.h>
#include <lvgl.h>

// Mesmo padrao do apps.h do ComPowderOS: cada app tem um "create" que monta
// a tela LVGL (uma vez, no setup) e um "update" chamado no loop().

void spyApps_init();                         // carrega spy/roupas salvas (chamar antes dos create)

lv_obj_t *wardrobeApp_create();              // guarda-roupa: Sam, Clover e Alex

lv_obj_t *jerryApp_create();                 // chamada da WOOHP
void jerryApp_update(unsigned long now);

lv_obj_t *missionApp_create();               // mini game
void missionApp_update(unsigned long now);
bool missionApp_capturesTouch();             // true = jogo rodando, o swipe nao troca de app

// Implementadas no ComPowderOS.ino
void compowder_gotoScreen(lv_obj_t *scr);    // pula direto para a tela de um app
bool compowder_wasSwipe();                   // o ultimo toque foi um swipe (ignorar o clique)

#endif
