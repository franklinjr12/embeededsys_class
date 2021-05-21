#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PART_TM4C1294NCPDT
#include "system_tm4c1294.h" // CMSIS-Core
#include "cmsis_os2.h" // CMSIS-RTOS

#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

#include "driverlib/ssi.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"

#include "utils/uartstdio.h"

#define BUFFER_SIZE 50

//doenst need print with board debuger
#define user_print(...) {;}
#define user_delay(milliseconds) {osDelay(milliseconds);}//{SysCtlDelay(milliseconds*12000);}

//an delay after messages is needed to avoid the simulator to freeze
#define MESSAGE_DELAY 300
#define INITIAL_ACCELERATION 6.0f

int comm_buffer_pos=0;
char comm_buffer[128];

char write_buffer[BUFFER_SIZE];
char read_buffer[BUFFER_SIZE];

const osThreadAttr_t thread_at = {
  .stack_size = 1024*8
};

osThreadId_t thread_sender_id;
osThreadId_t thread_receiver_id;
osThreadId_t thread_controller_id;

osMutexId_t uart_mutex_id; // Mutex ID

char buffer[BUFFER_SIZE];
float f;

bool should_write, should_read, should_control;

//init the PID variables
float err=0;
float kp=0.8;
float ki=0.06;
float err_i=0;
const float max_err_i = 5;
float kd = 1.3;
float err_last=0;

float car_sensor_read = 0;

void uart_init();

int comm_init () ;
int comm_write (char* wbuffer, int wbuffer_len) ;
int comm_read (char* rbuffer, int wbuffer_len, int* bytes_actually_read) ;

void car_wait_start () ;
int car_stop () ;
int car_turn (float units) ;
int car_accelerate (float units) ;
int car_read_laser (float* ret) ;
int car_read_ultrassound (float* ret) ;
int car_read_rf (float* ret) ;
int car_read_cam (float* ret) ;

void thread_sender_task(void *arg){
  
  while(1){       
    
    while (!should_write) osDelay(1);
            
    if (osMutexAcquire(uart_mutex_id, 10) == osOK) {
      should_write = false;  
      UARTwrite((char*)write_buffer, strlen((char*)write_buffer));
//      int len = strlen((char*)write_buffer);
//      for (int i = 0; i < len; i++) UARTCharPut(UART0_BASE, '!');
      while(UARTBusy(UART0_BASE)) osDelay(1);
      memset(write_buffer, 0, sizeof(write_buffer));
      osMutexRelease(uart_mutex_id);          
    }    
          
  }
} 
void thread_receiver_task(void *arg){
  while(1){   
    
    while (!should_read) osDelay(1);
    
    if (osMutexAcquire(uart_mutex_id, 10) == osOK) {
      should_read = false;
      uint8_t timeout = 0;
      const uint8_t TIMEOUT = 300;
      while (UARTBusy(UART0_BASE)) {
        osDelay(1);
        timeout++;
        if (timeout > TIMEOUT) {
          osMutexRelease(uart_mutex_id);
          break;
        }
      }
      while (!UARTCharsAvail(UART0_BASE)) {
        osDelay(1);
        timeout++;
        if (timeout > TIMEOUT) {
          osMutexRelease(uart_mutex_id);
          break;
        }        
      }
      
      memset(read_buffer, 0, sizeof(read_buffer));
      int i = 0;
      while (UARTCharsAvail(UART0_BASE)) {
        read_buffer[i++] = UARTCharGetNonBlocking(UART0_BASE);
      }
      
      err = -(float)atof((char*)&read_buffer[3]);
      should_control = true;
//      snprintf((char*)read_buffer, sizeof(read_buffer), "f=%d\n", (int)(f*100.0f));
//      //UARTwrite((char*)read_buffer, strlen((char*)read_buffer));
//      for (int i = 0; i < strlen((char*)write_buffer); i++) UARTCharPut(UART0_BASE, write_buffer[i]);
//      while(UARTBusy(UART0_BASE)) osDelay(1);
      
      osMutexRelease(uart_mutex_id);
    }        
    
  }
} 
void thread_controller_task(void *arg){

  //give the car an initial velocity and turn
  car_accelerate(1.0);
  user_delay((int)(1000.0f*INITIAL_ACCELERATION));
  car_accelerate(0.0);
  user_delay(300);
  car_turn(10.0);
  user_delay(1000);  
  
  should_control = true;
  while(1){   

    while (!should_control) osDelay(1);
    should_control = false;
    
    //read sensor
    car_read_rf(&car_sensor_read);
    user_delay(MESSAGE_DELAY);
    
    //update err and integrator err and fix integrator saturarion
    //err=-car_sensor_read;
    err_i+=err;
    if (err_i > max_err_i) err_i = max_err_i;
    else if (err_i < -max_err_i) err_i = -max_err_i;
    
    //actuate
    car_turn(kp*err+ki*err_i+kd*(err-err_last));
    user_delay(MESSAGE_DELAY);
    
    //update differentiator err
    err_last=err;
  
    user_delay(1000);      
  }
} 


