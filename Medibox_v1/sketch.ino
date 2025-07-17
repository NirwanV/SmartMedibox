#include <Arduino.h>


//include libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

//define oled parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET  -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET_DST 0

//declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//Global Variables
int days =0;
int hours =0;
int minutes =0;
int seconds =0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = {-1,-1};
int alarm_minutes[] = {-1,-1};
bool alarm_triggered[] = {false, false};

int n_notes = 8;
int C =262;
int D =294;
int E =330;
int F =349;
int G =392;
int A =440;
int B =494;
int C_H =523;
int notes[] = {C,D,E,F,G,A,B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time", "2 - Set Alarm 1", "3- Set Alarm 2", "4- Disable Alarm", " 5 - View Alarms "};

void print_line(String text, int column, int row, int text_size){
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);

  display.display();
}

int set_utc_offset() {
  int hours = 0;
  int minutes = 0;
  bool isNegative = false;
  bool settingHours = true;

  while (true) {
    display.clearDisplay();
    print_line("Set UTC Offset", 10, 0, 1);

    String sign = isNegative ? "-" : "+";

    if (settingHours) {
      print_line("Hours: " + sign + String(hours), 30, 20, 1);
      print_line("OK: Next | Cancel: +/-", 0, 50, 1);
    } else {
      print_line("Minutes: " + String(minutes), 30, 20, 1);
      print_line("OK: Confirm | Cancel: +/-", 0, 50, 1);
    }

    display.display();

    if (digitalRead(PB_UP) == LOW) {
      if (settingHours && hours < 14) {
        hours++;
      } else if (!settingHours && minutes < 45) {
        minutes += 15;
      }
      delay(1000);
    }

    if (digitalRead(PB_DOWN) == LOW) {
      if (settingHours && hours > 0) {
        hours--;
      } else if (!settingHours && minutes > 0) {
        minutes -= 15;
      }
      delay(200);
    }

    if (digitalRead(PB_CANCEL) == LOW) {
      isNegative = !isNegative;  // Toggle sign
      delay(300);
    }

    if (digitalRead(PB_OK) == LOW) {
      delay(300);
      if (settingHours) {
        settingHours = false;  // Switch to minute setting
      } else {
        // Final confirmation: calculate total offset in seconds
        int totalSeconds = (hours * 3600) + (minutes * 60);
        if (isNegative) totalSeconds *= -1;

        display.clearDisplay();
        print_line("UTC Offset Set!", 10, 20, 2);
        delay(1000);
        return totalSeconds;
      }
    }
  }
}

void setup() {
  //Put the setup code here to run once
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(9600);

  //SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation faild"));
    for(;;);
  }
  
  //Show the display buffer on the screen. You MUST call display() after
  //drawing commands to make them visible on screen!
  display.display();
  delay(2000);


  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED){
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WiFi", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to WiFi", 0, 0, 2);

  int UTC_OFFSET = set_utc_offset();

  configTime(UTC_OFFSET,UTC_OFFSET_DST, NTP_SERVER);
  
  //Clear the display
  display.clearDisplay();
  
  print_line("Welcome to Medibox!", 10 ,20, 2);
  delay(500);
  display.clearDisplay();

}

void print_time_now() {
  String timeText = String(hours) + ":" + String(minutes) + ":" + String(seconds);
  print_line(timeText, 0, 15, 1);
}

void print_date() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    print_line("Failed to get time", 0, 0, 1);
    return;
  }

  char dateString[20];
  strftime(dateString, sizeof(dateString), "%d-%m-%Y", &timeinfo); // Format: DD-MM-YYYY

  print_line(dateString, 0,0,1 );
}

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%H", &timeinfo);
  days = atoi(timeDay);
}

