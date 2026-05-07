#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "rtc.h"
#include <stdio.h>
#include "Key.h"
#include "TIMESET.h"
#include "menu.h"
#include "item.h"
#include "power.h"
#include "adc.h"
#define GET_MENU_INDEX(x) (((x) + 7) % 7)

// 使用Key.h中声明的全局变量Key_Num
void Paper1_Init(void)
{
    MX_RTC_Init();
    MX_ADC1_Init();
}

uint16_t ADValue;
float VBAT;
int Battery_Capacity;

void Shwo_Battery(void)
{
    int sum;
     for (int i=0;i<3000;i++)
    {
        HAL_ADC_Start(&hadc1);
        ADValue=HAL_ADC_GetValue(&hadc1);
        sum+=ADValue;
    }
    ADValue=sum/3000;
    VBAT=(float)ADValue/4095*3.3;
    Battery_Capacity=(ADValue-3276)*100/735;
    if (Battery_Capacity<0){Battery_Capacity=0;}
    
//	OLED_Printf(64,0,OLED_6X8,0,"%d",ADValue);
//	OLED_Printf(64,8,OLED_6X8,0,"VBAT:%.2fV",VBAT);
    OLED_ShowNum(85,4,Battery_Capacity,3,OLED_6X8);
    OLED_ShowChar(103,4,'%',OLED_6X8);
    if (Battery_Capacity==100)     OLED_ShowImage(110,0,16,16,Battery);
    else if (Battery_Capacity>=10&&Battery_Capacity<100) 
    {
        OLED_ShowImage(110,0,16,16,Battery);
        OLED_ClearArea((112+Battery_Capacity/10),5,(10-Battery_Capacity/10),6);
        OLED_ClearArea(85,4,6,8);
    }
    else if(Battery_Capacity<10)
    {
        OLED_ShowImage(110,0,16,16,Battery);
        OLED_ClearArea(112,5,10,6);
        OLED_ClearArea(85,4,6,8);
    }
}

