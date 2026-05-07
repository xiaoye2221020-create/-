#include "main.h"
#include "OLED.h"
#include "rtc.h"
#include <stdio.h>
#include "Key.h"
#include "TIMESET.h"
#include "menu.h"

RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;


void TimeSet_First_UI(void)
{
	uint16_t Year = 2000 + sDate.Year;
    OLED_ShowImage(0,0,16,16,Return);
    OLED_ShowString(0,16,"年",OLED_8X16);
    OLED_ShowString(0,32,"月",OLED_8X16);
    OLED_ShowString(0,48,"日",OLED_8X16);
    OLED_Printf(16,16,OLED_8X16,5,":%4d",Year);
    OLED_Printf(16,32,OLED_8X16,0,":%2d",sDate.Month);
    OLED_Printf(16,48,OLED_8X16,0,":%2d",sDate.Date);
}  

void TimeSet_Second_UI(void)
{
    OLED_ShowString(0,0,"时",OLED_8X16);
    OLED_ShowString(0,16,"分",OLED_8X16);
    OLED_ShowString(0,32,"秒",OLED_8X16);
	OLED_Printf(16,0,OLED_8X16,0,":%2d",sTime.Hours);
    OLED_Printf(16,16,OLED_8X16,0,":%2d",sTime.Minutes);
    OLED_Printf(16,32,OLED_8X16,0,":%2d",sTime.Seconds);
	
}  

int year_set(void)
{ 
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            if (sDate.Year > 0) sDate.Year--;
            else sDate.Year = 99;
        }
        else if (Key_Num == 2)
        {
            sDate.Year++;
            if (sDate.Year > 99) sDate.Year = 0;
        }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_First_UI();
		
        OLED_ReverseArea(24,16,32,16);
        OLED_Update();
    }

    
}


uint8_t Get_Max_Day(uint8_t year, uint8_t month)
{
    // 1-12月的天数表（平年）
    const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 如果不是2月，直接查表
    if (month != 2)
    {
        return days_in_month[month - 1];
    }

    // 如果是2月，判断闰年
   else{ // 简化的闰年判断：2000-2099年间，只要能被4整除就是闰年（2000年也是闰年）
    if (year % 4 == 0)
    {
        return 29; // 闰年2月
    }
    else
    {
        return 28; // 平年2月
    }
   }
}


int month_set(void)
{ 
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            sDate.Month--;
            if(sDate.Month<=0){sDate.Month=12;}
        }
        else if (Key_Num == 2)
        {
            sDate.Month++;
            if (sDate.Month > 12) sDate.Month = 1;
        }
		
		uint8_t max_day = Get_Max_Day(sDate.Year, sDate.Month);
            if (sDate.Date > max_day)
            {
                sDate.Date = max_day; 
            }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_First_UI();
        OLED_ReverseArea(24,32,16,16);
        OLED_Update();
		
		
		
    }

    
}








int date_set(void)
{ 
    uint8_t max_day = Get_Max_Day(sDate.Year, sDate.Month);
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            sDate.Date--;
            if(sDate.Date<=0){sDate.Date=max_day;}
        }
        else if (Key_Num == 2)
        {
            sDate.Date++;
            if (sDate.Date >max_day) sDate.Date = 1;
        }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_First_UI();
        OLED_ReverseArea(24,48,16,16);
        OLED_Update();
    }

    
}


int hour_set(void)
{ 
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            if (sTime.Hours > 0) sTime.Hours--;
            else sTime.Hours = 23;
        }
        else if (Key_Num == 2)
        {
            sTime.Hours++;
            if (sTime.Hours > 23) sTime.Hours = 0;
        }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_Second_UI();
        OLED_ReverseArea(24,0,16,16);
        OLED_Update();
    }

    
}


int minute_set(void)
{ 
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            if (sTime.Minutes > 0) sTime.Minutes--;
            else sTime.Minutes = 59;
        }
        else if (Key_Num == 2)
        {
            sTime.Minutes++;
            if (sTime.Minutes > 59) sTime.Minutes = 0;
        }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_Second_UI();
        OLED_ReverseArea(24,16,16,16);
        OLED_Update();
    }

    
}


int second_set(void)
{ 
    while (1)
		
    {
        Key_Num = Key_GetNum();
        if (Key_Num == 1)
        {
            if (sTime.Seconds > 0) sTime.Seconds--;
            else sTime.Seconds = 59;
        }
        else if (Key_Num == 2)
        {
            sTime.Seconds++;
            if (sTime.Seconds > 59) sTime.Seconds = 0;
        }
        else if (Key_Num == 3)
        {
            HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            return 0; 
        }
        TimeSet_Second_UI();
        OLED_ReverseArea(24,32,16,16);
        OLED_Update();
    }

    
}









int timeset_flg = 1;
void TimeSet(void)
{
    while (1)
    {
        uint8_t set_time_flag_temp=0;
        Key_Num=Key_GetNum();
        if (Key_Num==1)//上一项
        {
            timeset_flg --;
            if (timeset_flg<=0)timeset_flg=7;
            
        }
        else if (Key_Num==2)//下一项
        {
             timeset_flg ++;
            if (timeset_flg>=8)timeset_flg=1;
        }
        else if (Key_Num==3)//确定
        {
            OLED_Clear();
            OLED_Update();
            set_time_flag_temp=timeset_flg;
        }
        
        if (set_time_flag_temp==1){return;}
        else if (set_time_flag_temp==2){year_set();}//年份设置
        else if (set_time_flag_temp==3){month_set();}//月份设置
        else if (set_time_flag_temp==4){date_set();}//日期设置
        else if (set_time_flag_temp==5){hour_set();}//时间设置
        else if (set_time_flag_temp==6){minute_set();}//分钟设置
        else if (set_time_flag_temp==7){second_set();}//秒设置
        


    
    switch (timeset_flg)
    {
    case 1:
        OLED_Clear();
        TimeSet_First_UI();
        OLED_ReverseArea(0,0,16,16);
        OLED_Update();
        break;
    
    case 2:
         OLED_Clear();
        TimeSet_First_UI();
        OLED_ReverseArea(0,16,16,16);
        OLED_Update();
        break;

    case 3:
         OLED_Clear();
        TimeSet_First_UI();
        OLED_ReverseArea(0,32,16,16);
        OLED_Update();
        break;

    case 4:
        OLED_Clear();
        TimeSet_First_UI();
        OLED_ReverseArea(0,48,16,16);
        OLED_Update();
        break;

    case 5:
         OLED_Clear();
        TimeSet_Second_UI();
        OLED_ReverseArea(0,0,16,16);
        OLED_Update();
        break;

    case 6:
        OLED_Clear();
        TimeSet_Second_UI();
        OLED_ReverseArea(0,16,16,16);
        OLED_Update();
        break;

    case 7:
        OLED_Clear();
        TimeSet_Second_UI();
        OLED_ReverseArea(0,32,16,16);
        OLED_Update();
        break;


    }
  }

}
