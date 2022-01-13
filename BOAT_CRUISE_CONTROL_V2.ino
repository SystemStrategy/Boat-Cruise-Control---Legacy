#include <Adafruit_GPS.h>


// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h>

//Pins
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9,8,7,6,5,4);
const int stepPin = 14;
const int dirPin = 16;
const int onPin    = 2;
const int modePin  = 3;
const int upPin    = 1;
const int downPin  = 0;
const int Buzzer  = 10;

//Control Parameters
int Operator[10];
int initialization = 0;
boolean ON = 0;

int Arrows;
int Arrow_Change;
int Arrow_Val = 1;
unsigned long OLD_MILLIS_Arrows = 0;
unsigned long OLD_MILLIS_Arrows_Long = 0;
unsigned long OLD_MILLIS_MODE_Long = 0;

unsigned long Button_Time = 0;

int OnButton;
int ModeButton;
int Up_Arrow;
int Down_Arrow;

int OLD_OnButton;
int OLD_ModeButton;
int OLD_Up_Arrow;
int OLD_Down_Arrow;

int Main_Menu = 0;
int Ski_Mode = 1;
int Diagnostic1_Mode = 1;
int Diagnostic2_Mode = 1;

int OLD_ON = 9;
int MODE;
int OLD_MODE = 9;

String MODE_STR;
float temp;
int temp2;

int steps_per_rotation = 800;

unsigned long OLD_MILLIS;
float SETPOINT = 12;
float GPS_SPEED;
float GPS_SPEED_Array[10];
unsigned long OLD_MILLIS1;
long Update_Rate;
float error;
float deadband;
float Ski_Kp = 0.1;
float Ski_Ki = 1;
float Surf_Kp = 20;
float Surf_Ki = 4;
float Kp;
float Ki;
float integral;
float old_integral;
int output;
int old_output;
int max_steps = 200;
int min_steps = 0;
int step_ramp = 5;
unsigned int  steps;// stepper motor steps

unsigned long Buzzer_Timer;
int Buzzer_On = 0;

//GPS
#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);
uint32_t timer = millis();

int satellite_count;
int GPS_Fix;


void GPS_Values() {
    GPS_Fix = GPS.fix && GPS.fixquality;
    if (GPS.fix) {
      GPS_SPEED = GPS.speed * 1.15078;
    }
    else {
      GPS_SPEED = SETPOINT;
    }

     
    satellite_count = GPS.satellites;
  }


//Control FUNCTIONS
void PI_Loop(int x) {
  temp = Operator[Ski_Mode];
  SETPOINT = temp / 10;


  max_steps = steps_per_rotation / 4; //Quarter turn
  min_steps = 0;
  step_ramp = 10;

  if (Ski_Mode >= 2) {
    SETPOINT = (temp / 2);
    Kp = Ski_Kp;
    Ki = Ski_Ki;
  }
  else{
    Kp = Surf_Kp;
    Ki = Surf_Ki;    
  }
  
  if (x == 0) {
    integral = 0;
    old_integral = 0;
    output = 0;
    digitalWrite(dirPin, LOW);
  }
  else {
    error = constrain(GPS_SPEED - SETPOINT,-10,10);
    if (error > deadband) {
      error = error - deadband;
    }
    else if (error < (deadband * -1)) {
      error = error + deadband;
    }
    else error = 0;
    integral = constrain((integral + (error * Ki)), min_steps, max_steps);
    integral = constrain(integral, (old_integral - step_ramp), (old_integral + step_ramp));
    old_integral = integral;
    output = constrain((Kp * error) + integral, min_steps, max_steps);
    output = constrain(output, (old_output - step_ramp), (old_output + step_ramp));

    if ((SETPOINT - GPS_SPEED) > 5) {
      integral = 0;
      old_integral = 0;
      output = 0;
      digitalWrite(dirPin, LOW);
    }
  }

  if ((output - old_output) < 0) {
    digitalWrite(dirPin, LOW);
  }
  else digitalWrite(dirPin, HIGH);

  steps = abs(output - old_output);

  old_output = output;




  for (int x = 0; x < steps; x++) {
    digitalWrite(stepPin, HIGH);
    delay(1);
    digitalWrite(stepPin, LOW);
    delay(1);
  }
}


