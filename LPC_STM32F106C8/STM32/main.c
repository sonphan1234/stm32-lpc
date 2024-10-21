#include "stm32f10x.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <math.h>

#define LPC_ORDER 5
#define ADC_BUF_SIZE 1024
#define ADC_BUF_MASK (ADC_BUF_SIZE - 1)
#define OVS_COUNT 8
#define TIM_PERIOD ((72000000 / (8000 * OVS_COUNT)) - 1) // T?n s? l?y m?u 8kHz

volatile int16_t ovs_acc = 0;
volatile uint8_t ovs_cnt = 0;
volatile uint16_t adc_val = 0;
volatile uint8_t adc_buffer[ADC_BUF_SIZE];
volatile uint16_t adc_buf_pos = 0;

QueueHandle_t queue;

void tim4_ini(void) {
TIM_TimeBaseInitTypeDef TIMER_InitStructure;
RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
TIM_TimeBaseStructInit(&TIMER_InitStructure);
TIMER_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
TIMER_InitStructure.TIM_Prescaler = 0;
TIMER_InitStructure.TIM_Period = TIM_PERIOD; // Thi?t l?p t?n s? l?y m?u ? 8kHz
TIM_TimeBaseInit(TIM4, &TIMER_InitStructure);
TIM_SelectOutputTrigger(TIM4, TIM_TRGOSource_Update); // S? ki?n c?p nh?t du?c ch?n làm tín hi?u kích ho?t
TIM_Cmd(TIM4, ENABLE);
TIM_ARRPreloadConfig(TIM4, ENABLE); // Kích ho?t ch? d? n?p l?i
}

void adc_ini(void) {
ADC_InitTypeDef ADC_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;

RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);

GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
GPIO_Init(GPIOA, &GPIO_InitStructure);

NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
NVIC_Init(&NVIC_InitStructure);

RCC_ADCCLKConfig(RCC_PCLK2_Div6); // C?u hình clock ADC t?i da là 12MHz
RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
ADC_InitStructure.ADC_ScanConvMode = DISABLE;
ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
ADC_InitStructure.ADC_NbrOfChannel = 1;
ADC_Init(ADC1, &ADC_InitStructure);

ADC_InjectedSequencerLengthConfig(ADC1, 1);
ADC_InjectedChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_13Cycles5); // 13.5 cycles
ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T4_TRGO); // S? d?ng TIM4 TRGO làm tín hi?u kích ho?t
ADC_SetInjectedOffset(ADC1, ADC_InjectedChannel_1, 1900);
ADC_Cmd(ADC1, ENABLE);
ADC_ResetCalibration(ADC1);
while (ADC_GetResetCalibrationStatus(ADC1));
ADC_StartCalibration(ADC1);
while (ADC_GetCalibrationStatus(ADC1));
ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE); // Kích ho?t ng?t JEOC
ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE); // Kích ho?t kênh ADC
}

void ADC1_2_IRQHandler(void) {
if (ADC_GetITStatus(ADC1, ADC_IT_JEOC)) {
ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
ovs_acc += (int16_t)(ADC1->JDR1); // L?y giá tr? ADC
ovs_cnt++;
if (ovs_cnt >= OVS_COUNT) {
adc_val = ovs_acc / ovs_cnt;
adc_buffer[adc_buf_pos++] = (adc_val & 0x00FF);
adc_buffer[adc_buf_pos++] = (adc_val >> 8);
adc_buf_pos &= ADC_BUF_MASK;
ovs_acc = 0;
ovs_cnt = 0;
}
}
}

void SetSysClock(void) {
ErrorStatus HSEStartUpStatus;
RCC_DeInit();
RCC_HSEConfig(RCC_HSE_ON);
HSEStartUpStatus = RCC_WaitForHSEStartUp();

if (HSEStartUpStatus == SUCCESS) {
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
    FLASH_SetLatency(FLASH_Latency_2);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK2Config(RCC_HCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PLLConfig(0x00010000, RCC_PLLMul_9); // PLLCLK = 8MHz * 9 = 72 MHz 
    RCC_PLLCmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);
} else { 
    while (1); // L?i
}
}

