#include "main.h"
#include "OLED.h"
#include "Key.h"
#include "item.h"
#include "mpu6050.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdlib.h>

/*********************************MPU6050*************************/ 
int16_t AX, AY, AZ, GX, GY, GZ;
float rool_g, pitch_g, yaw_g;
float rool_a, pitch_a;
float Rool, Pitch, Yaw;
float a = 0.80f;  
float Delta_t = 0.005f; // 5ms
double pi = 3.14159265359;




//static uint8_t attitude_initialized = 0;
static float constrain_angle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}


static void Init_Attitude(void)
{
    HAL_StatusTypeDef status;
    int16_t ax, ay, az, gx, gy, gz;

    float sum_pitch = 0, sum_roll = 0;
    uint8_t valid_count = 0;

    for (int i = 0; i < 5; i++)
    {
        status = MPU6050_GetData(&ax, &gx, &ay, &gy, &az, &gz);
        if (status == HAL_OK)
        {
            sum_pitch += atan2f(-ax, az) * 180.0f / pi;
            sum_roll += atan2f(ay, az) * 180.0f / pi;
            valid_count++;
        }
        osDelay(2);
    }

    if (valid_count > 0)
    {
        Pitch = sum_pitch / valid_count;
        Rool = sum_roll / valid_count;
    }
    else
    {
        Pitch = 0.0f;
        Rool = 0.0f;
    }
    Yaw = 0.0f;
//    attitude_initialized = 1;
}

void MPU6050_Calculation(void)
{
    HAL_StatusTypeDef status;
    uint8_t read_fail_count = 0;
    Init_Attitude();
   

        status = MPU6050_GetData(&AX, &GX, &AY, &GY, &AZ, &GZ);

        if (status == HAL_OK)
        {
            read_fail_count = 0;
            float gyro_x_dps = (float)GX / 16.4f;
            float gyro_y_dps = (float)GY / 16.4f;
            float gyro_z_dps = (float)GZ / 16.4f;

            rool_g = Rool + gyro_x_dps * Delta_t;
            pitch_g = Pitch + gyro_y_dps * Delta_t;
            yaw_g = Yaw + gyro_z_dps * Delta_t;

            pitch_a = atan2f(-(float)AX, (float)AZ) * 180.0f / pi;
            rool_a = atan2f((float)AY, (float)AZ) * 180.0f / pi;

            Rool = a * rool_g + (1.0f - a) * rool_a;
            Pitch = a * pitch_g + (1.0f - a) * pitch_a;
            Yaw = yaw_g;

            Rool = constrain_angle(Rool);
            Pitch = constrain_angle(Pitch);
            Yaw = constrain_angle(Yaw);
        }
        else
        {
            read_fail_count++;
            if (read_fail_count >= 5)
            {
                MPU6050_Init();
                read_fail_count = 0;
            }
        }

       
    }


void Menu_MPU6050(void)
{
     while (1)
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 3)
        {
            OLED_Clear();
            OLED_Update();
//            attitude_initialized = 0;
            return;
        }
        MPU6050_Calculation();
        OLED_Clear();
        OLED_ShowImage(0, 0, 16, 16, Return);
        OLED_ReverseArea(0, 0, 16, 16);
        OLED_Printf(0, 16, OLED_8X16, 0, "Rool: %.2f", Rool);
        OLED_Printf(0, 32, OLED_8X16, 0, "Pitch:%.2f", Pitch);
        OLED_Printf(0, 48, OLED_8X16, 0, "Yaw:  %.2f", Yaw);
        OLED_Update();
        osDelay(5); // 5ms
    }
}

/*****************************?????****************************/
MenuWatch menu_watch = {0, 0, 0, 0};

void Show_WatchUI(void)
{
    OLED_Clear();
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_Printf(32, 20, OLED_8X16, 0, "%02d:%02d:%02d", menu_watch.hour, menu_watch.min, menu_watch.sec);
    OLED_ShowString(8, 44, "Begin", OLED_8X16);
    OLED_ShowString(48, 44, "Stop", OLED_8X16);
    OLED_ShowString(88, 44, "Reset", OLED_8X16);
}

void StopWatch_Tick(void)
{
    static uint16_t Count;
    Count++;
    if (Count >= 100)  // 100 * 10ms = 1000ms = 1
    {
        Count = 0;
        if (menu_watch.start_time_flage == 1)
        {
            menu_watch.sec++;
            if (menu_watch.sec >= 60)
            {
                menu_watch.sec = 0;
                menu_watch.min++;
                if (menu_watch.min >= 60)
                {
                    menu_watch.min = 0;
                    menu_watch.hour++;
                    if (menu_watch.hour >= 100)
                    {
                        menu_watch.hour = 0;
                    }
                }
            }
        }
    }
}

uint8_t stopwatch_flag = 1;

