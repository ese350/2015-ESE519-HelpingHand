/**
* developed for project HelpingHand as a part of ESE350
* by
* Chaitali Gondhalekar
* Yifeng Yuan
* 
* This code is for left hand unit!
*/
#include "main.h"

int main() {
    
    /*initialize storage for sensor data*/
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    send  = 0;
    /********* XBee init ***********/
    rst1 = 0; //Set reset pin to 0
    wait_ms(10);//Wait at least one millisecond
    rst1 = 1;//Set reset pin to 1
    wait_ms(10);//Wait another millisecond
       
    //vibration motor 
    vibrate = 0;
    //detect pinch - pressure sensors
    rightTurn.rise(&turnRight);
    leftTurn.rise(&turnLeft);
    turnCountRight = 0;
    turnCountLeft = 0;

    //acceleration plot
    x_ax = 0.0;
    
    /** detect closed fist **/
    quit = false; start = false; debounce = false;
    battery_flag = '1';//initialize, battery is good
    flex.rise(&flexed);   // attach the address of the toggle
    flex.fall(&unflexed);
    //check battery level every 10 seconds
    timerBattery.attach(&CheckBattery,10.0);
    
    usb.printf("starting transmission!\r\n");
    xbee1.baud(9600);
    

    while(!quit){

        isGameOver();

        /************** Menu ********************/
        playGame = false; plotData = false;
        usb.printf("waiting for user to choose an option..\r\n");
        //wait till user chooses an option
        //wait till hub sends ack for chosen option
//        usb.printf("waiting for ACK from Hub..\r\n");
        while(!xbee1.readable());
//        usb.printf("received something..\r\n");
        if(xbee1.readable()){
            int8_t received = xbee1.getc();
            if(received == 0) //option 1 = play game, changed from 1 to 0
                playGame = true;
            else if(received == 2) //no change
                plotData = true;
            else if (received == 4)//quit changed from 15 to 4
                quit = true;
//            usb.printf("implementing correct option: %d\r\n", received);
        }
        
        while(playGame){
            if(xbee1.readable()) 
                backToMenu();
            CheckSpeed();
            //DisplayLED();    
            /********************sending data***********************/
            decode();
            xbee1.putc(send);
            wait_ms(10);    
            xbee1.putc('\n');  
            wait_ms(10);    
            wait_ms(500);
        }//play game
        
//        usb.printf("plotting data\r\n");
        while(plotData){
            
            CheckSpeed();
            float l = leftData.read()*100;
            xbee1.putc(l);
            xbee1.putc(' '); 
            float r = rightData.read()*100;
            xbee1.putc(r);
            xbee1.putc(' ');
            xbee1.putc(x_ax);
            xbee1.putc('\n');
//            usb.printf("L: %f; R: %f Ac: %f\r\n",l,r,x_ax);
            wait_ms(500);  
        }//plot data
    }//quit
}

/**
* encode sensor data into one byte to be sent
*/
void decode(){
    send = 0;
    if(LEFT == 0){//left
        //reset 7th bit 
        send &= ~(1 << HAND);
    }
        
    //bits 6,5 - buf[0] - fwd/back/freeze
    int left = ~0 - ((1<< MOTION_MSB) - 1);
    int right = ((1<< MOTION_LSB)-1);
    int mask = left | right;
    send = (send & mask) | (buf[0] << MOTION_LSB );

//    //bits 4,3,2 - buf[1] - 
    left = ~0 - ((1<< SPEED_MSB) - 1);
    right = ((1<< SPEED_LSB) - 1);
    mask = left | right;
    send = (send & mask) | (buf[1] << SPEED_LSB );

//    //bit 1 - buf[2]
    if(buf[2] > 0) { //pressed
        //set bit 
        send |= 1 << LEFT_TURN;
    }
    if(buf[3] > 0){//pressed
        //set bit 0
        send |= 1 << RIGHT_TURN;
    }
}

/**
* derives speed of hand rotation from accelerometer readings
*/
void CheckSpeed(){
    
    int speed;
    int num_data = 10;
    float ax_raw_avg = 0.0;
    float x_temp;
    float z_temp;
    
    //detecing speed
    for (int j=0;j<num_data;j++) {
        axcl.read_xz(&x_temp,&z_temp);
        wait_ms(10);
        ax_raw_avg = ax_raw_avg + abs(x_temp);
    }
    ax_raw_avg = ax_raw_avg / num_data;
    x_ax = ax_raw_avg*100;
    speed = GearBox_ax(ax_raw_avg);
    buf[1] = speed;
}

