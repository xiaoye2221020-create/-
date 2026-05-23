# 智能手表项目

基于 STM32F103C8T6 + FreeRTOS 的嵌入式智能手表系统。

## 硬件平台

| 项目 | 详情 |
|------|------|
| 主控芯片 | STM32F103C8T6 (Cortex-M3, 72MHz) |
| 存储 | 64KB Flash / 20KB SRAM |
| 显示屏 | 0.96 寸 OLED (I2C, 128x64) |
| 姿态传感器 | MPU6050 六轴 (加速度 + 陀螺仪) |
| 时钟 | RTC 实时时钟 (外部 32.768kHz) |
| 按键 | 3 个 (上/下/确认，确认键支持长按) |
| 其他 | LED 手电筒 + 蜂鸣器 |
| 电源检测 | ADC 电池电压检测 |
| 调试接口 | SWD (PA13/PA14) |

## 软件架构

```
FreeRTOS v10.3.1 (CMSIS-RTOS v2, 1000Hz Tick)
├── DisplayTask     (Normal, 2048B)   UI 渲染 + 按键分发
├── SensorTask      (BelowNormal, 1024B) MPU6050 5ms 轮询 + 抬腕检测
├── BatteryTask     (Low, 640B)       ADC 5s 电池采样
├── GameTask        (Normal, 1280B)   恐龙游戏 (按需创建/销毁)
├── IDLE            (0, 128B)
└── TimerTask       (2, 256B)         软件定时器
```

### FreeRTOS 资源
| 类型 | 名称 | 用途 |
|------|------|------|
| 消息队列 | keyQueue (8槽) | 按键事件 ISR→任务缓冲 |
| 互斥锁 | oledMutex | OLED 缓冲区保护 |
| 互斥锁 | mpu6050Mutex | MPU6050 I2C 保护 |
| 信号量 | sensorDataReady | 传感器数据就绪通知 |
| 信号量 | batteryDataReady | 电池数据更新通知 |
| 信号量 | gameDone | 游戏结束通知 |
| 定时器 | oledSleepTimer | OLED 10s 自动休眠 |

### 关键 GPIO
| 引脚 | 功能 |
|------|------|
| PB12 | OLED 电源控制 (高=开) |
| PB13 | OLED 电源控制 (低=开) |
| PB15 | LED 手电筒 |
| PB8/PB9 | OLED 软件 I2C |
| PB10/PB11 | MPU6050 I2C2 |

## 功能列表

| 编号 | 功能 | 说明 |
|------|------|------|
| 1.1 | 主界面 | 时间/日期/电池/秒表/设置入口 |
| 1.2 | 时间设置 | RTC 年/月/日/时/分/秒设置 |
| 1.3 | 图形菜单 | 7 项图标轮播菜单系统 |
| 1.4 | 秒表 | 开始/停止/复位，99:59:59 计时 |
| 1.5 | LED 控制 | LED 手电筒开关 |
| 1.6 | 姿态显示 | 实时显示 Roll/Pitch/Yaw 角度 |
| 1.7 | 恐龙游戏 | 横版跑酷游戏 (50FPS) |
| 1.8 | 表情动画 | OLED 表情动态效果 |
| 1.9 | 水平仪 | 气泡水平仪，MPU6050 姿态驱动 |
| 1.10 | 电源显示 | ADC 电池百分比 + 图标 |
| 1.11 | 抬腕显示 | 加速度计检测抬腕，自动点亮 OLED |
| 1.12 | OLED 休眠 | 10 秒无操作自动关闭 OLED |

## 技术要点

### 按键系统
- 3 键状态机：IDLE → DEBOUNCE → PRESSED → LONG_WAIT_RELEASE
- 消抖 60ms，长按 800ms
- TIM2 ISR (20ms 周期) 扫描 → FreeRTOS 消息队列缓冲
- 按键消费时自动点亮 OLED + 重置休眠定时器

### MPU6050 姿态解算
- 互补滤波：`Angle = 0.80 * Gyro + 0.20 * Accel`
- 开机陀螺仪零偏校准 (20 次采样)
- I2C 总线锁死自动恢复
- SensorTask 后台 5ms (200Hz) 轮询

### 抬腕检测
- 监测加速度计俯仰角 (pitch_a)
- 检测腕部从下垂 (<-30°) 到水平 (>-15°) 的快速转换
- 2 秒冷却期防误触发

### 电源管理
- BatteryTask 后台 5 秒周期采样
- 10 次 ADC 平均滤波
- OLED 10 秒无操作自动休眠
- 按键或抬腕唤醒

### OLED 显示
- 3 种字号：OLED_6X8 / OLED_8X16 / OLED_12X24
- 软件 I2C (PB8/PB9)
- OLED_ReverseArea 菜单高亮

## 项目结构

```
手表/
├── Core/
│   ├── Inc/                  # 头文件
│   │   ├── FreeRTOSConfig.h  # FreeRTOS 配置
│   │   ├── Key.h             # 按键接口
│   │   ├── menu.h            # 菜单接口
│   │   └── item.h            # 功能模块接口
│   └── Src/                  # 源文件
│       ├── main.c            # 入口 + 任务创建 + 中断回调
│       ├── menu.c            # 时钟界面 + 菜单系统
│       ├── item.c            # 秒表/LED/MPU6050/游戏/表情/水平仪 + 任务函数
│       ├── TIMESET.c         # 时间设置
│       ├── Key.c             # 按键状态机 + 队列
│       ├── mpu6050.c         # MPU6050 驱动 + I2C
│       └── stm32f1xx_it.c    # 中断服务
├── Middlewares/FreeRTOS/     # FreeRTOS v10.3.1 内核
├── Drivers/                  # STM32 HAL 库
├── oled/                     # OLED 驱动 + 字库
└── MDK-ARM/                  # Keil MDK 工程
```

## 编译说明

1. Keil MDK v5.32 + ARM Compiler V5.06
2. 打开 `MDK-ARM/1_1 Test.uvprojx`
3. 目标芯片：STM32F103C8 (64KB Flash / 20KB SRAM)
4. 编码：UTF-8
5. 通过 ST-Link / J-Link 烧录

## 版本

| 组件 | 版本 |
|------|------|
| Keil MDK-ARM | v5.32 |
| ARM Compiler | V5.06 |
| STM32CubeMX | v6.17 |
| STM32Cube FW_F1 | v1.8.7 |
| FreeRTOS | v10.3.1 |
| CMSIS-RTOS | v2 |
