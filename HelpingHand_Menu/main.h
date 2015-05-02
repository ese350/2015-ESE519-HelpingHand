#include "L3GD20_YY.h"
#include "LSM303DLHC.h"
#include <math.h>
//#define bit numbers for Menu
#define LEFT 0 //means left
#define CHANGE_OPTION_UP 2  //decimal 2
#define CHANGE_OPTION_DOWN 1    //decimal 4
#define SELECT_OPTION 3         //decimal 8
//#define bit numbers for commands/data
#define RIGHT_TURN 0
#define LEFT_TURN 1
#define SPEED_LSB 2
#define SPEED_MSB 4
#define MOTION_LSB 5
#define MOTION_MSB 6
#define HAND 7

//L3GX_GYRO gyro(p_sda, p_scl, chip_addr, datarate, bandwidth, fullscale);
L3GX_GYRO gyro(p28, p27, 0x6b << 1); //sda 28, scl 27
LSM303DLHC axcl(p28, p27);

Serial usb(USBTX,USBRX);
Serial xbee1(p13, p14); //tx, rx
DigitalOut rst1(p30); //Digital reset for the XBee, 200ns for reset
DigitalOut vibrate(p26); //vibration motor
//for test
DigitalOut led1(LED1);
DigitalOut led2(LED2);
PwmOut led3(LED3);
InterruptIn rightTurn(p21); //from pressure sensor
InterruptIn leftTurn(p22);
InterruptIn flex(p24); //from flex sensor
AnalogIn ain(A0);//battery level detection, 1.8V - 1.95V represents 3.6V - 3.9V
AnalogIn rightData(A1);
AnalogIn leftData(A2);
Ticker timerRight;
Ticker timerLeft;
Ticker vibrateInterval;
Timer flexInterval;
Timer unflexInterval;
Ticker timerBattery;//for monitoring battery level

/*********** Functions *****************/
void CheckSpeed();
int GearBox_ax(float speed);
//for test
void DisplayLED();

/***************** ISRs***********/
void decode();
//pressure sensor
void turnRight();
void turnLeft();
void overflowRight();
void overflowLeft();

//flex sensor 
void flexed();
void unflexed();
//vibration motor
void vibration();

void isGameOver();
void backToMenu();
void CheckBattery();

/*********** Variables *******************/
int turnCountRight;
int turnCountLeft;
int buf[4];
uint8_t send;
bool start, quit, debounce;
/* Menu variables */
//bool chosen; 
bool playGame, plotData;
char battery_flag;//1 - battery good; 0 - need to be charged
float x_ax;