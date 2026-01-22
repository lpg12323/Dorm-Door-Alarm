#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Buzzer.h"
#include "Door.h"
#include "Key.h"
#include "OLED.h"

//定义三个状态：撤防（安全）、布防（警戒）、报警（抓人）
typedef enum {
	STATE_DISARMED = 0,
	STATE_ARMED,
	STATE_ALARMING
} SecurityState_t;

SecurityState_t CurrentState = STATE_DISARMED;
uint8_t StateChanged = 1;

int main(void)
{
	Buzzer_Init();
	Door_Init();
	Key_Init();
	OLED_Init();
	
	Buzzer_OFF();
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);
	
	uint32_t AlarmTimer = 0;
	
	while(1)
	{
        // ================= PART 1: 处理按键 (切换模式) =================
		if (Key_GetNum() == 1)
		{
			if (CurrentState == STATE_DISARMED)
			{
				CurrentState = STATE_ARMED;
				// 响两声短的，提示布防
				Buzzer_ON(); Delay_ms(100); Buzzer_OFF(); Delay_ms(100);
				Buzzer_ON(); Delay_ms(100); Buzzer_OFF();
			}
			else
			{
				CurrentState = STATE_DISARMED;
				// 响一声长，提示撤防
				Buzzer_ON(); Delay_ms(500); Buzzer_OFF();
			}
			StateChanged = 1;
		}
		
		// ================= PART 2: 根据状态干活 (状态机) =================
		switch (CurrentState)
		{
			case STATE_DISARMED: //【在家模式】
				Buzzer_OFF();
				GPIO_SetBits(GPIOC, GPIO_Pin_13); // 灯灭 (PC13高电平灭)
				break; 
			
			case STATE_ARMED:    //【警戒模式】
				GPIO_ResetBits(GPIOC, GPIO_Pin_13); // 灯亮 (PC13低电平亮)
				
				if (Door_GetState() == 1) // 如果门开了
				{
					CurrentState = STATE_ALARMING; // 切换状态
					StateChanged = 1;
				}
				break; 
				
			case STATE_ALARMING: //【报警模式】
				AlarmTimer ++;
				if (AlarmTimer >= 10)
				{
				
			    // 狂闪灯 + 狂叫
				Buzzer_Turn();
				
				// 翻转 LED 灯状态 (闪烁)
				if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) == 0)
					GPIO_SetBits(GPIOC, GPIO_Pin_13);
				else
					GPIO_ResetBits(GPIOC, GPIO_Pin_13);
				
				}
				break;
		}
		// ================= PART 3: 显示逻辑 =================
		if (StateChanged == 1)
		{
			OLED_Clear();
			switch (CurrentState)
			{
				case STATE_DISARMED:
					OLED_ShowString(1, 1, "Mode: DISARMED");
					OLED_ShowString(2, 1, "Safe & Sound  ");
					break;
				case STATE_ARMED:
					OLED_ShowString(1, 1, "Mode: STATE_ARMED");
					OLED_ShowString(2, 1, "Watching You     ");
					break;
				case STATE_ALARMING:
					OLED_ShowString(1, 1, "Mode: ALARMING");
					OLED_ShowString(2, 1, "POLICE COMING ");
					break;
			}
			StateChanged = 0;
		}
		Delay_ms(10);
	}
}		
