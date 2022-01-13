
void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}
void Delete_File(fs::FS &fs, const char * path) { 
  File file = fs.open(path);
  if (file && !file.isDirectory()) {
    int len = file.size();
    size_t flen = len;
  }
  file.close();
  if (fs.remove(path));
  delay(100);
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    File file = fs.open(path, FILE_APPEND);
    file.print(message);
    file.close();
}
void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}


void writeFile(fs::FS &fs, const char * path, const char * message, size_t len) {
  Serial.println("Attempting to write File");
  delay(1000);
  static uint8_t buf[512];
  int i = 0;
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  while (len) {
    size_t toWrite = len;
    if (toWrite > 512) {
      toWrite = 512;
    }
    memcpy(buf, message + (i * 512), toWrite);
    file.write(buf, toWrite);
    len -= toWrite;
    i++;
  }
  Serial.println("- file written");
  file.close();
}
void Save_Parameters_File() {
  DynamicJsonBuffer JsonBuffer;
  JsonObject& root = JsonBuffer.createObject();

  JsonArray& Motor_Params = root.createNestedArray("Motor_Params");
  Motor_Params.add(Motor_Reverse_Dir);
  Motor_Params.add(Motor_Steps_Per_Rotation);
  Motor_Params.add(Motor_Speed);
  Motor_Params.add(Motor_Max_Rotation);
  
  JsonArray& Tuning_Params = root.createNestedArray("Tuning_Params");
  Tuning_Params.add(kp);
  Tuning_Params.add(ki);
  Tuning_Params.add(kd);
  Tuning_Params.add(deadband);
  Tuning_Params.add(ramp_rate);

  JsonArray& Defaults = root.createNestedArray("Defaults");
  for (int i = 0; i < 4; i++) {
    Defaults.add(Default_Speeds[i]);
  }
  
  char data_to_file[800];
  root.printTo(data_to_file);
  root.printTo(Serial);
  int len = strlen(data_to_file); 
  deleteFile(SPIFFS, "/Parameters.txt");
  writeFile(SPIFFS, "/Parameters.txt", data_to_file, 800);
}


void Read_Parameters_File(fs::FS &fs, const char * path) {
  char data_to_read[1000];
  bool Got_File = false;
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return;
  }

  if (file && !file.isDirectory()) {
    //Serial.println("- read from file:");
    int j = 0;
    while (file.available()) {
      char c = (file.read());
      data_to_read[j] = c;
      Got_File = true;
      j++;
    }
    data_to_read[j] = '\0';
  }

  file.close();

  if (Got_File) {
    Serial.println(data_to_read);
    DynamicJsonBuffer JsonBuffer;
    JsonObject& root = JsonBuffer.parseObject(data_to_read);
  
    Motor_Reverse_Dir          = root["Motor_Params"][0].as<int>();
    Motor_Steps_Per_Rotation   = root["Motor_Params"][1].as<long>();  
    Motor_Speed                = root["Motor_Params"][2].as<int>();
    Motor_Max_Rotation         = root["Motor_Params"][3].as<int>();
    
    kp        = root["Tuning_Params"][0].as<float>();
    ki        = root["Tuning_Params"][1].as<float>();
    kd        = root["Tuning_Params"][2].as<float>();
    deadband  = root["Tuning_Params"][3].as<float>();
    ramp_rate = root["Tuning_Params"][4].as<float>();

    for (int i = 0; i < 4; i++) {
      Default_Speeds[i] = root["Defaults"][i].as<float>();
    }
    Serial.println("Read File");
  }
}

















void Save_Data_File() {
  pause_log = true;
  deleteFile(SPIFFS, "/Data.txt");
  //writeDataFile(SPIFFS, "/Data.txt", "Speed,Proportional,Integral,Derivative,Output\r\n");
  
  char buf[30];
  char Data_Set[20000];
  sprintf(Data_Set, "Proportional,Integral,Derivative,Output\r\n");
  
  
  for (int i = 0; i < 599; i++) {   
    sprintf(buf, "%.1f,%.1f,%.1f,%.1f,%lu\r\n",speed_log[i],proportional_log[i], integral_log[i],derivative_log[i],output_log[i]);
    strcat(Data_Set,buf); 
    Serial.println(i);
  }
    Serial.println("Write");
  int len = strlen(Data_Set); 
  writeFile(SPIFFS, "/Data1.txt", Data_Set,len);
    Serial.print("Done");
  pause_log = false;
  
}
