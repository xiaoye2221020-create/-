#ifndef __ITEM_H
#define __ITEM_H
#include "main.h"
typedef struct {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t start_time_flage;//0,未开始,1,正在计时,2,暂停
} MenuWatch;
void StopWatch_Tick (void);
void DIno_Tick(void);
extern MenuWatch menu_watch;

void Menu_Timer(void);
void Menu_LED(void);
void Menu_MPU6050(void);
void Menu_Game(void);
void Menu_Emoji(void);
void Menu_Gradienter(void);
#endif
