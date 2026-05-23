# 智能手表 FreeRTOS

> 基于 STM32F103C8T6 + FreeRTOS v10.3.1 的完整嵌入式项目解析

---

## 目录

1. [整体架构](#1-整体架构)
2. [FreeRTOS 内核配置](#2-freertos-内核配置)
3. [任务管理](#3-任务管理)
4. [消息队列](#4-消息队列)
5. [互斥锁](#5-互斥锁)
6. [信号量](#6-信号量)
7. [软件定时器](#7-软件定时器)
8. [中断与 FreeRTOS](#8-中断与-freertos)
9. [低功耗模式](#9-低功耗模式)
10. [原始代码 vs FreeRTOS 改造对比](#10-原始代码-vs-freertos-改造对比)

---

## 1. 整体架构

### 1.1 系统启动流程

```
main()
  ├── HAL_Init()                    // HAL 库初始化
  ├── SystemClock_Config()          // 72MHz 时钟配置
  ├── MX_GPIO/I2C/RTC/TIM2/ADC_Init()  // 外设初始化
  ├── Key_Init()                    // 按键状态机初始化
  ├── HAL_TIM_Base_Start_IT(&htim2) // 启动 TIM2 中断 (20ms)
  ├── keyQueueHandle = osMessageQueueNew(8, 1, NULL)  // 创建按键队列
  ├── osKernelInitialize()          // 初始化 FreeRTOS 内核
  ├── MX_FREERTOS_Init()            // 创建所有 RTOS 资源
  │     ├── osMutexNew × 2          // OLED 互斥锁, MPU6050 互斥锁
  │     ├── osSemaphoreNew × 3      // 传感器/电池/游戏信号量
  │     ├── osTimerNew × 1          // OLED 休眠定时器
  │     └── osThreadNew × 3         // DisplayTask + SensorTask + BatteryTask
  └── osKernelStart()               // 启动调度器 (永不返回)
```

**关键理解**：`osKernelStart()` 之后，CPU 控制权交给 FreeRTOS 调度器。main() 中 `while(1)` 永远不会执行。

### 1.2 任务拓扑图

```
                     FreeRTOS Scheduler (1000Hz)
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
   DisplayTask           SensorTask            BatteryTask
   (Normal, 2048B)       (BelowNormal, 1024B)  (Low, 640B)
        │                     │                     │
        ├─ 时钟界面           ├─ MPU6050 5ms 轮询    ├─ ADC 5s 采样
        ├─ 菜单系统           ├─ 抬腕检测           ├─ 电池百分比计算
        ├─ 秒表/LED/游戏      ├─ 姿态解算           └─ 信号量通知
        ├─ 表情/水平仪        └─ 信号量通知
        └─ 时间设置
                              │
                        GameTask (按需)
                        (Normal, 1280B)
                              │
                        └─ 恐龙游戏 50FPS
```

**优先级设计原理**：
- `DisplayTask`(Normal=24)：UI 交互需要最流畅的响应
- `SensorTask`(BelowNormal=16)：后台数据采集，不能抢占 UI
- `BatteryTask`(Low=8)：最低优先级，只在 CPU 空闲时运行
- `GameTask`(Normal=24)：与 DisplayTask 同优先级，互斥运行（信号量同步）

### 1.3 数据流图

```
TIM2 ISR (20ms)                    SensorTask (5ms)
    │                                    │
    ├─ Key_Scan() ──→ 消息队列 ──→ DisplayTask
    ├─ StopWatch_Tick() ──→ menu_watch (全局)
    └─ DIno_Tick() ──→ 游戏状态 (全局)
                                        │
                                   MPU6050 ──→ Rool, Pitch, Yaw (全局)
                                        │         │
                                  信号量释放    DisplayTask 读取
                                        │         │
                                  抬腕检测      Menu_MPU6050
                                        │         Menu_Gradienter
                                  唤醒 OLED
```

---

## 2. FreeRTOS 内核配置

文件位置：`Core/Inc/FreeRTOSConfig.h`

### 2.1 核心参数

```c
#define configUSE_PREEMPTION       1   // 抢占式调度
#define configTICK_RATE_HZ         1000 // Tick 频率 1000Hz → 1ms 时间片
#define configMAX_PRIORITIES       56  // 最大优先级数
#define configTOTAL_HEAP_SIZE      6144 // 堆大小 6KB (heap_4)
#define configMINIMAL_STACK_SIZE   128  // 最小任务栈 (IDLE 任务用)
```

**参数理解**：
- `configUSE_PREEMPTION = 1`：高优先级任务就绪时立即抢占低优先级任务。若设为 0，则为协作式调度（任务主动让出 CPU）。
- `configTICK_RATE_HZ = 1000`：系统心跳 1ms 一次。`osDelay(10)` = 阻塞 10ms。频率越高，时间分辨率越精细，但中断开销越大。
- `configTOTAL_HEAP_SIZE = 6144`：FreeRTOS 动态内存池。每个任务栈、队列、信号量、互斥锁、定时器都从这里分配。20KB SRAM 中分 6KB 给 RTOS 堆。

### 2.2 功能开关

```c
#define configUSE_MUTEXES               1  // 启用互斥锁
#define configUSE_RECURSIVE_MUTEXES     1  // 启用递归互斥锁
#define configUSE_COUNTING_SEMAPHORES   1  // 启用计数信号量
#define configUSE_IDLE_HOOK             1  // 启用空闲钩子 (用于低功耗)
#define configUSE_TIMERS                1  // 启用软件定时器
```

**资源消耗**：每启用一个功能，FreeRTOS 内核代码增大。互斥锁 + 信号量 + 定时器总共增加约 2KB 代码。

### 2.3 中断优先级映射

```c
// FreeRTOSConfig.h 中隐含 (通过 CMSIS 设置)
// configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5
// FreeRTOS 可管理的中断优先级: 5 ~ 15
// TIM2 中断优先级: 5 (允许调用 FromISR API)
// SysTick 优先级: 最低 (FreeRTOS 内部使用)
```

**关键规则**：中断优先级**数值越大优先级越低**（Cortex-M NVIC）。优先级 0~4 的中断**不能**调用 FreeRTOS API（如 `xQueueSendFromISR`），否则会破坏内核数据结构。

---

## 3. 任务管理

### 3.1 任务创建 (CMSIS-RTOS v2 API)

```c
// main.c — 任务属性定义
const osThreadAttr_t DisplayTask_attributes = {
    .name       = "DisplayTask",      // 调试用名称
    .stack_size = 512 * 4,           // 栈大小 = 2048 字节
    .priority   = osPriorityNormal,  // 优先级 (24/56)
};

// 创建任务
osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);
```

**栈大小如何确定**：
- Stack High Water Mark 检测：`uxTaskGetStackHighWaterMark(NULL)` 返回剩余栈空间
- DisplayTask 调用深度最大（OLED → I2C → HAL），需 2048 字节
- SensorTask 仅调用 MPU6050_Calculation，1024 字节足够
- BatteryTask 仅有 ADC 读取 + 简单运算，640 字节

### 3.2 任务函数结构

**典型任务模式**：
```c
void StartSensorTask(void *argument)
{
    osDelay(500);          // 1. 初始化等待 (等待硬件就绪)

    while (1)              // 2. 无限循环
    {
        // 3. 获取资源 (互斥锁)
        osMutexAcquire(mpu6050MutexHandle, osWaitForever);

        // 4. 执行任务逻辑
        MPU6050_Calculation();

        // 5. 释放资源
        osMutexRelease(mpu6050MutexHandle);

        // 6. 通知消费者
        osSemaphoreRelease(sensorDataReadySem);

        // 7. 周期等待 (让出 CPU)
        osDelay(5);        // 200Hz 采样率
    }
}
```

**关键理解**：
- `osDelay(5)` **不是空转等待**，而是让任务进入 Blocked 状态 5ms。期间 CPU 可执行其他任务。
- `osWaitForever` 表示如果资源不可用则永久阻塞，直到资源可用。

### 3.3 任务状态转换

```
     创建 → Ready → Running → Blocked (osDelay/队列等待/信号量等待)
                ↑        │
                │        → Suspended (被挂起)
                └────────┘
                     │
                  Deleted (被删除)
```

**本项目的实际调度时序**：
```
时间轴 (ms):  0    1    2    3    4    5    6    7    8    9   10
DisplayTask:  [====UI 渲染====] [Blocked: 队列等待按键] [==UI==]
SensorTask:   [Blocked: osDelay(5)] [读MPU] [Blocked: osDelay(5)] [读]
BatteryTask:  [Blocked: osDelay(5000)............................]
IDLE:         [  WFI 睡眠  ] [WFI]    [WFI] [    WFI 睡眠    ]
```

### 3.4 GameTask 动态创建/销毁

```c
// 进入游戏时创建
GameTaskHandle = osThreadNew(StartGameTask, NULL, &GameTask_attributes);
osSemaphoreAcquire(gameDoneSem, osWaitForever);  // 等待游戏结束

// 游戏结束后自销毁
void StartGameTask(void *argument)
{
    DinoGame_Animation();           // 游戏主循环 (阻塞)
    osSemaphoreRelease(gameDoneSem); // 通知 DisplayTask 游戏结束
    osThreadTerminate(osThreadGetId()); // 销毁自己
}
```

**为什么按需创建**：GameTask 需要 1280 字节栈，20KB SRAM 紧张。不玩游戏时不占用内存。

---

## 4. 消息队列

### 4.1 为什么需要队列

**原始代码的问题**：
```c
// 原始 — ISR 直接写全局变量
void Key_Scan(void) {
    Key_Num = key_value;        // ISR 写
    if (key_value != KEY_NONE) return; // 按键未消费就丢弃新按键！
}
uint8_t Key_GetNum(void) {
    uint8_t temp = Key_Num;     // 任务读
    Key_Num = KEY_NONE;         // 非原子操作，有竞争风险
    return temp;
}
```

**问题**：
1. **按键丢失**：应用忙碌时（如表情动画阻塞 1.1 秒），新按键被丢弃
2. **竞态条件**：ISR 和任务同时访问 `Key_Num`，没有保护
3. **不可扩展**：多任务无法共享按键输入

### 4.2 FreeRTOS 队列方案

```c
// main.c — 创建队列 (8 槽, 每槽 1 字节)
osMessageQueueId_t keyQueueHandle;
keyQueueHandle = osMessageQueueNew(8, 1, NULL);
```

```c
// Key.c — ISR 中推入队列
void Key_Scan(void)  // 在 TIM2 ISR 中调用
{
    for (int i = 0; i < 3; i++) {
        Key_Process(&keys[i]);
        if (key_value != KEY_NONE) {
            osMessageQueuePut(keyQueueHandle, &key_value, 0, 0); // 0,0=非阻塞,ISR安全
            key_value = KEY_NONE;
            break;
        }
    }
}

// Key.c — 任务中读取队列
uint8_t Key_GetNum(void)
{
    uint8_t key = KEY_NONE;
    osMessageQueueGet(keyQueueHandle, &key, NULL, 0);  // 非阻塞轮询
    return key;
}
```

**队列如何工作**：
```
ISR (生产者)          队列 [8槽]          任务 (消费者)
   │                  ┌───┐                  │
   ├─ KEY_1 ────────→│ 1 │                  │
   │                  │ 2 │                  │
   ├─ KEY_2 ────────→│ 3 │                  │
   │                  │   │←── Key_GetNum() ─┤
   ├─ KEY_3 ────────→│   │                  │
   │                  └───┘                  │
   └─ 队列满时丢弃      FIFO 先进先出         └─ 空队列返回 KEY_NONE
```

**优势**：
1. **不丢按键**：队列可缓存 8 个按键事件
2. **线程安全**：FreeRTOS 内部保证队列操作的原子性
3. **解耦**：ISR 只负责生产，任务只负责消费

---

## 5. 互斥锁

### 5.1 为什么需要互斥锁

当多个任务可能访问同一个硬件资源时：

```
DisplayTask                    SensorTask
    │                              │
    ├─ OLED_Clear()                │
    ├─ OLED_ShowString()           ├─ MPU6050_GetData()
    ├─ OLED_Update() ← 写 I2C      ├─ I2C 读 MPU6050
    │                              │
    └── 如果同时操作 OLED 缓冲区 ──┘ → 屏幕撕裂/数据错乱
```

### 5.2 本项目互斥锁用法

```c
// main.c — 创建互斥锁
oledMutexHandle = osMutexNew(NULL);
mpu6050MutexHandle = osMutexNew(NULL);
```

```c
// menu.c — 保护 OLED 渲染序列
void Menu_Animation(void)
{
    osMutexAcquire(oledMutexHandle, osWaitForever); // 获取锁
    OLED_Clear();
    // ... 绘制所有图像 ...
    OLED_Update();
    osMutexRelease(oledMutexHandle);                // 释放锁
}
```

```c
// item.c — SensorTask 保护 MPU6050 I2C 传输
void StartSensorTask(void *argument)
{
    while (1) {
        osMutexAcquire(mpu6050MutexHandle, osWaitForever); // 锁定 I2C
        MPU6050_Calculation();  // 连续 6 次 I2C 读取
        osMutexRelease(mpu6050MutexHandle);                // 解锁
        osDelay(5);
    }
}
```

**互斥锁与优先级反转**：
- FreeRTOS 互斥锁支持**优先级继承**：如果低优先级任务持有锁，高优先级任务等待时，低优先级任务临时提升到高优先级，防止中优先级任务无限执行。
- 本项目未遇到此问题（持有锁的时间极短，仅几次 I2C 寄存器写入）。

---

## 6. 信号量

### 6.1 信号量类型对比

| 类型 | 用途 | 本项目使用 |
|------|------|-----------|
| 二进制信号量 | 事件通知 (0/1) | 传感器数据就绪、电池更新、游戏结束 |
| 计数信号量 | 资源计数 (0~N) | 未使用 |
| 互斥锁 | 资源互斥访问 | OLED、I2C |

### 6.2 传感器信号量 — 事件驱动更新

```c
// 创建 (最大计数=1, 初始=0)
sensorDataReadySem = osSemaphoreNew(1, 0, NULL);

// 生产者: SensorTask 每 5ms 释放一次
osSemaphoreRelease(sensorDataReadySem);  // 计数 +1 → 变为 1

// 消费者: Menu_MPU6050 等待新数据
osSemaphoreAcquire(sensorDataReadySem, osWaitForever);  // 计数 -1 → 变为 0
// 读取 Rool, Pitch, Yaw (由 SensorTask 更新)
OLED_Printf(0, 16, OLED_8X16, 0, "Rool: %.2f", Rool);
```

**为什么用信号量而不是直接读全局变量**：
- 信号量保证数据是**最新一次**采样结果，不会读到 SensorTask 写到一半的数据
- 事件驱动：Menu_MPU6050 在数据更新后才刷新显示，不会做无用渲染

### 6.3 游戏结束信号量 — 任务同步

```c
// Menu_Game 创建游戏任务后阻塞等待
GameTaskHandle = osThreadNew(StartGameTask, NULL, &GameTask_attributes);
osSemaphoreAcquire(gameDoneSem, osWaitForever);  // 阻塞直到游戏结束

// GameTask 结束时释放信号量
osSemaphoreRelease(gameDoneSem);  // 唤醒 DisplayTask
osThreadTerminate(osThreadGetId());
```

**时序图**：
```
DisplayTask                    GameTask
    │                              │
    ├─ 创建 GameTask ─────────────→│
    ├─ 等待 gameDoneSem (阻塞)     ├─ 游戏循环 (50FPS)
    │                              ├─ 碰撞检测: Game Over
    │                              ├─ 释放 gameDoneSem ─┐
    ├← 被唤醒 ─────────────────────────────────────────┘
    ├─ 继续菜单显示
```

---

## 7. 软件定时器

### 7.1 OLED 休眠定时器

```c
// main.c — 创建单次定时器
oledSleepTimerHandle = osTimerNew(OledSleepTimerCallback, osTimerOnce, NULL, NULL);

// 回调函数 (在 TimerTask 上下文中执行)
void OledSleepTimerCallback(void *argument)
{
    OLED_WriteCommand(0xAE);  // SSD1306 休眠命令 — 像素全灭
    oled_asleep = 1;          // 标记进入休眠
}

// 启动/重置定时器
osTimerStop(oledSleepTimerHandle);        // 停止旧定时
osTimerStart(oledSleepTimerHandle, 10000); // 10 秒后触发
```

**何时重置定时器**：
- 任何按键按下 → `Key_GetNum()` 中重置
- 抬腕检测触发 → `CheckRaiseToWake()` 中重置
- 系统启动 → `StartDisplayTask()` 中首次启动

### 7.2 定时器类型

```c
osTimerOnce     // 单次触发 (本项目使用)
osTimerPeriodic // 周期触发 (可用于周期性采集)
```

**重要**：定时器回调在 **TimerTask** 上下文执行（优先级 2），不是中断上下文。可以在回调中调用大部分 FreeRTOS API。

---

## 8. 中断与 FreeRTOS

### 8.1 SysTick — HAL 与 FreeRTOS 共享

```c
// stm32f1xx_it.c
void SysTick_Handler(void)
{
    HAL_IncTick();              // HAL 库延时计数器
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();  // FreeRTOS Tick 递增
    }
}
```

**关键设计**：同一个 SysTick 同时服务 HAL 库和 FreeRTOS。`xPortSysTickHandler()` 内部会触发任务调度。

### 8.2 TIM2 ISR — 20ms 周期中断

```c
// main.c — TIM2 中断回调
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        StopWatch_Tick();   // 秒表计时
        Key_Scan();         // 按键扫描 → 推入队列
        DIno_Tick();        // 恐龙游戏 Tick
    }
}
```

**为什么 TIM2 优先级设为 5**：
- 原始代码：`HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0)` → 优先级 0（最高）
- FreeRTOS 要求：中断优先级 ≥ 5 才能调用 `osMessageQueuePut` 等 FromISR API
- 修改后：`HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0)` → 满足要求

### 8.3 FreeRTOS 中断安全 API 调用规则

| 上下文 | 可调用 API | 超时参数 |
|--------|-----------|---------|
| 任务上下文 | `osMessageQueuePut/Get`, `osMutexAcquire/Release`, `osDelay` 等全部 | 可阻塞 |
| ISR 上下文 | **仅** `osMessageQueuePut`, `osSemaphoreRelease` 等 FromISR 函数 | **必须** 0 |

```c
// ISR 中正确用法
osMessageQueuePut(keyQueueHandle, &key_value, 0, 0);  // 超时=0, 非阻塞

// ISR 中错误用法 (会导致崩溃)
osDelay(10);            // ❌ ISR 中不能阻塞!
osMutexAcquire(...);    // ❌ ISR 中不能用互斥锁!
```

---

## 9. 低功耗模式

### 9.1 休眠架构

```
正常模式                    休眠模式
┌──────────────┐          ┌──────────────────┐
│ OLED ON      │  10s     │ OLED OFF (0xAE)  │
│ CPU 全速运行  │ ──────→ │ CPU Sleep (WFI)  │
│ 所有任务运行  │          │ TIM2 继续 (20ms) │
│              │  ←────── │ I2C2 继续        │
└──────────────┘  KEY3/   │ DisplayTask 跳过 │
                   抬腕    └──────────────────┘
```

### 9.2 实现细节

```c
// main.c — 空闲钩子 (FreeRTOS 在无任务可运行时调用)
void vApplicationIdleHook(void)
{
    if (oled_asleep)
    {
        __WFI();  // Wait For Interrupt — CPU 进入 Sleep 模式
                  // 任意中断 (SysTick, TIM2) 都会唤醒 CPU
    }
}

// DisplayTask — 休眠时跳过渲染
while (1)
{
    if (oled_asleep) {
        osDelay(100);   // 休眠期间只做 100ms 轮询
        continue;       // 跳过 UI 渲染
    }
    // ... 正常 UI 逻辑 ...
}
```

### 9.3 唤醒源

| 唤醒源 | 实现 | 恢复动作 |
|--------|------|---------|
| KEY3 按键 | `Key_GetNum()` 中检测 → `oled_asleep=0` → `OLED_WriteCommand(0xAF)` | OLED 点亮 + 重置定时器 |
| 抬腕检测 | `CheckRaiseToWake()` 中检测 → 同上 | 同上 |

**为什么 KEY3 唤醒后不传递按键事件**：
```c
if (oled_asleep) {
    if (key != KEY_3 && key != KEY_3_LONG) return KEY_NONE;
    oled_asleep = 0;
    OLED_WriteCommand(0xAF);
    // ... 重置定时器 ...
    return KEY_NONE;  // ← 吞掉唤醒按键，不传给应用层
}
```

---

## 10. 原始代码 vs FreeRTOS 改造对比

### 10.1 架构对比

| 维度 | 原始代码 (裸机) | FreeRTOS 改造后 |
|------|----------------|----------------|
| **程序结构** | 超级循环 + 嵌套 while(1) | 多任务并行 + 事件驱动 |
| **CPU 利用** | 忙等待/空转 (`for i<3000` 等) | 阻塞等待, CPU 空闲时自动睡眠 |
| **按键处理** | 全局变量, 未消费就丢弃 | 消息队列 8 槽缓冲 |
| **传感器** | UI 循环中同步调用, 重复初始化 | 独立任务 200Hz 轮询, 只初始化一次 |
| **电池检测** | 3000 次 busy-wait ADC 循环 | BatteryTask 后台 5s 采样 |
| **游戏** | 100% CPU 无延时循环 | 50FPS (osDelay(20)) |
| **资源共享** | 无保护 | 互斥锁 + 信号量 |
| **低功耗** | 无 | OLED 自动休眠 + CPU Sleep |
| **并发** | 不可能 | 4 任务并行 |

### 10.2 具体改进案例

#### 案例 1：电池检测

```c
// 原始 — busy-wait, 阻塞整个 UI
void Shwo_Battery(void) {
    int sum;
    for (int i = 0; i < 3000; i++) {  // 3000 次循环！
        HAL_ADC_Start(&hadc1);
        ADValue = HAL_ADC_GetValue(&hadc1);
        sum += ADValue;                // 约阻塞 30ms+
    }
    // ... 计算和显示 ...
}

// 改造后 — 后台任务 + 直接读取
void StartBatteryTask(void *argument) {
    while (1) {
        // 10 次采样 (非 busy-wait, 有 osDelay)
        for (int i = 0; i < 10; i++) {
            HAL_ADC_Start(&hadc1);
            sum += HAL_ADC_GetValue(&hadc1);
            osDelay(1);               // 让出 CPU 给其他任务
        }
        Battery_Capacity = ...;       // 更新全局变量
        osDelay(5000);               // 5 秒采样一次
    }
}

void Shwo_Battery(void) {
    // 直接读取已计算好的值, 无阻塞
    OLED_ShowNum(85, 4, Battery_Capacity, 3, OLED_6X8);
}
```

#### 案例 2：按键不丢失

```
场景：用户在看表情动画 (阻塞 1.1 秒)

原始代码:  按键1  [丢失!]     按键2  [丢失!]     按键3  [可能丢失]
           ─────────────────────────────────────────────────────→ 时间
                                              ↑ 动画结束, Key_Num=KEY_NONE

改造后:    按键1  [入队]      按键2  [入队]      按键3  [入队]
           ─────────────────────────────────────────────────────→ 时间
                                              ↑ 动画结束, 逐个处理 KEY_1, KEY_2, KEY_3
```

#### 案例 3：MPU6050 重复初始化

```c
// 原始 — 每次计算都重新初始化 (浪费 10ms)
void MPU6050_Calculation(void) {
    Init_Attitude();  // 每次都执行！5 次采样 × osDelay(2) = 10ms
    // ... 姿态解算 ...
}

// 改造后 — 只初始化一次
static uint8_t attitude_initialized = 0;
void MPU6050_Calculation(void) {
    if (!attitude_initialized) Init_Attitude(); // 仅首次执行
    // ... 姿态解算 ...
}
```

### 10.3 FreeRTOS 引入的额外开销

| 开销项 | 大小 | 说明 |
|--------|------|------|
| FreeRTOS 内核代码 | ~6KB Flash | tasks.c, queue.c, timers.c, heap_4.c 等 |
| 内核数据结构 | ~2KB SRAM | TCB、队列控制块、就绪列表等 |
| 任务栈 | ~5KB SRAM | DisplayTask(2K) + SensorTask(1K) + BatteryTask(0.6K) + GameTask(1.3K) |
| 同步对象 | ~0.5KB SRAM | 队列、互斥锁、信号量、定时器控制块 |

**总开销**：~6KB Flash + ~7.5KB SRAM。STM32F103C8T6 有 64KB Flash / 20KB SRAM，完全可承受。

### 10.4 何时不应该用 RTOS

- **极简应用**：如果只有 1 个传感器 + 1 个 LED 闪烁，裸机更合适
- **SRAM 极度紧张**：<2KB SRAM 时 RTOS 开销占比太高
- **硬实时要求**：需要精确到微秒级的定时（此时应裸机 + 定时器中断）

---

## 附录 A：FreeRTOS API 速查表

### CMSIS-RTOS v2 API (本项目使用的)

| API | 功能 | 使用位置 |
|-----|------|---------|
| `osKernelInitialize()` | 初始化内核 | main.c |
| `osKernelStart()` | 启动调度器 | main.c |
| `osThreadNew(func, arg, attr)` | 创建任务 | main.c |
| `osThreadTerminate(id)` | 终止任务 | item.c: GameTask |
| `osDelay(ms)` | 阻塞延时 (ms) | 各文件 |
| `osMessageQueueNew(n, size, attr)` | 创建消息队列 | main.c |
| `osMessageQueuePut(q, data, prio, timeout)` | 发送消息 | Key.c (ISR) |
| `osMessageQueueGet(q, data, prio, timeout)` | 接收消息 | Key.c (Task) |
| `osMutexNew(attr)` | 创建互斥锁 | main.c |
| `osMutexAcquire(m, timeout)` | 获取互斥锁 | menu.c, item.c, mpu6050.c |
| `osMutexRelease(m)` | 释放互斥锁 | 同上 |
| `osSemaphoreNew(max, init, attr)` | 创建信号量 | main.c |
| `osSemaphoreAcquire(s, timeout)` | 获取信号量 | item.c |
| `osSemaphoreRelease(s)` | 释放信号量 | item.c |
| `osTimerNew(cb, type, arg, attr)` | 创建定时器 | main.c |
| `osTimerStart(t, ms)` | 启动定时器 | Key.c, item.c |
| `osTimerStop(t)` | 停止定时器 | Key.c, item.c |

### 超时值说明

| 值 | 含义 | 使用场景 |
|----|------|---------|
| `0` | 非阻塞，立即返回 | ISR 中发送队列、任务中轮询 |
| `osWaitForever` | 无限等待 | 等待信号量、获取互斥锁 |
| `N (>0)` | 等待 N 个 Tick | 带超时的操作 |

---

## 附录 B：关键文件速查

| 文件 | 内容 | FreeRTOS 相关 |
|------|------|--------------|
| `Core/Src/main.c` | 入口 + 任务创建 + 中断回调 | 全部 RTOS 资源初始化 |
| `Core/Src/Key.c` | 按键状态机 + 队列操作 | 消息队列、OLED 休眠定时器 |
| `Core/Src/menu.c` | 时钟界面 + 菜单系统 | OLED 互斥锁 |
| `Core/Src/item.c` | 全部功能模块 + 3 个任务函数 | 任务函数、信号量、互斥锁 |
| `Core/Src/TIMESET.c` | 时间设置界面 | OLED 互斥锁 |
| `Core/Src/mpu6050.c` | MPU6050 驱动 + I2C | MPU6050 互斥锁 |
| `Core/Src/stm32f1xx_it.c` | 中断服务 | SysTick、TIM2 |
| `Core/Inc/FreeRTOSConfig.h` | FreeRTOS 配置 | 全部内核参数 |