void initialize() {
  if (initialization == 0) {
    lcd.setCursor(5, 0);
    lcd.print("WakeSetter");
    lcd.setCursor(5, 2);
    lcd.print("Cruise Pro");
    digitalWrite(dirPin, LOW);

    for (int x = 0; x < 10; x++) {
      Operator[x] = EEPROM.read(x);
      if (Operator[x] > 250) {
        EEPROM.write(x, 0);
        Operator[x] = 0;
      }

    }

    temp = Operator[4];
    Ski_Kp = temp;
    temp = Operator[5];
    Ski_Ki = temp / 10;
    steps_per_rotation = ceil(pow(2, (Operator[6] + 1))) * 100;
    temp = Operator[7];
    deadband = temp / 20;
    temp = Operator[8];
    step_ramp = temp;

    for (int x = 0; x < (steps_per_rotation / 6); x++) {
      digitalWrite(stepPin, HIGH);
      delay(10);
      digitalWrite(stepPin, LOW);
      delay(10);

      if (x > steps_per_rotation / 10) {
        lcd.setCursor(4, 2);
        lcd.print("Surfing Time");
      }
      if (x > steps_per_rotation / 8) {
        lcd.setCursor(0, 2);
        lcd.print("Do You Need A Beer?");
      }
    }
    Ski_Mode = 1;
    initialization = 1;
    lcd.clear();
  }
}

void Digitals() {

  Arrow_Change     = 0;
  OLD_OnButton    = OnButton;
  OLD_ModeButton  = ModeButton;
  OLD_Up_Arrow    = Up_Arrow;
  OLD_Down_Arrow  = Down_Arrow;

  OnButton     = digitalRead(onPin);
  ModeButton   = digitalRead(modePin);
  Up_Arrow     =  digitalRead(upPin);
  Down_Arrow   =  digitalRead(downPin);

  if ((OnButton == 1) && (OnButton != OLD_OnButton) && ((millis() - Button_Time) > 250)) {
    ON = !ON;
    Main_Menu = 0;
    Ski_Mode = 1;
    Button_Time = millis();
    if (!ON) {
      lcd.clear();
    }
  }
  if ((ModeButton == 1) && ((millis() - OLD_MILLIS_MODE_Long) > 2000)) {
    lcd.clear();
    OLD_MILLIS_MODE_Long = millis();
    Main_Menu = Main_Menu + 1;
    if (Main_Menu > 3) Main_Menu = 0;
  }
  if ((ModeButton == 0) && (ModeButton != OLD_ModeButton) && ((millis() - Button_Time) > 250) && ((millis() - OLD_MILLIS_MODE_Long) > 1000)) {
    MODE = MODE + 1;
    Button_Time = millis();
    if (Main_Menu == 0) {
      Ski_Mode = Ski_Mode + 1;
      if (Ski_Mode > 3) Ski_Mode = 1;
    }
    else if (Main_Menu == 1) {
      Diagnostic1_Mode = Diagnostic1_Mode + 1;
      if (Diagnostic1_Mode > 5) Diagnostic1_Mode = 1;
    }
  }
  else if ((ModeButton == 0) && (OLD_ModeButton == 0) && ((millis() - OLD_MILLIS_MODE_Long) > 1000)) {
    OLD_MILLIS_MODE_Long = millis() - 1000;
  }





  if ((Up_Arrow == 1) || (Down_Arrow == 1)) {
    if (millis() > (OLD_MILLIS_Arrows + 250)) {
      OLD_Up_Arrow = 0;
      OLD_Down_Arrow = 0;
      Arrow_Val = 1;
      if (millis() > (OLD_MILLIS_Arrows_Long + 8000)) {
        Arrow_Val = 4;
      }
      else if (millis() > (OLD_MILLIS_Arrows_Long + 2000)) {
        Arrow_Val = 2;
      }
    }

  }
  else {
    OLD_MILLIS_Arrows_Long  = millis();
  }
  if ((Up_Arrow == 1) && (Up_Arrow != OLD_Up_Arrow) && ((millis() - Button_Time) > 250)) {
    Arrow_Change = 1;
    Arrows  = Arrow_Val;
    OLD_MILLIS_Arrows       = millis();
    Button_Time = millis();

  }
  if ((Down_Arrow == 1) && (Down_Arrow != OLD_Down_Arrow) && ((millis() - Button_Time) > 250)) {
    Arrow_Change = 1;
    Arrows = -1 * Arrow_Val;
    OLD_MILLIS_Arrows       = millis();
    Button_Time = millis();
  }



  if (Arrow_Change == 1) {
    Arrow_Change = 1;
    if (Main_Menu == 0) {
      Operator[Ski_Mode] = constrain(Operator[Ski_Mode] + Arrows, 0, 255);
      EEPROM.write(Ski_Mode, Operator[Ski_Mode]);
    }
    if (Main_Menu == 1) {
      int inttemp;
      inttemp = Diagnostic1_Mode + 3;
      Operator[inttemp] = constrain(Operator[inttemp] + Arrows, 0, 255);
      EEPROM.write(inttemp, (Operator[inttemp]));
    }

    if (Main_Menu == 2) {
      int inttemp;
      inttemp = Diagnostic2_Mode + 3;
      Operator[inttemp] = constrain(Operator[inttemp] + Arrows, 0, 255);
      EEPROM.write(inttemp, (Operator[inttemp]));
    }
  }
}