void ring_alarm() {
  display.clearDisplay();
  print_line("Medicine Time!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  //Ring the busser
  while(!break_happened && digitalRead(PB_CANCEL) == HIGH){
    for (int i =0; i<n_notes; i++){
      if (digitalRead(PB_OK) == LOW){
        // Snooze
        alarm_minutes[i] += 5;
        if (alarm_minutes[i] >= 60) {
          alarm_minutes[i] -= 60;
          alarm_hours[i] = (alarm_hours[i] + 1) % 24;
        }
        display.clearDisplay();
        print_line("Snoozed 5 mins", 0, 20, 2);
        delay(1000);
        break_happened = true;
        break;
      } 
      else if (digitalRead(PB_CANCEL) == LOW){
        delay(200);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }
  
  digitalWrite(LED_1, LOW);
  display.clearDisplay();

}  

void update_time_with_check_alarm(void){
  update_time();
  print_time_now();

  if(alarm_enabled == true){
    for (int i=0; i<n_alarms; i++){
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes){
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

int wait_for_button_press(){
  while (true){
    if( digitalRead(PB_UP)== LOW){
      delay(200);
      return PB_UP;
    }

    else if( digitalRead(PB_DOWN)== LOW){
      delay(200);
      return PB_DOWN;
    }  

    else if( digitalRead(PB_OK)== LOW){
      delay(200);
      return PB_OK;
    }  

    else if( digitalRead(PB_CANCEL)== LOW){
      delay(200);
      return PB_CANCEL;
    }  

    update_time();  
  }
}

void set_time() {

  int temp_hour = hours;
  while(true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour+=1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN){
      delay(200);
      temp_hour -=1;
      if (temp_hour < 0){
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK){
      delay(200);
      hours = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }

  int temp_minute = minutes;
  while(true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute+=1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN){
      delay(200);
      temp_minute -=1;
      if (temp_minute < 0){
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK){
      delay(200);
      minutes = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  delay(1000);
}

void set_alarm(int alarm){

  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour:" + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP){
      delay(200);
      temp_hour +=1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour =23;
      } 
    }

    else if (pressed == PB_OK){
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute:" + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP){
      delay(200);
      temp_minute +=1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute =59;
      } 
    }

    else if (pressed == PB_OK){
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is Set:", 0, 0, 2);
  delay(1000);
}

void delete_alarm() {
  if (n_alarms == 0) { // No alarms set
    display.clearDisplay();
    print_line("No alarms to delete", 0, 0, 1);
    delay(1500);
    return;
  }

  int alarm_index = 0;
  
  while (true) {
    display.clearDisplay();
    print_line("Delete Alarm " + String(alarm_index + 1) + "?", 0, 0, 2);
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      alarm_index = (alarm_index + 1) % n_alarms;
    } else if (pressed == PB_DOWN) {
      alarm_index = (alarm_index - 1 + n_alarms) % n_alarms;
    } else if (pressed == PB_OK) { // Confirm deletion
      for (int i = alarm_index; i < n_alarms - 1; i++) {
        alarm_hours[i] = alarm_hours[i + 1];
        alarm_minutes[i] = alarm_minutes[i + 1];
        alarm_triggered[i] = alarm_triggered[i + 1];
      }
      n_alarms--; // Reduce count
      display.clearDisplay();
      print_line("Alarm Deleted", 0, 0, 2);
      delay(1000);
      break;
    } else if (pressed == PB_CANCEL) {
      break;
    }
  }
}

void view_alarms() {
  display.clearDisplay();
  
  if (n_alarms == 0) {  // Check if there are no alarms
    print_line("No alarms set", 0, 0, 1);
  } else {
    for (int i = 0; i < n_alarms; i++) {
      print_line("Alarm " + String(i + 1) + ": " + 
                 String(alarm_hours[i]) + ":" + 
                 (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]), 
                 0, i * 10, 1);
    }
  }
  
  display.display();  // Update the display with new content
  delay(3000);
}

void run_mode(int mode){
  if (mode == 0){
    set_time();    
  } 

  else if (mode == 1 || mode == 2){
    set_alarm(mode -1);
  }

  else if (mode == 3){
     delete_alarm();
  }
  else if (mode == 4){
    view_alarms();
  }
  

}
 
void go_to_menu(){
  while (digitalRead(PB_CANCEL) == HIGH){
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    } 

    else if(pressed == PB_DOWN){
      delay(200);
      current_mode -= 1;
      if(current_mode< 0){
        current_mode = max_modes - 1;
      }
    } 

    else if(pressed == PB_OK){
      delay(200);
      run_mode(current_mode);
    }
    
    

    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    } 
  }
} 

void check_temp(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32 ){
    print_line("TEMP HIGH", 0, 35, 1);
  }
  else if (data.temperature < 24){
    print_line("TEMP LOW", 0, 35, 1);
  }
}

void check_humidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.humidity > 80){
    print_line("HUMIDITY HIGH", 0, 50, 1);
  }
  else if (data.humidity < 65){
    print_line("HUMIDITY LOW", 0,50, 1);
  }
}

void display_status() {
  // This is  the home screen 
  display.clearDisplay();
  //Print date
  print_date();
  // Display time
  print_time_now();  
  
  // Display Temperature
  check_temp();

  // Display humidity
  check_humidity();

  delay(1000);
}

void loop() {update_time();  // updates hours/minutes/seconds
  
  display_status();

  update_time_with_check_alarm(); // keeps alarm triggering
  if (digitalRead(PB_OK) == LOW) {
    delay(20);
    go_to_menu();
  }
}