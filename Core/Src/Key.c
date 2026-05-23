#include "main.h"
#include "gpio.h"
#include "Key.h"
#include "OLED.h"
#include "cmsis_os.h"

extern osMessageQueueId_t keyQueueHandle;
extern osTimerId_t oledSleepTimerHandle;
extern uint8_t oled_asleep;

uint8_t Key_Num = KEY_NONE;

// 三个按键的状态
static Key_t keys[3] = {
    {KEY_STATE_IDLE, 0, 0, KEY1_PORT, KEY1_PIN, KEY_1, KEY_NONE},  // 按键1 - 只有短按
    {KEY_STATE_IDLE, 0, 0, KEY2_PORT, KEY2_PIN, KEY_2, KEY_NONE},  // 按键2 - 只有短按
    {KEY_STATE_IDLE, 0, 0, KEY3_PORT, KEY3_PIN, KEY_3, KEY_3_LONG} // 按键3 - 短按+长按
};

static uint8_t key_value = KEY_NONE;  // 按键返回值

/**
 * @brief 读取按键原始状态（低电平有效）
 */
static uint8_t Key_ReadRaw(Key_t* key)
{
    return (HAL_GPIO_ReadPin(key->port, key->pin) == GPIO_PIN_RESET);
}

/**
 * @brief 处理单个按键的状态机
 */
static void Key_Process(Key_t* key)
{
    uint8_t pressed = Key_ReadRaw(key);
    
    switch (key->state) {
        case KEY_STATE_IDLE:
            if (pressed) {
                key->debounce_cnt = 0;
                key->state = KEY_STATE_DEBOUNCE;
            }
            break;
            
        case KEY_STATE_DEBOUNCE:
            if (pressed) {
                if (++key->debounce_cnt >= DEBOUNCE_CNT) {
                    // 消抖通过，确认按下
                    key->hold_cnt = 0;
                    key->state = KEY_STATE_PRESSED;
                }
            } else {
                // 抖动，回到空闲
                key->state = KEY_STATE_IDLE;
            }
            break;
            
        case KEY_STATE_PRESSED:
            if (pressed) {
                key->hold_cnt++;
                // 检查是否达到长按条件（只有配置了长按编码的按键才检测）
                if (key->key_code_long != KEY_NONE && key->hold_cnt >= LONG_PRESS_CNT) {
                    key_value = key->key_code_long;
                    key->state = KEY_STATE_LONG_WAIT_RELEASE;
                }
            } else {
                // 松开，触发短按
                key_value = key->key_code;
                key->state = KEY_STATE_IDLE;
            }
            break;
            
        case KEY_STATE_LONG_WAIT_RELEASE:
            // 长按已触发，等待释放
            if (!pressed) {
                key->state = KEY_STATE_IDLE;
            }
            break;
            
        default:
            key->state = KEY_STATE_IDLE;
            break;
    }
}

/**
 * @brief 按键初始化（可在main中调用）
 */
void Key_Init(void)
{
    key_value = KEY_NONE;
    for (int i = 0; i < 3; i++) {
        keys[i].state = KEY_STATE_IDLE;
        keys[i].debounce_cnt = 0;
        keys[i].hold_cnt = 0;
    }
}

/**
 * @brief 按键扫描（在定时器中断中调用，建议10ms周期）
 */
void Key_Scan(void)
{
    for (int i = 0; i < 3; i++) {
        Key_Process(&keys[i]);
        if (key_value != KEY_NONE) {
            Key_Num = key_value;
            osMessageQueuePut(keyQueueHandle, &key_value, 0, 0);
            key_value = KEY_NONE;
            break;
        }
    }
}

/**
 * @brief 获取按键值（在主循环中调用）
 * @return 按键值，读取后自动清零
 */
uint8_t Key_GetNum(void)
{
    uint8_t key = KEY_NONE;
    osMessageQueueGet(keyQueueHandle, &key, NULL, 0);
    Key_Num = KEY_NONE;
    if (key != KEY_NONE)
    {
        if (oled_asleep)
        {
            if (key != KEY_3 && key != KEY_3_LONG) return KEY_NONE;
            oled_asleep = 0;
            OLED_WriteCommand(0xAF);
            osTimerStop(oledSleepTimerHandle);
            osTimerStart(oledSleepTimerHandle, 10000);
            return KEY_NONE;
        }
        osTimerStop(oledSleepTimerHandle);
        osTimerStart(oledSleepTimerHandle, 10000);
    }
    return key;
}
