#ifndef __MENU_H
#define __MENU_H

typedef struct {
    uint8_t pre_selection; // 上一次选择的菜单项
    uint8_t target_selection; // 当前选择的菜单项
    uint8_t x_pre; // 上一次选择的菜单项的x坐标
    uint8_t Speed; // 动画速度
    uint8_t move_flag; // 是否正在移动.1,正在移动,0,停止
} MenuItem;
extern MenuItem menu_item;
void Paper1_Init(void);
void Show_Clock_UI(void);
int First_Page_Clack(void);
void Show_SettingPage(void);
int SettingPage(void);
int Menu(void);
#endif