void LPC_Analyze(const int16_t *signal, int length, float *lpc_coefficients) {
float autocorrelation[LPC_ORDER + 1] = {0};
float E[LPC_ORDER + 1] = {0};
float k[LPC_ORDER] = {0};
float a[LPC_ORDER + 1] = {0};

// Tính toán các h? s? t? tuong quan
for (int i = 0; i <= LPC_ORDER; i++) {
    for (int j = 0; j < length - i; j++) {
        autocorrelation[i] += (float)signal[j] * signal[j + i];
    }
}

// Thu?t toán Levinson-Durbin d? tính các h? s? LPC
E[0] = autocorrelation[0];
for (int i = 1; i <= LPC_ORDER; i++) {
    float sum = 0;
    for (int j = 1; j < i; j++) {
        sum += a[j] * autocorrelation[i - j];
    }
    k[i] = (autocorrelation[i] - sum) / E[i - 1];
    a[i] = k[i];

    for (int j = 1; j < i; j++) {
        a[j] = a[j] - k[i] * a[i - j];
    }
    E[i] = (1 - k[i] * k[i]) * E[i - 1];
}

// Sao chép các h? s? LPC vào m?ng k?t qu?
for (int i = 1; i <= LPC_ORDER; i++) {
    lpc_coefficients[i - 1] = a[i];
}
}

void Reconstruct_ADC_Data(float *lpc_coefficients, int length, uint16_t adc_value, uint16_t *reconstructed_data) {
float prediction_error = adc_value; // Sai s? d? doán ban d?u là giá tr? ADC
for (int i = 0; i < length; i++) {
float prediction = 0;
for (int j = 1; j <= LPC_ORDER; j++) {
if (i >= j) {
prediction += lpc_coefficients[j - 1] * reconstructed_data[i - j];
}
}
reconstructed_data[i] = (uint16_t)(prediction + prediction_error + 0.5); // D? li?u tái t?o là d? doán c?ng v?i sai s?
prediction_error = adc_value - reconstructed_data[i]; // C?p nh?t sai s? d? doán cho l?n l?y m?u ti?p theo
}
}

void vTaskReadSensor(void *pvParameters) {
int16_t signal_buffer[100];
int index = 0;
float lpc_coefficients[LPC_ORDER];
struct {
uint16_t adc_value;
float lpc_coefficients[LPC_ORDER];
} data_to_send;
while (1) {
    if (adc_buf_pos != 0) {
        uint16_t adc_value = (adc_buffer[adc_buf_pos - 2]) | (adc_buffer[adc_buf_pos - 1] << 8);
        signal_buffer[index++] = adc_value;

        if (index >= 100) {
            LPC_Analyze(signal_buffer, 100, lpc_coefficients);
            data_to_send.adc_value = adc_value;
            for (int i = 0; i < LPC_ORDER; i++) {
                data_to_send.lpc_coefficients[i] = lpc_coefficients[i];
            }
            xQueueSend(queue, &data_to_send, portMAX_DELAY);
            index = 0; 
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000 / 8000)); // Delay tuong ?ng v?i t?n s? l?y m?u 8kHz
}
}

void vTaskSendUART(void *pvParameters) {
char buffer[50];
struct {
uint16_t adc_value;
float lpc_coefficients[LPC_ORDER];
} received_data;
while (1) {
    if (xQueueReceive(queue, &received_data, portMAX_DELAY) == pdPASS) {
        snprintf(buffer, sizeof(buffer), "%d\n", received_data.adc_value);
        uart_SendStr(buffer);

        for (int i = 0; i < LPC_ORDER; i++) {
            snprintf(buffer, sizeof(buffer), "LPC[%d]: %.6f\n", i, received_data.lpc_coefficients[i]);
        }
    }
}
}

// Tác v? g?i d? li?u tái t?o lên UART
void vTaskReconstructADC(void *pvParameters) {
struct {
uint16_t adc_value;
float lpc_coefficients[LPC_ORDER];
} received_data;
while (1) {
    if (xQueueReceive(queue, &received_data, portMAX_DELAY) == pdPASS) {
        uint16_t reconstructed_data;
        Reconstruct_ADC_Data(received_data.lpc_coefficients, 1, received_data.adc_value, &reconstructed_data);

        char buffer[50];
        snprintf(buffer, sizeof(buffer), "%d\n", reconstructed_data);
        uart_SendStr(buffer);
    }
}
}

int main(void) {
SystemInit();
uart_Init();
SetSysClock();
adc_ini();
tim4_ini();
queue = xQueueCreate(10, sizeof(float[LPC_ORDER]));

xTaskCreate(vTaskReadSensor, "ReadSensor", 128, NULL, 1, NULL);
xTaskCreate(vTaskSendUART, "SendUART", 128, NULL, 1, NULL);
xTaskCreate(vTaskReconstructADC, "ReconstructADC", 128, NULL, 1, NULL);

vTaskStartScheduler();

while (1) {
}
}