//LCD FUNCTIONS
void Settings() {
  int temp3;
  Digitals();
  PI_Loop(1);
  OLD_MILLIS_Arrows_Long  = millis();
  if (MODE != OLD_MODE) {
    lcd.clear();
    OLD_MODE = MODE;
  }
  if (Diagnostic1_Mode == 4) {
    temp3 = 1;
    temp2 = 10;
  }
  else if (Diagnostic1_Mode == 5) {
    temp3 = 2;
    temp2 = 10;
  }
  else {
    temp3 = Diagnostic1_Mode;
    temp2 = 0;
  }
  temp = Operator[4];
  Ski_Kp = temp;
  temp = Operator[5];
  Ski_Ki = temp / 10;
  steps_per_rotation = ceil(pow(2, (Operator[6] + 1))) * 100;

  temp = Operator[7];
  deadband = temp / 20;
  temp = Operator[8];
  step_ramp = temp;


  lcd.setCursor(0, 0);
  lcd.print("Settings 1");
  lcd.setCursor(2, 1);
  lcd.print("KP:");
  lcd.print(Kp, 1);
  lcd.print("  ");


  lcd.setCursor(12, 1);
  lcd.print("DB:");
  lcd.print(deadband, 2);
  lcd.print("  ");
  lcd.setCursor(2, 2);
  lcd.print("KI:");
  lcd.print(Ki, 1);
  lcd.print("  ");
  lcd.setCursor(12, 2);
  lcd.print("RR:");
  lcd.print(step_ramp);
  lcd.print("  ");
  lcd.setCursor(2, 3);
  lcd.print("Motor Steps:");
  lcd.print(steps_per_rotation);
  lcd.print("  ");
  lcd.setCursor(temp2, temp3);
  lcd.print("->");
}
void Settings2() {

  Digitals();
  PI_Loop(1);
  lcd.setCursor(0, 0);
  lcd.print("Control Loop");
  lcd.setCursor(0, 1);
  lcd.print("Error:");
  lcd.print(error);
  lcd.print("  ");
  lcd.setCursor(0, 2);
  lcd.print("Integral:");
  lcd.print(integral);
  lcd.print("  ");
  lcd.setCursor(0, 3);
  lcd.print("Output:");
  lcd.print(output);
  lcd.print("  ");
}

void Settings3() {

  Digitals();
  PI_Loop(1);
  lcd.setCursor(0, 0);
  lcd.print("GPS Settings");
  lcd.setCursor(0, 1);
  lcd.print("Fix:");
  lcd.print(GPS_Fix);
  lcd.print("  ");
  lcd.setCursor(0, 2);
  lcd.print("Satellites:");
  lcd.print(satellite_count);
  lcd.print("  ");
  lcd.setCursor(0, 3);
  lcd.print("MSG Rate ms:");
  lcd.print(Update_Rate);
  lcd.print("  ");
}


