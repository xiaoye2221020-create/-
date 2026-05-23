# 手表项目

## 硬件平台
- MCU: STM32F103C8T6 (Cortex-M3, 64KB Flash, 20KB SRAM, LQFP48)
- 外设:
  - OLED 显示屏 (0.96", I2C2, 地址 0x3C)
  - MPU6050 六轴传感器 (I2C1)
  - 4 个按键 (PA0/PA4/PA6/PB1, 上拉输入)
  - LED (PB8/PB9)
  - 蜂鸣器 (PB13)
  - ADC 电池电压检测 (PA0, ADC1_CH0)
  - RTC 实时时钟 (外部 32.768kHz 晶振)
  - TIM2 定时器 (用于按键扫描、菜单刷新等)

## 软件架构
- 固件库: STM32 HAL + FreeRTOS v10.3.1 (CMSIS-RTOS v2 API)
- 开发环境: Keil MDK-ARM
- 编码: UTF-8

### FreeRTOS 任务
| 任务 | 优先级 | 栈 | 职责 |
|------|--------|-----|------|
| DisplayTask | Normal | 2048 | UI 渲染/按键分发 |
| SensorTask | BelowNormal | 1024 | MPU6050 5ms 轮询 + 抬腕检测 |
| BatteryTask | Low | 640 | ADC 5s 电池采样 |
| GameTask | Normal | 1280 | 恐龙游戏（按需创建） |

### FreeRTOS 资源
- 按键消息队列 (8槽) | OLED/I2C 互斥锁 | 传感器/电池/游戏信号量 | OLED 休眠定时器 (10s)

### 关键 GPIO
- PB12/PB13: OLED 电源 | PB15: 手电筒 LED | TIM2 中断优先级: 5

## 功能模块
- 1_1 主界面 — 时间/日期显示
- 1_2 时间修改 — RTC 时间设置
- 1_3 菜单 — 多级菜单系统
- 1_4 游戏 — 小恐龙游戏 (dino)
- 1_5 电源 — 电池电压显示、低功耗

## 关键文件
- `Core/Src/main.c` — 主函数和主循环
- `Core/Src/item.c` — 菜单项/界面管理
- `Core/Src/menu.c` — 菜单逻辑
- `Core/Src/Key.c` — 按键驱动
- `Core/Src/i2c.c` — I2C 通信
- `Core/Src/mpu6050.c` — 姿态传感器
- `Core/Src/adc.c` — 电池 ADC
- `Core/Src/rtc.c` — RTC 时钟

## 教学资料
- 参考代码（标准库版本）: `../完整代码（有注释）/`
- 教程资料: `../stm32可编程多功能手表教程配套资料/`
- 子模块独立测试: `../1_1 Test/`, `../1_2时间修改/` 等
