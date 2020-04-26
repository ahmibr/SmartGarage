#include <Servo.h>

Servo entry_servo;

const int PIN_ENTRY_SERVO = 9;
const int SERVO_SETUP_TIME = 2000;
const int ENTRY_SERVO_CLOSED_VALUE = 90;
const int ENTRY_SERVO_OPEN_VALUE = 0;

const int PIN_SPEAKER = 10;

int pushButton = 2;

bool door_is_open = false;
unsigned long last_open_time = 0;

void open_door(){
  entry_servo.write(ENTRY_SERVO_OPEN_VALUE);
  delay(SERVO_SETUP_TIME);
}

void close_door(){
  entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
  delay(SERVO_SETUP_TIME);
}

void setup_entry_servo(){
  entry_servo.attach(PIN_ENTRY_SERVO);
  entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
  door_is_open = false;
  delay(SERVO_SETUP_TIME);
}

const long OPEN_SECONDS_THRESHOLD = 3;

void play_door_alarm(){
  digitalWrite(PIN_SPEAKER, HIGH);
  delay(400);
  digitalWrite(PIN_SPEAKER, LOW);
  delay(500);
}

bool is_open_overdue(){
  if(door_is_open && (millis()-last_open_time > 1000*OPEN_SECONDS_THRESHOLD))
    return true;
  
  return false;
}

void setup() {
    setup_entry_servo();
	Serial.begin(9600);
    pinMode(pushButton, INPUT);
    pinMode(PIN_SPEAKER, OUTPUT);
}

void loop() {
  int buttonState = digitalRead(pushButton);

  if(is_open_overdue()){
    play_door_alarm();
  }
  
  if (buttonState == 1) {
      if(door_is_open)
      {
        close_door();
        door_is_open = false;
      }
      else
      {
        open_door();
        door_is_open = true;
        last_open_time = millis();
      }
      Serial.println(buttonState);
  }
  delay(1);
}