void Menu_Timer(void)
{
    Show_WatchUI();
    OLED_Update();
    while (1)
    {
        uint8_t stopwatch_flag_temp = 0;
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            stopwatch_flag--;
            if (stopwatch_flag <= 0) stopwatch_flag = 4;
        }
        else if (Key_Num == 2)
        {
            stopwatch_flag++;
            if (stopwatch_flag >= 5) stopwatch_flag = 1;
        }
        else if (Key_Num == 3)
        {
            OLED_Clear();
            OLED_Update();
            stopwatch_flag_temp = stopwatch_flag;
        }

        if (stopwatch_flag_temp == 1) { return; }
        else if (stopwatch_flag_temp == 2) { menu_watch.start_time_flage = 1; }
        else if (stopwatch_flag_temp == 3) { menu_watch.start_time_flage = 0; }
        else if (stopwatch_flag_temp == 4) { menu_watch.start_time_flage = 0; menu_watch.hour = 0; menu_watch.min = 0; menu_watch.sec = 0; }

        switch (stopwatch_flag)
        {
        case 1:
            Show_WatchUI();
            OLED_ReverseArea(0, 0, 16, 16);
            OLED_Update();
            break;

        case 2:
            Show_WatchUI();
            OLED_ReverseArea(8, 44, 32, 16);
            OLED_Update();
            break;

        case 3:
            Show_WatchUI();
            OLED_ReverseArea(48, 44, 32, 16);
            OLED_Update();
            break;

        case 4:
            Show_WatchUI();
            OLED_ReverseArea(88, 44, 32, 16);
            OLED_Update();
            break;
        }
    }
}

/*****************************Game****************************/
struct Object_Position
{
    uint8_t minX, maxX;
    uint8_t minY, maxY;
};

int Score;//????
void Show_Score(void)
{
    OLED_ShowNum(92, 0, Score, 5,OLED_6X8);
}
uint16_t Ground_Pos;
void Show_Ground(void)
{
    if (Ground_Pos<128)
    {
        for (uint8_t i = 0; i < 128; i++)
        {
            OLED_DisplayBuf[7][i]=Ground[Ground_Pos+i];
        }
    }
    else
    {
        for (uint8_t i = 0; i < 255-Ground_Pos; i++)
        {
            OLED_DisplayBuf[7][i]=Ground[Ground_Pos-128+i];
        } 
    
        for (uint8_t i = 255-Ground_Pos; i < 128; i++)
        {
            OLED_DisplayBuf[7][i]=Ground[i-(255-Ground_Pos)];
        }
    }
}


uint8_t Barrier_flag;
uint8_t Barrier_pos;

struct Object_Position barrieer;



void Show_Barrier(void)
{
	if(Barrier_pos>=143){
    Barrier_flag=rand()%3;}
    OLED_ShowImage(127-Barrier_pos, 44, 16, 18, Barrier[Barrier_flag]);
    barrieer.minX=127-Barrier_pos;
    barrieer.maxX=127-Barrier_pos+16;
    barrieer.minY=44;
    barrieer.maxY=44+18;

}

uint8_t Dino_jump_flag=0;
uint16_t Dino_jump_time;
uint8_t Dino_jump_pos;
extern double pi;
struct Object_Position dino;

void Show_Dino(void)
{
    if (Key_Num==1)Dino_jump_flag=1;
    Dino_jump_pos= 28*sin((float)(pi*Dino_jump_time/100));
    if (Dino_jump_flag==0)
    {
        if (Score%2==0)OLED_ShowImage(0,44,16,18,Dino[0]);
        else OLED_ShowImage(0,44,16,18,Dino[1]);
    }
    else
    {
        OLED_ShowImage(0,44-Dino_jump_pos,16,18,Dino[2]);
    }
    dino.minX=0;
    dino.maxX=16;
    dino.minY=44-Dino_jump_pos;
    dino.maxY=44-Dino_jump_pos+18;
}

int isColliding(struct Object_Position *a, struct Object_Position *b)
{
    if ((a->maxX > b->minX) && (a->minX < b->maxX) && (a->maxY > b->minY) &&(a->minY < b->maxY))
    {
        OLED_Clear();
        OLED_ShowString(28, 24, "Game Over!", OLED_8X16);
        OLED_Update();
        osDelay(1000);
        OLED_Clear();
        OLED_Update();
        return 1;    
    }
    return 0;
}


int DinoGame_Animation(void)
{
    while (1)
    {
        int return_flag;
        OLED_Clear();
        Show_Score();
        Show_Ground();
        Show_Barrier();
        Show_Dino();
        OLED_Update();
        return_flag=isColliding(&barrieer, &dino);
        if (return_flag==1)
        {
            return 0;
        }
    }
    
    
}

void DIno_Tick(void)
{
    static uint8_t Count;
    Count++;
    Ground_Pos++;
    Barrier_pos++;
    if (Count >= 10)  // 10 * 10ms = 100ms
    {
        Count = 0;
        Score++;
    }
    if (Ground_Pos>=256)Ground_Pos=0;
    if (Barrier_pos>=144)Barrier_pos=0;
    if (Dino_jump_flag==1)
    {
        Dino_jump_time++;
        if (Dino_jump_time>=100)        
        {
            Dino_jump_time=0;
            Dino_jump_flag=0;
        }
    }
    
}



