

TaskHandle_t Display_Task; // run webpages and display
TaskHandle_t GPS_Task; // runs gps, control loop
TaskHandle_t Step_Task; // manage stepper outputs

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Fonts/FreeSerifBold24pt7b.h>"
#include <Fonts/FreeSerifBold18pt7b.h>"
#include <Fonts/FreeSerif18pt7b.h>"
#include <Fonts/FreeSerif9pt7b.h>"
#include <XPT2046_Touchscreen.h>

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

#include "FS.h"
#include "LITTLEFS.h"
#include <ArduinoJson.h>//Version 5
#define SPIFFS LITTLEFS
#define FORMAT_LITTLEFS_IF_FAILED true
bool File_Avaliable;

#include <Adafruit_GPS.h>
#define GPSSerial Serial2
Adafruit_GPS GPS(&GPSSerial);

// For the Adafruit shield, these are the default.
#define TFT_CS    22
#define TFT_RST   21
#define TFT_DC    5
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_MISO  19
#define TFT_LED   4

#define Touch_CS  2
#define TIRQ_PIN  15
XPT2046_Touchscreen ts(Touch_CS);
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC,TFT_RST);
// If using the breakout, change pins as desired
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

#define Dir_Pin 32
#define Step_Pin 33

int GPS_Update_Ms[400];
unsigned long GPS_Last_Ms;

float Speed = 0;
float Setpoint = 10;
bool Active = false;
int Control_Mode = 0; //0-off || 1-surf || 2-Wakeboard || 3-Ski
int hour;
int minute;

//Control Parameters
bool pause_log = false;
float Default_Speeds[6];

bool Got_GPS_Parse = false;
long  Motor_Reverse_Dir;
long  Motor_Steps_Per_Rotation;
int   Motor_Speed;
long  Motor_Max_Rotation;
int   GPS_Hz;
float kp;
float ki;
float kd;
float deadband;
float ramp_rate;
float proportional;
float integral;
float old_integral;
float derivative;
float error;
float previous_error;
int output;
int Current_Output;
int Old_Output;
int old_Direction;

//Web Server Files
float speed_log[1201];
float proportional_log[1201];
float integral_log[1201];
float derivative_log[1201];
int   output_log[1201];


//AP Timeout
bool AP_Off     = 0;
bool Old_AP_Off = 0;
unsigned long AP_Timer = 0;

//TFT Parameters
unsigned long old_speed_millis = 0;
unsigned long old_time_millis = 0;
unsigned long old_touch_millis = 0;
unsigned long old_down_millis = 0;
unsigned long old_up_millis = 0;
int old_minute;
float Filtered_Speed = 0;
int Old_TFT_MPH = -1;
int MPH_100;
int MPH_10;
int MPH_1;
int Old_MPH_100 = -1;
int Old_MPH_10  = -1;
int Old_MPH_1   = -1;
bool  Cruise_Old_Active;
float Cruise_Old_Setpoint = -1;
int   Old_Mode = -1;

//tft touch calibration
int X_min =469;
int X_max = 4200;
int Y_min = 459;
int Y_max = 4000;
int TFT_X_Max = 340;
int TFT_Y_Max = 240;

//Junk Test Params
float Ramp_Speed = 0.05;

WebServer server(80); //Server on port 80
#include "file_handler.h";
#include "webpages.h"
#include "webpage_handler.h"

void  TFT_Static_Elements();
void  TFT_MPH(float MPH);
void  Time(int hour, int minute);
void  Cruise_Box(bool Active,float MPH_SP);
void  Up_Arrow(bool touched);
void  Down_Arrow(bool touched);
void  Mode_Box(int Mode); 

