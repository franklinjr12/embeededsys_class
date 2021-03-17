//#include "system_tm4c1294.h" // CMSIS-Core
//#include "driverleds.h" // device drivers
//#include "cmsis_os2.h" // CMSIS-RTOS
//
//osThreadId_t thread1_id, thread2_id;
//
//void thread1(void *arg){
//  uint8_t state = 0;
//  
//  while(1){
//    state ^= LED1;
//    LEDWrite(LED1, state);
//    osDelay(100);
//  } // while
//} // thread1
//
//void thread2(void *arg){
//  uint8_t state = 0;
//  uint32_t tick;
//  
//  while(true){
//    tick = osKernelGetTickCount();
//    
//    state ^= LED2;
//    LEDWrite(LED2, state);
//    
//    osDelayUntil(tick + 100);
//  } // while
//} // thread2
//
//void main(void){
//  LEDInit(LED2 | LED1);
//
//  osKernelInitialize();
//
//  thread1_id = osThreadNew(thread1, NULL, NULL);
//  thread2_id = osThreadNew(thread2, NULL, NULL);
//
//  if(osKernelGetState() == osKernelReady)
//    osKernelStart();
//
//  while(1);
//} // main

#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h"      // device drivers
#include "cmsis_os2.h"       // CMSIS-RTOS

#define true 1
#define false 0

#define NUM_THREADS 4
osThreadId_t threads[NUM_THREADS];

typedef struct{
  int led;
  long time;  
} aux_struct;

void thread_body(void *param){
  int b = 0;
  long count;
  aux_struct *s = (aux_struct *)param;
  while (true){
    count = osKernelGetTickCount();
    b ^= s->led;
    LEDWrite(s->led, b);
    osDelayUntil(count + s->time);
  }
}

void main(void){
  LEDInit(LED1 | LED2 | LED3 | LED4);

  osKernelInitialize();

  aux_struct structs[NUM_THREADS];
  structs[0].time = 200;
  structs[0].led = LED1;
  structs[1].time = 300;
  structs[1].led = LED2;
  structs[2].time = 500;
  structs[2].led = LED3;
  structs[3].time = 700;
  structs[3].led = LED4;

  for(int i = 0; i < NUM_THREADS; i++){
    threads[i] = osThreadNew(thread_body, &structs[i], NULL);
  }

  if (osKernelGetState() == osKernelReady)
    osKernelStart();

  while(true){}    
}
