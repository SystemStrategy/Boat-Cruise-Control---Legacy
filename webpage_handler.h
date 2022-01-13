void handleRoot() {  
  String s = MAIN_page; //Read HTML contents
  unsigned long GPS_Total_Time = 0;
  int GPS_Min = 1000;
  int GPS_Max = 0;
  for (int i = 0; i < 299; i++) {
    GPS_Total_Time = GPS_Total_Time + GPS_Update_Ms[i];
    GPS_Min = min(GPS_Min,GPS_Update_Ms[i]);
    GPS_Max = max(GPS_Max,GPS_Update_Ms[i]);
  }
  GPS_Hz = (GPS_Total_Time/300);
  s.replace("@@gpsfix@@",         String((int)GPS.fix));
  s.replace("@@gpsspeed@@",       String(Speed));
  s.replace("@@motorposition@@",  String(Current_Output));
  s.replace("@@prop@@",     String(proportional,2));
  s.replace("@@integ@@",     String(integral,2));
  s.replace("@@deriv@@",     String(derivative,2));
  
  if(Motor_Reverse_Dir) s.replace("@@MRev@@", "checked");
  s.replace("@@MSteps@@",   String(Motor_Steps_Per_Rotation));
  s.replace("@@MSpeed@@",   String(Motor_Speed));
  s.replace("@@Mmax@@",     String(Motor_Max_Rotation));
  s.replace("@@gpshz@@",    String(GPS_Hz));
  s.replace("@@gpsmin@@",    String(GPS_Min));
  s.replace("@@gpsmax@@",    String(GPS_Max));
  s.replace("@@kp@@",       String(kp,2));
  s.replace("@@ki@@",       String(ki,2));
  s.replace("@@kd@@",       String(kd,2));
  s.replace("@@deadband@@", String(deadband,2));
  s.replace("@@ramp@@",     String(ramp_rate,2));
  s.replace("@@Default1@@",     String(Default_Speeds[1],2));
  s.replace("@@Default2@@",     String(Default_Speeds[2],2));
  s.replace("@@Default3@@",     String(Default_Speeds[3],2));
  server.send(200, "text/html", s); //Send web page
}




void motor_params(){    
  Motor_Reverse_Dir         = server.arg("MRev").toInt();
  Motor_Steps_Per_Rotation  = server.arg("MSteps").toInt();
  Motor_Speed               = server.arg("MSpeed").toInt();
  Motor_Max_Rotation        = server.arg("Mmax").toInt();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated-- Press Back Button");
}

void tuning_params(){    
  kp        = server.arg("kp").toFloat();
  ki        = server.arg("ki").toFloat();
  kd        = server.arg("kd").toFloat();
  deadband  = server.arg("deadband").toFloat();
  ramp_rate = server.arg("ramp").toFloat();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated-- Press Back Button");
}
void defaults_params(){
  Default_Speeds[1] = server.arg("Default1").toFloat();
  Default_Speeds[2] = server.arg("Default2").toFloat();
  Default_Speeds[3] = server.arg("Default3").toFloat();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated-- Press Back Button");
}

void DownloadFile(String filename){
  if (File_Avaliable) { 
    File download = SPIFFS.open("/"+filename,  "r");
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    }
  }
}
void Save_File_Prog(){  
  Save_Parameters_File();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Updated-- Press Back Button");  
}
void handleSystemUpload(){ // upload a new file to the Filing system
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          server.sendHeader("Location", "/");
          server.send(302, "text/plain", "Updated-- Press Back Button");
          delay(2000);
        } else {
          Update.printError(Serial);
        }
      }
      delay(1000);
      ESP.restart();    
}

void Start_Webpage_Server(){    //Check all the webpages for valid credentials
    server.on("/", []() {
      Serial.println("Webpage Access Request");
      handleRoot();
    });
      
    server.on("/motor_params", []() {
      motor_params();      //form action is handled here
    });
    server.on("/tuning_params", []() {
      tuning_params();      //form action is handled here
    });
    server.on("/defaults", []() {
      defaults_params();      //form action is handled here
    });
    server.on("/save", []() {
      Save_File_Prog();      //form action is handled here
    });
    server.on("/download_data", []() { 
      Save_Data_File();
      DownloadFile("Data.txt");
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin()) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      } else {
        Serial.printf("Update Failed Unexpectedly (likely broken connection): status=%d\n", upload.status);
      }  // Route for root / web page
    });

    server.begin();
}