/**
* maps speed of rotation to a value 1 through 5 
*/
int GearBox_ax(float speed) {
    if (speed <= 0.25) {
        return 0;
    } else if (speed > 0.25 && speed <= 0.45) {
        return 1;
    } else if (speed > 0.45 && speed <= 0.65) {
        return 2;
    } else if (speed > 0.65 && speed <= 0.85) {
        return 3;
    } else if (speed > 0.85 && speed <= 1.05) {
        return 4;
    } else {
        return 5;
    }
}

/**
* called when user taps his index finger
* records the tap count
* resets count to 0 if count reaches 5 or interval is over
*/
void turnRight(){
    
    if(turnCountRight == 0){
        timerRight.attach(&overflowRight, 5); //start timer
    }
     if(turnCountRight == 5){
        timerRight.detach(); //stop timer
        timerRight.attach(&overflowRight, 5); //start timer
        //reset the count after 90 deg turn
        turnCountRight = 0;
    }
    turnCountRight++;
    buf[3] = turnCountRight;
}

/**
* called when user taps his index finger
* records the tap count
* resets count to 0 if count reaches 5 or interval is over
*/
void turnLeft(){
    
    if(turnCountLeft == 0){
        timerLeft.attach(&overflowLeft, 5); //start timer
    }
     if(turnCountLeft == 5){
        timerLeft.detach(); //stop timer
        timerLeft.attach(&overflowLeft, 5); //start timer
        //reset the count after 90 deg turn
        turnCountLeft = 0;
    }
    turnCountLeft++;  
    buf[2] = turnCountLeft; 
}

/**
* called when middle finger pinch time interval is over
* resets tap count to 0
*/           
void overflowRight(){
    
    timerRight.detach(); //stop timer
    turnCountRight = 0;
    buf[3] = 0;
} 

/**
* called when index finger pinch time interval is over
* resets tap count to 0
*/ 
void overflowLeft(){
    
    timerLeft.detach(); //stop timer
    turnCountLeft = 0;
    buf[2] = 0;
}   

/**
* called when user flexes his index hand
* takes care of debouncing
*/
void flexed() {
    
    if(!start){
        flexInterval.start();
        start = true;
        buf[0] = 1;//backward
        debounce = false;
    }
    else{
        if(flexInterval.read_ms() > 50)  {//valid flex
            flexInterval.stop();
            flexInterval.reset(); 
            start = false;
            debounce = true;  
        }
    }   
}

/**
* called when user unflexes his index hand
* takes care of debouncing
*/
void unflexed() {
    int t= flexInterval.read_ms();
    if(start && !debounce && t > 80) { //valid unflex
        buf[0] = 0;//forward
        flexInterval.stop();
        flexInterval.reset();  
        start = false;     
        debounce = false;
        unflexInterval.start();
    }
    
}

/**
* called once per iteration of loop
* checks if battery voltage is below threshold value 
* if yes, sets a flag to indicate so
*/
void CheckBattery() {
    if (ain <= 0.54) {//battery is low, voltage <= 3.6V
        battery_flag = '0';
    }
}

/**
* debug
*/
void DisplayLED() {
    if (buf[0] == 0) {led1 = 1;}//forward
    else if (buf[0] == 1) {led1 = 0;}//backward
    if (buf[2] == 1) {led2 = 1;}//turn left
    if (buf[3] == 1) {led2 = 0;}//turn right
    switch (buf[1]) {
        case 0:
            led3 = 0.0;
            break;
        case 1:
            led3 = 0.2;
            break;
        case 2:
            led3 = 0.4;
            break;
        case 3:
            led3 = 0.6;
            break;
        case 4:
            led3 = 0.8;
            break;
        case 5:
            led3 = 1.0;
            break;
    }
}

/**
* called once per iteration of loop
* processes message received from Hub 
* decides state of the game
*/
void backToMenu(){
    
        int8_t received = xbee1.getc();
        if(received == 4){
            playGame = false; plotData = false;
        }
        else if(received == 1){//collision
            received = 0;
            vibrate = 1;
            vibrateInterval.attach(&vibration, 1.0);
        }
}

/**
* called once per loop
* processes message received from Hub 
* decides state of the game
*/
void isGameOver(){

    if(xbee1.readable()){
        int8_t received = xbee1.getc();
        if(received == 3){
            quit = false; playGame = false; plotData = false;
        }
    }
}

/**
* called when after 1 s vibration interval ends
* stops the ticker and vibration motor 
*/
void vibration(){

    vibrateInterval.detach();
    vibrate = 0;
}
