#ifndef __TIMESET_H
#define __TIMESET_H

extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;

int year_set(void);
void TimeSet_First_UI(void);
void TimeSet_Second_UI(void);
int year_set(void);
int month_set(void);
uint8_t Get_Max_Day(uint8_t year, uint8_t month);
int date_set(void);
int hour_set(void);
int minute_set(void);
int second_set(void);
void TimeSet(void);
#endif
