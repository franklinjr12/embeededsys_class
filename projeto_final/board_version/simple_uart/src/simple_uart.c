#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"

#include "system_TM4C1294.h" 


int comm_buffer_pos=0;
char comm_buffer[128];

//#define user_print(...) {UARTprintf(__VA_ARGS__);}
#define user_print(...) {;}
#define user_delay(milliseconds) {SysCtlDelay(milliseconds * (SysCtlClockGet() / 100 ));}//{SysCtlDelay(milliseconds*12000);}

#define MESSAGE_DELAY 1000
#define INITIAL_ACCELERATION 6.0f

extern void UARTStdioIntHandler(void);

void UARTInit(void){
  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 9600, SystemCoreClock);
} // UARTInit

void UART0_Handler(void){
  UARTStdioIntHandler();
} // UART0_Handler

void SysTick_Handler(void){
} // SysTick_Handler

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

void main(void){

  comm_init();
  
  SysTickPeriodSet(12000000); // f = 1Hz para clock = 24MHz
  
  SysTickIntEnable();
  SysTickEnable();

  comm_init();  

  //car_wait_start();  

  while(1) {
    float f;
    car_turn(5.0f);
    user_delay(MESSAGE_DELAY);
    car_read_rf(&f);
    user_delay(MESSAGE_DELAY);
  }
  
    car_accelerate(1.0);
    user_delay((int)(1000.0f*INITIAL_ACCELERATION));
    car_accelerate(0.0);
    user_delay(300);
    car_turn(10.0);
    user_delay(1000);
    car_turn(-10.0);
    user_delay(1000);

  
    float err=0;
    float kp=0.8;
    float ki=0.06;
    float err_i=0;
    const float max_err_i = 5;
    float kd = 1.3;
    float err_last=0;

    float car_sensor_read = 0;
  
  while(1){
    
        car_read_rf(&car_sensor_read);
        user_delay(MESSAGE_DELAY);

        err=-car_sensor_read;
        err_i+=err;
        if (err_i > max_err_i) err_i = max_err_i;
        else if (err_i < -max_err_i) err_i = -max_err_i;

        car_turn(kp*err+ki*err_i+kd*(err-err_last));
        err_last=err;
        user_delay(MESSAGE_DELAY);
    
  } // while
} // main

int comm_init() {
  UARTInit();
  //user_print("hello world\n");
  return 0;
}


int comm_write(char * wbuffer, int wbuffer_len) {
  UARTwrite(wbuffer, wbuffer_len);
  return 0;
}

int comm_read (char* rbuffer, int wbuffer_len, int* bytes_actually_read) {
    
  int aval = UARTRxBytesAvail();
  while (aval < 1) {
    user_delay(10);
    aval = UARTRxBytesAvail();
  }
  int i = 0;
  char c = 1;
  for (; i < aval; i++) {
      c = UARTgetc();
      comm_buffer[comm_buffer_pos++]=c;    
  }
  comm_buffer[comm_buffer_pos++]='\0';
  comm_buffer_pos=0;
  /*
    while( c!='\r' && c!='\n' && c!='\0') {
      c = UARTgetc();
      comm_buffer[comm_buffer_pos++]=c;
    }
    comm_buffer[comm_buffer_pos++]=c;
    comm_buffer[comm_buffer_pos++]='\0';
    user_print("uart got %s\n", comm_buffer);
    comm_buffer_pos = 0;  
    */    
    //UARTgets(rbuffer, wbuffer_len);
    user_print("comm got %s\n", rbuffer);
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
    //user_print("turn %f\n",units);
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
    // if (velocity > 0) acceleration -= 0.1;
    // else if (velocity < 0) acceleration += 0.1;
    // snprintf(buf, 30, "A%2.2f;", acceleration);
    snprintf(buf, 30, "S;");
    return comm_write(buf, strlen(buf));
}

int car_read_laser (float* ret) {
    char buf[30];
    // snprintf(buf, 30, "Pl;");
    strcpy(buf, "Pl;");
    int res = comm_write(buf, strlen(buf));
    if (res != 0) return res;
    // user_delay(10);
    res = comm_read(buf, 30, NULL);    
    *ret = atof(&buf[2]);
    return res;
}
int car_read_ultrassound (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Pu;");
    int res = comm_write(buf, strlen(buf));
    if (res != 0) return res;
    // user_delay(1);
    res = comm_read(buf, 30, NULL);
    *ret = atof(&buf[2]);    
    return res;
}
int car_read_rf (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Prf;");
    int res = comm_write(buf, strlen(buf));
    if (res != 0) return res;
    user_delay(10);
    res = comm_read(buf, 30, NULL);
    *ret = atof(&buf[3]);    
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