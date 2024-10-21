#include "uart.h"
#include <stdio.h> // B? sung thu vi?n stdio.h d? s? d?ng FILE và __stdout

// Không c?n d?nh nghia l?i __FILE ? dây

// Ð?nh nghia hàm fputc d? s? d?ng cho printf
int fputc(int ch, FILE *f) {
    uart_SendChar(ch); // G?i ký t? qua UART
    return ch; // Tr? v? ký t? dã g?i
}


void uart_Init(void){
    GPIO_InitTypeDef gpio_typedef;
    USART_InitTypeDef usart_typedef;

    // B?t clock cho GPIOA và USART1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // C?u hình chân Tx (A9) là chân AF push-pull
    gpio_typedef.GPIO_Pin = GPIO_Pin_9;
    gpio_typedef.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_typedef);	

    // C?u hình chân Rx (A10) là chân input floating
    gpio_typedef.GPIO_Pin = GPIO_Pin_10;
    gpio_typedef.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_typedef.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_typedef);

    // C?u hình USART1
    usart_typedef.USART_BaudRate = 115200;
    usart_typedef.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_typedef.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; 
    usart_typedef.USART_Parity = USART_Parity_No;
    usart_typedef.USART_StopBits = USART_StopBits_1;
    usart_typedef.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &usart_typedef);

    USART_Cmd(USART1, ENABLE);
}

void uart_SendChar(char _chr){
    USART_SendData(USART1, _chr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void uart_SendStr(char *str){
    while(*str != NULL){
        uart_SendChar(*str++);		
    }
}