void setup() {
  Serial.begin(115200);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);   // turn the LCD backlight off till after boot
  pinMode(Dir_Pin, OUTPUT);  
  pinMode(Step_Pin, OUTPUT);  
   
  delay(500); 
  
  if(!SPIFFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        File_Avaliable = false;
        return;
    }
  else File_Avaliable = true;
  Read_Parameters_File(SPIFFS, "/Parameters.txt");

  AP_Timer = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Cruise_Pro", "SpeedSetter");//Configuration Access Point
  Start_Webpage_Server();


  //create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
                    Display_Task_Code,   /* Task function. */
                    "Task1",     /* name of task. */
                    30000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Display_Task,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
    xTaskCreatePinnedToCore(
                    GPS_Task_Code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    &GPS_Task,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
    delay(500); 
    xTaskCreatePinnedToCore(
                    Step_Task_Code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Step_Task,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
    delay(500); 
  
}


void Display_Task_Code( void * pvParameters ){  Start_Webpage_Server();   
  Serial.print("Task1 Display running on core ");
  Serial.println(xPortGetCoreID());
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  TFT_Static_Elements();
  Cruise_Box(false,Setpoint);
  Up_Arrow(false);
  Down_Arrow(false);
  Mode_Box(Control_Mode); 
  if (!ts.begin()) { 
    Serial.println("Unable to start touchscreen.");
  } 
  else { 
    Serial.println("Touchscreen started."); 
  } 
  digitalWrite(TFT_LED, HIGH);   // turn the LCD backlight on after boot 

  
  for(;;){   
    if((millis() - AP_Timer) < 6000000){
      server.handleClient();          //Handle client requests
      AP_Off = false;
    }
    else AP_Off = true;
    if((AP_Off != Old_AP_Off) && AP_Off){
      WiFi.mode(WIFI_OFF);  
      Old_AP_Off = AP_Off;     
    }
    vTaskDelay( 1 / portTICK_PERIOD_MS );
    yield();
    if ((millis() - old_speed_millis) > 500) {
      TFT_MPH(Speed);
      old_speed_millis = millis();
    }
    if ((millis() - old_time_millis) > 5000) {
      Time(GPS.hour,GPS.minute);
      old_time_millis = millis();
    }
    Up_Arrow(false);
    Down_Arrow(false);
    
    if (ts.touched() && ((millis() - old_touch_millis) > 250)) {
      old_touch_millis = millis();  
      TS_Point p = ts.getPoint();
      int Touch_X;
      int Touch_Y;
      Touch_X = Scale(X_min,X_max,TFT_X_Max,p.x);
      Touch_Y = Scale(Y_min,Y_max,TFT_Y_Max,p.y);
      if((Touch_X < 200) && (Touch_Y < 80)){
        Control_Mode++;
        if(Control_Mode >= 4) Control_Mode = 0;
        Mode_Box(Control_Mode);      
      }
      if((Touch_X > 220) && (Touch_Y < 120)){
        if(Control_Mode == 1) Default_Speeds[Control_Mode] = Default_Speeds[Control_Mode] + 0.1;
        else                  Default_Speeds[Control_Mode] = Default_Speeds[Control_Mode] + 0.5;
        Cruise_Box(Active,Default_Speeds[Control_Mode]);
        Up_Arrow(true);        
      }    
      if((Touch_X > 220) && (Touch_Y > 120)){
        if(Control_Mode == 1) Default_Speeds[Control_Mode] = Default_Speeds[Control_Mode] - 0.1;
        else                  Default_Speeds[Control_Mode] = Default_Speeds[Control_Mode] - 0.5;
        Cruise_Box(Active,Default_Speeds[Control_Mode]);
        Down_Arrow(true);        
      }
    }
    vTaskDelay( 20 / portTICK_PERIOD_MS );
  }
}
void GPS_Task_Code( void * pvParameters ){
  Serial.print("Task2 PID running on core ");
  Serial.println(xPortGetCoreID());

  GPS.begin(38400);
  vTaskDelay(1000 / portTICK_PERIOD_MS );
  unsigned long SP_Last_Ms = millis();  

  for(;;){    
    if (GPS.available()){
      GPS.read();
      if (GPS.newNMEAreceived()) {
        //Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
        Got_GPS_Parse = true;
        if (!GPS.parse(GPS.lastNMEA())){ // this also sets the newNMEAreceived() flag to false
          Got_GPS_Parse = false;        
        }
      }
    }
    else vTaskDelay( 1 / portTICK_PERIOD_MS );
    if (Got_GPS_Parse) {
      if((int)GPS.fix > 0){      
        for (int i = 300; i >= 0; i--) {
          GPS_Update_Ms[i+1] = GPS_Update_Ms[i];
        }
        GPS_Update_Ms[0] = millis() - GPS_Last_Ms;
        GPS_Last_Ms = millis();   
        Speed = GPS.speed * 1.15077945;
        Filtered_Speed = Filtered_Speed + (0.1 * (Speed - Filtered_Speed));
        int Speed_Min = Speed - 1;
        int Speed_Max = Speed + 1;
        Filtered_Speed = constrain(Filtered_Speed,Speed_Min,Speed_Max);
        Got_GPS_Parse = false;        
        PI_Loop(Control_Mode == 0);
        
        if(!pause_log){
          for (int i = 600; i >= 0; i--) {
            speed_log[i+1]        = speed_log[i];
            proportional_log[i+1] = proportional_log[i];
            integral_log[i+1]     = integral_log[i];
            derivative_log[i+1]   = derivative_log[i];
            output_log[i+1]       = output_log[i];
          }
          speed_log[0]        = Speed;
          proportional_log[0] = proportional;
          integral_log[0]     = integral;
          derivative_log[0]   = derivative;
          output_log[0]       = Current_Output;
        }
      }
    }   
  } 
}


void Step_Task_Code( void * pvParameters ){
  Motor_Direction(!Motor_Reverse_Dir);
  for (int i = 0; i < (Motor_Max_Rotation); i++) {
    Motor_Pulse_Step(1);
  }
  Current_Output = 0;
  delay(100);  
  for(;;){  
    if(output > (Current_Output + 2)){
      Motor_Direction(Motor_Reverse_Dir);
      Motor_Pulse_Step(1);
      Current_Output++;
    }     
    else if((output < (Current_Output - 2)) || ((output < Current_Output) && (output == 0))){
      Motor_Direction(!Motor_Reverse_Dir);
      Motor_Pulse_Step(1);
      Current_Output--;
    } 
    else{ 
      Motor_Direction(!Motor_Reverse_Dir);  
      vTaskDelay( 20 / portTICK_PERIOD_MS ); 
    }
  }  
}

void loop() {  
}


void Motor_Direction(bool Dir){
  if(Dir) digitalWrite(Dir_Pin, HIGH);   // turn the LCD backlight on after boot 
  else    digitalWrite(Dir_Pin, LOW);   // turn the LCD backlight on after boot 
  if(Dir != old_Direction)  vTaskDelay( 1 / portTICK_PERIOD_MS );    
  old_Direction = Dir;    
}

void Motor_Pulse_Step(int ms){
  digitalWrite(Step_Pin, HIGH);   // turn the LCD backlight on after boot 
  vTaskDelay( ms / portTICK_PERIOD_MS );
  digitalWrite(Step_Pin, LOW);   // turn the LCD backlight on after boot 
  vTaskDelay( ms / portTICK_PERIOD_MS );
}


//Control FUNCTIONS
void PI_Loop(bool disable){ 
  if (disable){
    integral = 0;
    old_integral = 0;
    output = 0;
    previous_error = 0;
  }
  else {
    error = Filtered_Speed - Default_Speeds[Control_Mode];
    if (error > deadband) {
      error = error - deadband;
    }
    else if (error < (deadband * -1)) {
      error = error + deadband;
    }
    else error = 0;
    
    proportional  = kp * error;
    
    derivative    = kd * (error - previous_error);
    previous_error = error;
    
    int integral_min  = (old_integral - ramp_rate);
    integral_min      = constrain(integral_min,0,Motor_Max_Rotation);
    
    int integral_max  = (old_integral + ramp_rate);
    integral_max      = constrain(integral_max,0,Motor_Max_Rotation);
    
    integral      = integral + (error * ki);
    integral      = constrain(integral,integral_min, integral_max);
    old_integral  = integral;

    output = proportional + integral + derivative;
    output = constrain(output, 0, Motor_Max_Rotation);
  }
}










void TFT_MPH(float MPH) {
  int x_coordinate;
  int y_coordinate;
  //twos place
  int MPH_100  = MPH / 10;
  int Decimal = MPH_100%10;
  if(Old_MPH_100 != MPH_100){
    x_coordinate  = 20;
    y_coordinate  = 140;
    tft.fillRect(x_coordinate+2, y_coordinate - 64, 44, 68, ILI9341_BLACK);
    tft.setFont(&FreeSerifBold24pt7b);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
    tft.setCursor(x_coordinate, y_coordinate);
    if(Decimal > 0) tft.println(Decimal);
    else            tft.println(" ");
    Old_MPH_100 = MPH_100;
  }
  
  //ones place
  int MPH_10  = MPH;
  Decimal = MPH_10%10;
  if(Old_MPH_10 != MPH_10){
    x_coordinate  = 70;
    y_coordinate  = 140;
    tft.fillRect(x_coordinate+2, y_coordinate - 64, 44, 68, ILI9341_BLACK);
    tft.setFont(&FreeSerifBold24pt7b);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
    tft.setCursor(x_coordinate, y_coordinate);
    tft.println(Decimal);
    Old_MPH_10 = MPH_10;
  }
  
  //Decimal
  int MPH_1  = MPH * 10;
  Decimal = MPH_1%10;
  if(Old_MPH_1 != MPH_1){
    x_coordinate  = 142;
    y_coordinate  = 140;
    tft.fillRect(x_coordinate+2, y_coordinate - 64, 44, 68, ILI9341_BLACK);
    tft.setFont(&FreeSerifBold24pt7b);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
    tft.setCursor(x_coordinate, y_coordinate);
    tft.println(Decimal);
    Old_MPH_1 = MPH_1;
  }

}

  

void TFT_Static_Elements() {

  //MPH 
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.setFont(&FreeSerif9pt7b);
  int x_coordinate  = 78;
  int y_coordinate  = 120;
  tft.setCursor(x_coordinate + 30, y_coordinate+ 50);
  tft.println("MPH");
  
  //Decimal Point
  x_coordinate  = 118;
  y_coordinate  = 140;
  tft.fillRect(x_coordinate+4, y_coordinate - 64, 16, 68, ILI9341_BLACK);
  tft.setFont(&FreeSerifBold24pt7b);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.setCursor(x_coordinate, y_coordinate);
  tft.println(".");

}

void Cruise_Box(bool Active,float MPH_SP){  
  if((Active != Cruise_Old_Active) || (MPH_SP != Cruise_Old_Setpoint)){
    if(Active){
      tft.fillRoundRect(20, 190, 170, 40 , 10, ILI9341_GREEN );
      tft.setFont(&FreeSerif18pt7b);
      tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(1);
    }
    if(!Active){
      tft.fillRoundRect(20, 190, 170, 40 , 10, tft.color565(160, 163,140) );
      tft.setFont(&FreeSerif18pt7b);
      tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(1);
    }    
    
    if(Control_Mode > 0){
      tft.setFont(&FreeSerif9pt7b);
      tft.setCursor(40, 215);
      tft.println("CRUISE");
      tft.setFont(&FreeSerifBold18pt7b);
      tft.setCursor(110, 220);
      tft.println(MPH_SP,1);
    }
    else{
      tft.setFont(&FreeSerif9pt7b);
      tft.setCursor(40, 215);
      tft.println("CRUISE");
      tft.setFont(&FreeSerifBold18pt7b);
      tft.setCursor(110, 215);
      tft.println("----");
    }    
    Cruise_Old_Active = Active;
    Cruise_Old_Setpoint = MPH_SP;
  }
  //tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, ILI9341_BLACK);
}

void Mode_Box(int Mode){  
  if(Mode != Old_Mode){
    tft.fillRoundRect(20, 10, 170, 40 , 10, tft.color565(160, 163,140));
    tft.setFont(&FreeSerif18pt7b);
    tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(1);

    
    if(Mode == 0){
      tft.setCursor(75, 40);
      tft.println("OFF");
    }
    if(Mode == 1){
      tft.setCursor(75, 40); 
      tft.println("Surf");
    }
    if(Mode == 2){
      tft.setCursor(20, 40); 
      tft.println("WakeBoard");
    }
    if(Mode == 3){
      tft.setCursor(80, 40); 
      tft.println("Ski");    
    }
    Cruise_Box(Active,Default_Speeds[Control_Mode]);
    Old_Mode = Mode;
  }
}

void Up_Arrow(bool touched){
  if(touched) old_down_millis = millis();
  int x_coordinate  = 250;
  int y_coordinate  = 100;
  int height        = 25;
  int length        = 50;
  if((millis() - old_down_millis) < 500){
    tft.fillTriangle(x_coordinate,y_coordinate, x_coordinate + (length/2), y_coordinate - height, x_coordinate + length, y_coordinate,
      ILI9341_GREEN );
  }
  else{
    tft.fillTriangle(x_coordinate,y_coordinate, x_coordinate + (length/2), y_coordinate - height, x_coordinate + length, y_coordinate,
      tft.color565(160, 163,140));
  }
}
void Down_Arrow(bool touched){
  if(touched) old_up_millis = millis();    
  int x_coordinate  = 250;
  int y_coordinate  = 140;
  int height        = -25;
  int length        = 50;
  if((millis() - old_up_millis) < 500){
    tft.fillTriangle(x_coordinate,y_coordinate, x_coordinate + (length/2), y_coordinate - height, x_coordinate + length, y_coordinate,
      ILI9341_RED );
  }
  else{
    tft.fillTriangle(x_coordinate,y_coordinate, x_coordinate + (length/2), y_coordinate - height, x_coordinate + length, y_coordinate,
      tft.color565(160, 163,140));
  }
}

void Time(int hour, int minute){
  if(old_minute != minute){
    int hour10;
    int hour1;
    int minute10;
    int minute1;
    int AM_PM;
    hour = hour - 7;
    if(hour < 0 ){
      hour = 24 + hour;
    }
    if(hour > 24){
     hour = hour  - 24;
    }
    if(hour > 12) hour = hour - 12;
    char Time_Text[8];
    char Hour_Buffer[3];
    char Minute_Buffer[3];

    
    if (hour < 10) { 
      sprintf(Hour_Buffer," %d",hour);
    }
    else sprintf(Hour_Buffer,"%d",hour);
    strcpy(Time_Text, Hour_Buffer);
    strcat(Time_Text, ":");
    if (minute < 10) { 
      sprintf(Minute_Buffer,"0%d",minute);
    }
    else sprintf(Minute_Buffer,"%d",minute);
    strcat(Time_Text, Minute_Buffer);    

    
    tft.fillRoundRect(220, 10, 120, 40 , 10, ILI9341_BLACK);
    tft.setFont(&FreeSerif18pt7b);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
    tft.setCursor(230, 40);
    tft.println(Time_Text);
    old_minute = minute;
  }
}




int Scale(int x1,int x2,int y1, int value){
  return ((value* y1)/(x2-x1)) ;
}
