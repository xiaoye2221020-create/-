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
- 固件库: STM32 HAL 库
- 开发环境: Keil MDK-ARM (Project.uvprojx)
- 代码生成: STM32CubeMX 6.17 (.ioc 文件)
- 无 RTOS，裸机开发 (超级循环 + 定时器中断)

## 项目结构
```
手表/
├── Core/
│   ├── Inc/      # 头文件
│   └── Src/      # 源文件 (main.c, gpio.c, i2c.c, Key.c, item.c, menu.c 等)
├── Drivers/      # HAL 库驱动
├── MDK-ARM/      # Keil 工程文件
├── oled/         # OLED 驱动
└── 1_1 Test.ioc  # CubeMX 配置文件
```

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
