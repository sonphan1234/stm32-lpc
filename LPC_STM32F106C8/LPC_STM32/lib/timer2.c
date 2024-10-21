#include "stm32f10x.h"

void TIM2_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // Tính toán giá tr? Prescaler và Period d? d?t t?n s? 8 kHz
    uint32_t timer_clock = 72000000; // 72 MHz
    uint32_t sample_rate = 8000; // 8 kHz
    uint16_t prescaler = (uint16_t)((timer_clock / (sample_rate * 1000)) - 1);
    uint16_t period = 1000 - 1; // Ð?t giá tr? period cho 1 kHz

    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update); // Ch?n s? ki?n c?p nh?t làm ngu?n kích ho?t (TRGO)
    TIM_Cmd(TIM2, ENABLE);
}

void delay_ms(uint32_t t)
{
  while(t)
	{
	  TIM_SetCounter(TIM2, 0);
		while (TIM_GetCounter(TIM2) < 1000) {}
	  --t;
	}
}

void delay_us(uint32_t t)
{
	  TIM_SetCounter(TIM2, 0);
		while (TIM_GetCounter(TIM2) < t) {}
}
