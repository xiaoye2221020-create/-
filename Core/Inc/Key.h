#ifndef __KEY_H
#define __KEY_H

#include "main.h"

// 按键返回值定义
#define KEY_NONE        0
#define KEY_1           1   // 上键短按
#define KEY_2           2   // 下键短按  
#define KEY_3           3   // 确认键短按
#define KEY_3_LONG      4   // 确认键长按

// 全局按键值（兼容性，供外部直接使用）
extern uint8_t Key_Num;

// 按键硬件定义
#define KEY1_PORT       GPIOB
#define KEY1_PIN        GPIO_PIN_1
#define KEY2_PORT       GPIOA
#define KEY2_PIN        GPIO_PIN_4
#define KEY3_PORT       GPIOA
#define KEY3_PIN        GPIO_PIN_6

// 按键参数配置（基于10ms定时器周期）
#define KEY_DEBOUNCE_MS     30      // 消抖时间 30ms
#define KEY_LONG_PRESS_MS   800     // 长按时间 800ms
#define KEY_SCAN_INTERVAL   10      // 扫描间隔 10ms

#define DEBOUNCE_CNT        (KEY_DEBOUNCE_MS / KEY_SCAN_INTERVAL)      // 3
#define LONG_PRESS_CNT      (KEY_LONG_PRESS_MS / KEY_SCAN_INTERVAL)    // 80

// 按键状态机状态
typedef enum {
    KEY_STATE_IDLE = 0,         // 空闲
    KEY_STATE_DEBOUNCE,         // 消抖中
    KEY_STATE_PRESSED,          // 已确认按下
    KEY_STATE_LONG_WAIT_RELEASE // 长按已触发，等待释放
} KeyState_t;

// 按键数据结构
typedef struct {
    KeyState_t state;           // 当前状态
    uint8_t debounce_cnt;       // 消抖计数
    uint16_t hold_cnt;          // 按住计数（用于长按）
    GPIO_TypeDef* port;         // GPIO端口
    uint16_t pin;               // GPIO引脚
    uint8_t key_code;           // 按键编码（短按）
    uint8_t key_code_long;      // 按键编码（长按）
} Key_t;

void Key_Init(void);
void Key_Scan(void);
uint8_t Key_GetNum(void);

// 兼容性宏，保持原有代码可用

#endif
