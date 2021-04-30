#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define user_print(...) {printf(__VA_ARGS__);}
#define user_delay(milliseconds) {Sleep(milliseconds);}

#define COM_PORT "\\\\.\\COM3"
#define baudrate 9600
#define byteSize 8
#define stopBits TWOSTOPBITS
#define parity NOPARITY

float v_inicial = 0;
float a_inicial = 0;

HANDLE serialHandle;
DCB serialParams = { 0 };
COMMTIMEOUTS timeout = { 0 };

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

int main (int argc, char **argv) {

    user_print("hello world!\n");

    comm_init();

    // car_wait_start();
    // user_delay(1000);

    float car_sensor_read = 0;

    while (true) {
        // car_turn(10.0f);
        // user_delay(2000);

        if (!car_read_laser(&car_sensor_read)) {user_print("Laser: %f\n", car_sensor_read);}
        else {user_print("error reading laser");}
        user_delay(1000);

        if (!car_read_ultrassound(&car_sensor_read)) {user_print("Ultrassound: %f\n", car_sensor_read);}
        else {user_print("error reading laser");}
        user_delay(1000);

        car_read_rf(&car_sensor_read);
        user_print("Rf: %f\n", car_sensor_read);
        user_delay(1000);

        car_read_cam(&car_sensor_read);
        user_print("Cameras: %f\n", car_sensor_read);
        user_delay(1000);
    }

    CloseHandle(serialHandle);
    return 0;
}

int comm_init () {
    serialHandle = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    serialParams.DCBlength = sizeof(serialParams);
    GetCommState(serialHandle, &serialParams);
    serialParams.BaudRate = baudrate;
    serialParams.ByteSize = byteSize;
    serialParams.StopBits = stopBits;
    serialParams.Parity = parity;
    SetCommState(serialHandle, &serialParams);
    timeout.ReadIntervalTimeout = 5;
    timeout.ReadTotalTimeoutConstant = 5;
    timeout.ReadTotalTimeoutMultiplier = 1;
    timeout.WriteTotalTimeoutConstant = 5;
    timeout.WriteTotalTimeoutMultiplier = 1;
    SetCommTimeouts(serialHandle, &timeout);
    return 0;
}

int comm_write(char* wbuffer, int wbuffer_len) {
    DWORD wrote_len;
    LPOVERLAPPED lpOverlapped = NULL;
    user_print("sending i2c: %s\n", wbuffer);
    WriteFile(serialHandle, wbuffer, wbuffer_len, &wrote_len, lpOverlapped);
    return 0;
}
int comm_read(char* rbuffer, int rbuffer_len, int* bytes_actually_read) {
    DWORD dwCommEvent;
    DWORD dwRead;
    char chRead[rbuffer_len];
    int pos = 0;
    if (!SetCommMask(serialHandle, EV_RXCHAR)) return -1;
    while (true) {
        // if (true) {
        if (WaitCommEvent(serialHandle, &dwCommEvent, NULL)) {
            if (ReadFile(serialHandle, chRead, rbuffer_len, &dwRead, NULL)) {                
                user_print("comm got %s\n", chRead);
                if (bytes_actually_read != NULL) *bytes_actually_read = dwRead;
                if (strlen(chRead) < rbuffer_len) {
                    strcpy(rbuffer, chRead);
                    return 0;
                }
                else return -1;                
            }
            else {
                // An error occurred in the ReadFile call.
                return -1;
                break;
            }
        }
        else {
            // Error in WaitCommEvent.
            return -1;
            break;
        }
    }    
    return 0;
}

void car_wait_start () {
    const int buffer_size = 20;
    char buffer[buffer_size];
    strcpy(buffer, "");
    while (strcmp(buffer, "inicio") != 0) {
        comm_read(buffer, buffer_size, NULL);
        user_delay(100);
    }    
}

int car_turn (float units) {
    char buf[30];
    snprintf(buf, 30, "V%2.2f;", units);
    return comm_write(buf, 30);
}

int car_accelerate (float units) {
    char buf[30];
    snprintf(buf, 30, "A%2.2f;", units);
    return comm_write(buf, 30);
}

int car_stop () {
    char buf[30];
    snprintf(buf, 30, "S;");
    return comm_write(buf, 30);
}

int car_read_laser (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Pl;");
    int res = comm_write(buf, 30);
    if (res != 0) return res;
    // user_delay(1);
    res = comm_read(buf, 30, NULL);
    *ret = atof(buf);
    return res;
}
int car_read_ultrassound (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Pu;");
    int res = comm_write(buf, 30);
    if (res != 0) return res;
    // user_delay(1);
    res = comm_read(buf, 30, NULL);
    *ret = atof(buf);    
    return res;
}
int car_read_rf (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Prf;");
    int res = comm_write(buf, 30);
    if (res != 0) return res;
    user_delay(10);
    res = comm_read(buf, 30, NULL);
    *ret = atof(buf);    
    return res;
}
int car_read_cam (float* ret) {
    char buf[30];
    snprintf(buf, 30, "Pbcam;");
    int res = comm_write(buf, 30);
    if (res != 0) return res;
    user_delay(10);
    res = comm_read(buf, 30, NULL);
    *ret = atof(buf);    
    return res;
}