void Show_Clock_UI(void)
{
    Shwo_Battery();
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    OLED_Printf(0, 0,OLED_6X8,0,"20%02d-%02d-%02d",sDate.Year, sDate.Month, sDate.Date);
    OLED_Printf(16,16,OLED_12X24,0,"%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
    OLED_ShowString(0,48,"Men",OLED_8X16);
    OLED_ShowString(96,48,"Set",OLED_8X16);
}

int clkflag=1;     

int First_Page_Clack(void)
{
    while (1)
    {
        Key_Num=Key_GetNum();
        if (Key_Num==1)//?????
        {
            clkflag --;
            if (clkflag<=0)clkflag=2;
        }
        else if (Key_Num==2)//?????
        {
            clkflag ++;
            if (clkflag>=3)clkflag=1;
        }
        else if (Key_Num==3)//???
        {
            OLED_Clear();
            OLED_Update();
            return clkflag;
        }
        else if (Key_Num==4)//???
        {
           HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
           HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
        }

    switch (clkflag)
    {
    case 1:
        Show_Clock_UI();
        OLED_ReverseArea(0,48,24,16);
        OLED_Update();
        break;

    case 2:
        Show_Clock_UI();
        OLED_ReverseArea(96,48,24,16);
        OLED_Update();
        break;
    }
  }
}


void Show_SettingPage(void)

{
    OLED_Clear();
    OLED_ShowImage(0,0,16,16,Return);
    OLED_ShowString(0,16,"Time Set",OLED_8X16);
}

int setflag=1; 

int SettingPage(void)
{
	Show_SettingPage();
	OLED_Update();
    while (1)
    {
		uint8_t setflag_temp=0;
        Key_Num=Key_GetNum();
        if (Key_Num==1)//?????
        {
            setflag --;
            if (setflag<=0)setflag=2; 
        }
        else if (Key_Num==2)//?????

        {
            setflag ++;
			if (setflag>=3)setflag=1;
        }
		
        else if (Key_Num==3)//???
		{
            OLED_Clear();
            OLED_Update();
            setflag_temp=setflag;
        }
	if(setflag_temp==1){return 0;}
    else if(setflag_temp==2){TimeSet();}
    
    switch (setflag)
    {
    case 1:

        Show_SettingPage();
        OLED_ReverseArea(0,0,16,16);
        OLED_Update();
        break;

    case 2:

        Show_SettingPage();
        OLED_ReverseArea(0,16,96,16);
        OLED_Update();
        break;
    }
  }
}



/*------------------------------???????-------------------------------*/

MenuItem menu_item = {0, 0, 48, 8, 0}; // ??????????

void Menu_Animation(void)

{
    OLED_Clear();
    OLED_ShowImage(42,10,44,44,Frame);
    if (menu_item.pre_selection<menu_item.target_selection) // ???????
    {
        menu_item.x_pre -= menu_item.Speed;
        if (menu_item.x_pre==0) // ???????????
        {
            menu_item.pre_selection ++;
            menu_item.move_flag = 0; // ?????
			menu_item.x_pre = 48; // ????x????
        }
    }
    else if (menu_item.pre_selection>menu_item.target_selection) // ???????
    {
        menu_item.x_pre += menu_item.Speed;
        if (menu_item.x_pre == 96) // ???????????
        {
			menu_item.pre_selection --;
            menu_item.move_flag = 0; // ?????
            menu_item.x_pre = 48; // ????x????
        }
    }

    if ( menu_item.pre_selection>=1)
    {
        OLED_ShowImage(menu_item.x_pre-48,16,32,32,Menu_Graph[menu_item.pre_selection-1]);
    }

    
    if ( menu_item.pre_selection>=2)
    {
        OLED_ShowImage(menu_item.x_pre-96,16,32,32,Menu_Graph[menu_item.pre_selection-2]);
    }
    OLED_ShowImage(menu_item.x_pre+48,16,32,32,Menu_Graph[menu_item.pre_selection+1]);
    OLED_ShowImage(menu_item.x_pre,16,32,32,Menu_Graph[menu_item.pre_selection]);
    OLED_ShowImage(menu_item.x_pre+96,16,32,32,Menu_Graph[menu_item.pre_selection+2]);
    OLED_Update();

}


void Set_Selection(uint8_t move_flag,uint8_t Pre_Selection,uint8_t Target_Selection)
{  
    if (move_flag == 1) // ??????
    {
        menu_item.pre_selection = Pre_Selection;
        menu_item.target_selection = Target_Selection;
        Menu_Animation();
    }
}

void Menu_FirstPage(void)

{
	menu_item.pre_selection %= 7; 

    OLED_Clear(); 
    OLED_ShowImage(42, 10, 44, 44, Frame); 
    OLED_ShowImage(0, 16, 32, 32, Menu_Graph[GET_MENU_INDEX(menu_item.pre_selection - 1)]);
    OLED_ShowImage(48, 16, 32, 32, Menu_Graph[GET_MENU_INDEX(menu_item.pre_selection)]);
    OLED_ShowImage(96, 16, 32, 32, Menu_Graph[GET_MENU_INDEX(menu_item.pre_selection + 1)]);
    OLED_Update();

}



uint8_t menu_flag=1;

int Menu(void)

{
    Menu_FirstPage();
    uint8_t DirectFlag;//1,????????,2,????????
    while (1)

    {
        Key_Num=Key_GetNum();
        uint8_t menu_flag_temp=0;
        if (Key_Num==1)//?????
        {
            DirectFlag=1;
            menu_item.move_flag=1;
            menu_flag --;
            if (menu_flag<=0)menu_flag=7;  
        }
        else if (Key_Num==2)
        {
            DirectFlag=2;
            menu_item.move_flag=1;
            menu_flag ++;
            if (menu_flag>=8)menu_flag=1;
        }
        else if (Key_Num==3)//???
        {
            OLED_Clear();
            OLED_Update();
            menu_flag_temp=menu_flag;
        }

        if (menu_flag_temp==1){return 0;}
        else if (menu_flag_temp==2){Menu_Timer();Menu_FirstPage();OLED_Update();}
        else if (menu_flag_temp==3){Menu_LED();Menu_FirstPage();OLED_Update();}
        else if (menu_flag_temp==4){Menu_MPU6050();Menu_FirstPage();OLED_Update();}
        else if (menu_flag_temp==5){Menu_Game();Menu_FirstPage();OLED_Update();}
        else if (menu_flag_temp==6){Menu_Emoji();Menu_FirstPage();OLED_Update();}
        else if (menu_flag_temp==7){Menu_Gradienter();Menu_Gradienter();Menu_FirstPage();OLED_Update();}

		if (menu_flag==1)
		{
			if(DirectFlag==1)Set_Selection(menu_item.move_flag,1,0);
			else if(DirectFlag==2)Set_Selection(menu_item.move_flag,0,0);
		}
		else
		{
			if(DirectFlag==1)Set_Selection(menu_item.move_flag,menu_flag,menu_flag-1);
			else if(DirectFlag==2)Set_Selection(menu_item.move_flag,menu_flag-2,menu_flag-1);
    }
  }
}