uint8_t game_flag=1;
void Game_UI(void)
{
    OLED_ShowImage(0,0,16,16,Return);
    OLED_ShowString(0,16,"Google Dino",OLED_8X16);
}



void DinoGame_Init(void)
{
    Score=0;
    Ground_Pos=0;
    Barrier_flag=0;
    Barrier_pos=0;
    Dino_jump_flag=0;
    Dino_jump_time=0;
}

void Menu_Game(void)
{
    while (1)
    {
        uint8_t game_flag_temp=0;
        Key_Num=Key_GetNum();
        if (Key_Num==1)
        {
            game_flag--;
            if (game_flag<=0)game_flag=2;
        }
        else if (Key_Num==2)
        {
             game_flag++;
            if (game_flag>=3)game_flag=1;
        }
        else if (Key_Num==3)
        {
            OLED_Clear();
            OLED_Update();
            game_flag_temp=game_flag;
        }
        
    if (game_flag_temp==1){return;}
    else if (game_flag_temp==2){DinoGame_Init();DinoGame_Animation();}
    
    
    switch (game_flag)
    {
    case 1:
        Game_UI();
        OLED_ReverseArea(0,0,16,16);
        OLED_Update();
        break;
    
    case 2:
        Game_UI();
        OLED_ReverseArea(0,16,80,16);
        OLED_Update();
        break;
    }
}
}

//*********************???????***********************/
void Show_Emoji(void)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        OLED_Clear();
        OLED_DrawEllipse(40,32,6,6-i,1);
        OLED_DrawEllipse(88,32,6,6-i,1);
        OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);
        OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);
        OLED_ShowImage(54,20,20,20,Mouth);
        OLED_Update();
        osDelay(100);
    }
    for (uint8_t i = 0; i < 3; i++)
    {
        OLED_Clear();
        OLED_DrawEllipse(40,32,6,4+i,1);
        OLED_DrawEllipse(88,32,6,4+i,1);
        OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);
        OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);
        OLED_ShowImage(54,20,20,20,Mouth);
        OLED_Update();
        osDelay(100);
    }
    osDelay(500);
    
}

void Menu_Emoji(void)
{
    
    while (1)
    {
		Show_Emoji();
        Key_Num=Key_GetNum();
        if (Key_Num==3)
        {
            OLED_Clear();
            OLED_Update();
            return;
        }
    }
}
//*********************LED***********************/
#define LED_GPIO    GPIOB
#define LED_PIN     GPIO_PIN_15

void LED_UI(void)
{
    OLED_ShowImage(0,0,16,16,Return);
    OLED_ShowString(0,16,"LED ON",OLED_8X16);
    OLED_ShowString(0,32,"LED OFF",OLED_8X16);
//	OLED_Update();
}

uint8_t LED_flag=1;
void Menu_LED(void)
{
	LED_UI();

    while (1)
    {
        uint8_t LED_flag_temp=0;
        Key_Num=Key_GetNum();
        if (Key_Num==1)
        {
            LED_flag--;
            if (LED_flag<=0)LED_flag=3;
        }
        else if (Key_Num==2)
        {
             LED_flag++;
            if (LED_flag>=4)LED_flag=1;
        }
        else if (Key_Num==3)
        {
            OLED_Clear();
            OLED_Update();
            LED_flag_temp=LED_flag;
        }

        if (LED_flag_temp==1){return;}
        else if (LED_flag_temp==2){HAL_GPIO_WritePin(LED_GPIO,LED_PIN, GPIO_PIN_RESET);}
        else if (LED_flag_temp==3){HAL_GPIO_WritePin(LED_GPIO,LED_PIN, GPIO_PIN_SET);}
        
        
        switch (LED_flag)
        {
        case 1:
            LED_UI();
            OLED_ReverseArea(0,0,16,16);
            OLED_Update();
            break;
        
        case 2:
            LED_UI();
            OLED_ReverseArea(0,16,48,16);
            OLED_Update();
            break;

        case 3:
            LED_UI();
            OLED_ReverseArea(0,32,56,16);
            OLED_Update();
            break;
        }
    }

}

//*********************????***********************/

void Show_Gradienter_UI(void)
{
    MPU6050_Calculation();
    OLED_DrawCircle(64,32,30,0);
    OLED_DrawCircle(64-Rool,32+Pitch,4,1);
}

void Menu_Gradienter(void)
{
    while (1)
    {
		Show_Gradienter_UI();
        Key_Num=Key_GetNum();
        if (Key_Num==3)
        {
            OLED_Clear();
            OLED_Update();
            return;
        }
        OLED_Clear();
        Show_Gradienter_UI();
        OLED_Update();
    }
}
