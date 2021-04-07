#include <stdbool.h>
#include <stdint.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverbuttons.h"

#define BUFFER_SIZE 8

osThreadId_t produtor_id, consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;
uint8_t buffer[BUFFER_SIZE];

uint8_t p_index_i = 0, p_count = 0, b_int_enable = 0;

void consumidor(void *arg){
  uint8_t index_o = 0, state;
  
  while(1){    
    osSemaphoreAcquire(cheio_id, osWaitForever); // há dado disponível?
    state = buffer[index_o]; // retira do buffer
    osSemaphoreRelease(vazio_id); // sinaliza um espaço a mais
    
    index_o++;
    if(index_o >= BUFFER_SIZE) // incrementa índice de retirada do buffer
      index_o = 0;
    
    LEDWrite(LED4 | LED3 | LED2 | LED1, state); // apresenta informação consumida

    if(b_int_enable == 0){
      osDelay(300);
      b_int_enable = 1;
      ButtonIntEnable(USW1);
    }    
    
    osDelay(100);
  } // while
} // consumidor

void isr_handler(void){
  ButtonIntClear(USW1);
  
  osSemaphoreAcquire(vazio_id, osWaitForever); // há espaço disponível?
  buffer[p_index_i] = p_count; // coloca no buffer
  osSemaphoreRelease(cheio_id); // sinaliza um espaço a menos
  
  p_index_i++; // incrementa índice de colocação no buffer
  if(p_index_i >= BUFFER_SIZE)
    p_index_i = 0;
  
  p_count++;
  p_count &= 0x0F; // produz nova informação
  
  ButtonIntDisable(USW1);//desabilita a interrupção por alguns ms
                        //para nao pular a contagem
  b_int_enable = 0;  
}

void main(void){
  SystemInit();
  LEDInit(LED4 | LED3 | LED2 | LED1);

  osKernelInitialize();

  consumidor_id = osThreadNew(consumidor, NULL, NULL);

  vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL); // espaços disponíveis = BUFFER_SIZE
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0

  
  GPIOIntRegister(GPIO_PORTJ_BASE, isr_handler);   
  ButtonInit(USW1);
  ButtonIntEnable(USW1);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1){
  }
} // main