void main(void){
  SystemInit();
  osKernelInitialize();
  
  comm_init();
  
  should_write = false;
  should_read = false;  
  should_control = false;  
  
//  thread_sender_id = osThreadNew(thread_sender_task, NULL, NULL);
//  thread_receiver_id = osThreadNew(thread_receiver_task, NULL, NULL);
//  thread_controller_id = osThreadNew(thread_controller_task, NULL, NULL);
  

  
  thread_sender_id = osThreadNew(thread_sender_task, NULL, &thread_at);
  if (thread_sender_id == NULL) while(1);
  thread_receiver_id = osThreadNew(thread_receiver_task, NULL, &thread_at);
  if (thread_receiver_id == NULL) while(1);
  thread_controller_id = osThreadNew(thread_controller_task, NULL, &thread_at);  
  if (thread_controller_id == NULL) while(1);
  
  uart_mutex_id = osMutexNew(NULL);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();
  
  while(1){
  }
} // main


void uart_init() {
  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  //  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
//  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){}  
  //UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));  
  
//  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
//  GPIOPinConfigure(GPIO_PA0_U0RX);
//  GPIOPinConfigure(GPIO_PA1_U0TX);  
//  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
//  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
//                      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
//                       UART_CONFIG_PAR_NONE));
//  UARTEnable(UART0_BASE);
//  UARTFIFOEnable(UART0_BASE);
 
  
//  UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));
//  UARTEnable(UART0_BASE);
  
  
  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  
  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 9600, SystemCoreClock);
}

int comm_init() {
  uart_init();
  return 0;
}


int comm_write(char * wbuffer, int wbuffer_len) {
  
  strcpy(write_buffer, wbuffer);
  should_write = true;
  
  //UARTwrite(wbuffer, wbuffer_len);
  return 0;
}

int comm_read (char* rbuffer, int wbuffer_len, int* bytes_actually_read) {
//  strcpy(rbuffer, read_buffer);
  should_read = true;
//  memset(comm_buffer, 0, sizeof(comm_buffer));
//  memset(rbuffer, 0, wbuffer_len);
//  
//  int aval = UARTRxBytesAvail();
//  while (aval < 1) {
//    user_delay(100);
//    aval = UARTRxBytesAvail();
//  }
//  for (int i=0; i < aval; i++) {    
//    comm_buffer[comm_buffer_pos++]=UARTgetc();    
//  }
//  comm_buffer[comm_buffer_pos++]='\0';
//  comm_buffer_pos=0;
//
//  int temp = strlen(comm_buffer);
//  if ( temp <= wbuffer_len)
//    for (int i = 0; i < temp; i++) rbuffer[i] = comm_buffer[i]; 
//  else return -1;
//  
//  user_print("comm got %s\n", rbuffer);

  return 0;
}

void car_wait_start () {
  const int buffer_size = 20;
  char buffer[20];
  strcpy(buffer, "");
  while (strcmp(buffer, "inicio") != 0) {
    comm_read(buffer, buffer_size, NULL);
    user_delay(100);
  }    
}

int car_turn (float units) {
  if (units > 10.0f) units = 10.0f;
  else if (units < -10.0f) units = -10.0f;  
  char buf[30];
  snprintf(buf, 30, "V%2.2f;", units);
  return comm_write(buf, strlen(buf));
}

int car_accelerate (float units) {
  char buf[30];
  snprintf(buf, 30, "A%2.2f;", units);
  return comm_write(buf, strlen(buf));
}

int car_stop () {
  char buf[30];
  snprintf(buf, 30, "S;");
  return comm_write(buf, strlen(buf));
}

int car_read_laser (float* ret) {
  char buf[30];
  strcpy(buf, "Pl;");
  int res = comm_write(buf, strlen(buf));
  if (res != 0) return res;
  user_delay(10);
  res = comm_read(buf, 30, NULL);    
  *ret = atof(&buf[2]);
  return res;
}
int car_read_ultrassound (float* ret) {
  char buf[30];
  snprintf(buf, 30, "Pu;");
  int res = comm_write(buf, strlen(buf));
  if (res != 0) return res;
  user_delay(1);
  res = comm_read(buf, 30, NULL);
  *ret = atof(&buf[2]);    
  return res;
}
int car_read_rf (float* ret) {  
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Prf;");
  int res = comm_write(buffer, strlen(buffer));
  if (res != 0) return res;
  user_delay(10);
  res = comm_read(buffer, sizeof(buffer), NULL);
  (*ret) = atof(&buffer[3]);      
  return res;
}
int car_read_cam (float* ret) {
  char buf[30];
  snprintf(buf, 30, "Pbcam;");
  int res = comm_write(buf, strlen(buf));
  if (res != 0) return res;
  user_delay(10);
  res = comm_read(buf, 30, NULL);
  *ret = atof(&buf[5]);    
  return res;
}