void PrintMode(int x) {
  lcd.setCursor(0, 0);
  if (x == 0) {
    MODE_STR = "OFF      ";
  }
  else if (x == 1) {
    MODE_STR = "SURF     ";
  }
  else if (x == 2) {
    MODE_STR = "WAKEBOARD";
  }
  else if (x == 3) {
    MODE_STR = "SKI      ";
  }
  lcd.print(MODE_STR);
}

void PrintSpeed(float x) {
  lcd.setCursor(3, 2);
  lcd.print("SPEED:");
  if (x >= 60) {
    lcd.print("ERROR");
  }
  else if (x >= 10) {
    lcd.print(x, 1);
    lcd.print("MPH");
    lcd.print(" ");
  }
  else {
    lcd.print(" ");
    lcd.print(x, 1);
    lcd.print("MPH");
    lcd.print(" ");
  }
}

void PrintSetpoint(float x) {
  lcd.setCursor(0, 3);
  if (x == -1) {
    lcd.print("                ");
  }
  else {
    lcd.print("SETPOINT:");
    if (x >= 10) {
      lcd.print(x, 1);
      lcd.print("MPH");
      lcd.print(" ");
    }
    else {
      lcd.print(" ");
      lcd.print(x, 1);
      lcd.print("MPH");
      lcd.print(" ");
    }
    if(output > 0){
      x = (output / (steps_per_rotation/2))*100;
      lcd.setCursor(18, 3);
      if (x >= 10) {
        lcd.print(x, 0);
      }
      else {
        lcd.print(" ");
        lcd.print(x, 0);
      }
    }
    else {
      lcd.print("  ");
    }
  }
}

void PrintGPS(int x) {
  lcd.setCursor(10, 0);
  if(Update_Rate > 60000){
   lcd.print("Problem   ");  
  }
  else if (x == 1 ) {
     
    lcd.print("   ");
    
    int hour;
    int minute;
    int AM_PM;
    
    hour = GPS.hour - 7;
    if(hour < 0 ){
      hour = 24 + hour;
    }
    if(hour > 24){
     hour = hour  - 24;
    }
    AM_PM = 0;
    if( hour > 12){
      AM_PM = 1;
      hour =hour - 12;      
    }
    minute = GPS.minute;
    if (hour < 10) { lcd.print(" "); }
    lcd.print(hour, DEC);
     lcd.print(':');
    if (minute < 10){ lcd.print('0'); }
    lcd.print(minute, DEC); 
    if(AM_PM) {lcd.print("PM");}    
    if(!AM_PM){lcd.print("AM");}   
    }
  else if(x == 0 ) {
    lcd.print("GPS:AQ SIG");
  }
}



void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  pinMode(onPin, INPUT);
  pinMode(modePin, INPUT);
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);

  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ); // 10 Hz update rate
  //GPS.sendCommand(PGCMD_ANTENNA);

    delay(1000);
  //
  //  // Ask for firmware version
  //  GPSSerial.println(PMTK_Q_RELEASE);
}


void loop() {

  initialize();
  char c = GPS.read();
  if (GPS.newNMEAreceived()) {
    GPS_Values();
    Update_Rate = millis() - OLD_MILLIS1;
    OLD_MILLIS1 = millis();
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  
  Digitals();
  if (millis() >= OLD_MILLIS + 500) {
    OLD_MILLIS = millis();
    if (!ON) {
      PI_Loop(0);
      PrintMode(0);
      PrintGPS(GPS_Fix);
      PrintSpeed(GPS_SPEED);
      PrintSetpoint(-1);
    }
    else if (ON && (Main_Menu == 0)) {
      PI_Loop(1);
      PrintMode(Ski_Mode);
      PrintGPS(GPS_Fix);
      PrintSpeed(GPS_SPEED);
      if(Buzzer_On){
        lcd.setCursor(0, 3);
        lcd.print("Control Active    ");               
      }
      else{        
        PrintSetpoint(SETPOINT);
      }
    }
    else if (Main_Menu == 1) {
      Settings();
    }
    else if (Main_Menu == 2) {
      Settings2();
    }

    else if (Main_Menu == 3) {
      Settings3();
    }
    else { //CONTROLLER OFF
      ON = 0;
    }
  }
}
