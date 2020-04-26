#include <Servo.h>
#include <IRremote.h>

Servo entry_servo;

const int PIN_ENTRY_SERVO = 9;
const int SERVO_SETUP_TIME = 2000;
const int ENTRY_SERVO_CLOSED_VALUE = 90;
const int ENTRY_SERVO_OPEN_VALUE = 0;

const int PIN_SPEAKER = 10;
const int PIN_IR = 2;
const long IR_COMMAND_VALUE = 16580863;

IRrecv ir_recv(PIN_IR);
decode_results ir_result;

bool door_is_open = false;
unsigned long last_open_time = 0;

void open_door()
{
	entry_servo.write(ENTRY_SERVO_OPEN_VALUE);
	delay(SERVO_SETUP_TIME);
}

void close_door()
{
	entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
	delay(SERVO_SETUP_TIME);
}

void setup_entry_servo()
{
	entry_servo.attach(PIN_ENTRY_SERVO);
	entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
	door_is_open = false;
	delay(SERVO_SETUP_TIME);
}

const long OPEN_DOOR_TIME_THRESHOLD = 10000;

unsigned long last_alarm_high_time = 0;
unsigned long last_alarm_low_time = 0;
bool alarm_signal_high = false;
const long ALARM_HIGH_SIGNAL_TIME = 200;
const long ALARM_LOW_SIGNAL_TIME = ALARM_HIGH_SIGNAL_TIME *1.5;

void play_door_alarm()
{
	if (alarm_signal_high)
	{
		if (millis() - last_alarm_high_time > ALARM_HIGH_SIGNAL_TIME)
		{
			digitalWrite(PIN_SPEAKER, LOW);
			last_alarm_low_time = millis();
			alarm_signal_high = false;
		}
	}
	else
	{
		if (millis() - last_alarm_low_time > ALARM_LOW_SIGNAL_TIME)
		{
			digitalWrite(PIN_SPEAKER, HIGH);
			last_alarm_high_time = millis();
			alarm_signal_high = true;
		}
	}

	delay(10);
}

void stop_door_alarm()
{
	digitalWrite(PIN_SPEAKER, LOW);
	if (alarm_signal_high)
		last_alarm_high_time = 0;
	alarm_signal_high = false;
	delay(10);
}

bool is_open_overdue()
{
	if (door_is_open && (millis() - last_open_time > OPEN_DOOR_TIME_THRESHOLD))
		return true;

	return false;
}

void setup()
{
  	Serial.begin(9600);
	setup_entry_servo();
	pinMode(PIN_SPEAKER, OUTPUT);
 	ir_recv.enableIRIn(); 
}

void loop()
{
	if (is_open_overdue())
	{
		play_door_alarm();
	}

	if (ir_recv.decode(&ir_result))
	{
              
      	Serial.println("Received from IR");
      	Serial.println(ir_result.value);
      
		if (ir_result.value == IR_COMMAND_VALUE)
		{
			if (door_is_open)
			{
				stop_door_alarm();
				close_door();
				door_is_open = false;
			}
			else
			{
				open_door();
				door_is_open = true;
				last_open_time = millis();
			}
		}
		ir_recv.resume();	// Receive the next value
	}

	delay(10);
}