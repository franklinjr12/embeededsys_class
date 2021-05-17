#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOWS
#define MESSAGE_DELAY 300
#define INITIAL_ACCELERATION 6.0f

float velocity = 0;
float acceleration = 0;
float angle = 0;
const float max_angle = 10.0f;
float last_angle = angle;

#if defined(WINDOWS)
#include <windows.h>
#define user_print(...) {printf(__VA_ARGS__);}
#define user_delay(milliseconds) {Sleep(milliseconds);}
#define COMM_TIMEOUT 300
#define COM_PORT "\\\\.\\COM3"
#define baudrate 9600
#define byteSize 8
#define stopBits ONESTOPBIT
#define parity NOPARITY

HANDLE serialHandle;
DCB serialParams = { 0 };
COMMTIMEOUTS timeout = { 0 };

DWORD WINAPI f(LPVOID param) {
    while(1) {
        velocity += acceleration;        
        // user_print("v %f a %f\n", velocity, acceleration);
        user_delay(1000);
    }
}
#endif


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

    // user_print("hello world!\n");

#if defined(WINDOWS)
    CreateThread(NULL, 0, f, NULL, 0, NULL);
#endif
    comm_init();

    // car_wait_start();
    user_delay(1000);

    float car_sensor_read = 0;

    car_accelerate(1.0);
    user_delay(1000.0f*INITIAL_ACCELERATION);
    car_accelerate(0);
    // car_turn(10);
    // angle = 10.0f;
    // user_delay(1000);

    float err=0;
    float kp=0.8;
    float ki=0.06;
    float err_i=0;
    float kd = 1.3;
    float err_last=0;

    const float max_err_i = 5;

    while (true) {
        // car_turn(10.0f);
        // // user_delay(2000);

        // if (!car_read_laser(&car_sensor_read)) {user_print("Laser: %f\n", car_sensor_read);}
        // else {user_print("error reading laser\n");}
        // // user_delay(1000);

        // if (!car_read_ultrassound(&car_sensor_read)) {user_print("Ultrassound: %f\n", car_sensor_read);}
        // else {user_print("error reading laser\n");}
        // // user_delay(1000);

        // car_read_rf(&car_sensor_read);
        // user_print("Rf: %f\n", car_sensor_read);
        // // user_delay(1000);

        // car_read_cam(&car_sensor_read);
        // user_print("Cameras: %f\n", car_sensor_read);
        // // user_delay(1000);
    

        car_read_rf(&car_sensor_read);
        user_delay(MESSAGE_DELAY);

        err=-car_sensor_read;
        err_i+=err;
        if (err_i > max_err_i) err_i = max_err_i;
        else if (err_i < -max_err_i) err_i = -max_err_i;

        car_turn(kp*err+ki*err_i+kd*(err-err_last));
        err_last=err;
        user_delay(MESSAGE_DELAY);
    }

    CloseHandle(serialHandle);
    return 0;
}

#if defined(WINDOWS)
int comm_init () {
    // serialHandle = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    serialHandle = CreateFile(COM_PORT, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (serialHandle == INVALID_HANDLE_VALUE) {
       //  Handle the error.
       user_print ("CreateFile failed with error %d.\n", GetLastError());
       return -1;
    }
    serialParams.DCBlength = sizeof(DCB);
    GetCommState(serialHandle, &serialParams);
    serialParams.BaudRate = baudrate;
    serialParams.ByteSize = byteSize;
    serialParams.StopBits = stopBits;
    serialParams.Parity = parity;
    bool status = SetCommState(serialHandle, &serialParams);
    if (!status) {
        user_print("failed to init comm");
        return -1;
    }
    timeout.ReadIntervalTimeout = COMM_TIMEOUT;
    timeout.ReadTotalTimeoutConstant = COMM_TIMEOUT;
    timeout.ReadTotalTimeoutMultiplier = COMM_TIMEOUT;
    timeout.WriteTotalTimeoutConstant = COMM_TIMEOUT;
    timeout.WriteTotalTimeoutMultiplier = COMM_TIMEOUT;
    status = SetCommTimeouts(serialHandle, &timeout);
    if (!status) {
        //  Handle the error.
        user_print ("SetCommState failed with error %d.\n", GetLastError());
        return -1;
    }
    PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return 0;
}

int comm_write(char* wbuffer, int wbuffer_len) {
    PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    DWORD wrote_len;
    LPOVERLAPPED lpOverlapped = NULL;
    // user_print("sending: %s\n", wbuffer);
    char temp[wbuffer_len+5];
    strcpy(temp, wbuffer);
    strcat(temp, "\r\n\0");
    // WriteFile(serialHandle, wbuffer, wbuffer_len, &wrote_len, lpOverlapped);
    WriteFile(serialHandle, temp, strlen(temp), &wrote_len, lpOverlapped);
    if (((int)wrote_len) != strlen(temp)) {
        user_print("should write %d wrote %d\n", strlen(temp), wrote_len);
        return -1;
    }
    return 0;
}
int comm_read(char* rbuffer, int rbuffer_len, int* bytes_actually_read) {
    PurgeComm(serialHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
    DWORD dwCommEvent;
    DWORD dwRead;
    char chRead[rbuffer_len];
    int pos = 0;
    // if (!SetCommMask(serialHandle, EV_RXCHAR)) return -1;
    while (true) {
        if (true) {
        // if (WaitCommEvent(serialHandle, &dwCommEvent, NULL)) {
            if (ReadFile(serialHandle, chRead, rbuffer_len, &dwRead, NULL)) {                
                // user_print("comm got %s\n", chRead);
                // for (int i = 0; i < dwRead; i++) user_print("%d ",chRead[i]);
                // user_print(" END\n");
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
#endif
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
    if (units > 10) units = 10;
    else if (units < -10) units = -10;
    user_print("turn %f\n",units);
    char buf[30];
    snprintf(buf, 30, "V%2.2f;", units);
    return comm_write(buf, strlen(buf));
}

int car_accelerate (float units) {
    acceleration=units;
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