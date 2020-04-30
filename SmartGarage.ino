#include <Servo.h>
#include <IRremote.h>

/***************Constants*******************/
/*    Pins     */
const int PIN_ENTRY_SERVO = 9;
const int PIN_ULTRASONIC_TRIGGER = 4;
const int PIN_ULTRASONIC_ECHO = 3;
const int PIN_SPEAKER = 10;
const int PIN_IR = 2;
const int PIN_RESET_BUTTON = 5;
/*************************************/
/*    Servo constants    */
const int SERVO_SETUP_TIME = 2000; //time needed for servo to reach a point
const int ENTRY_SERVO_CLOSED_VALUE = 90; //servo angle at which door closes
const int ENTRY_SERVO_OPEN_VALUE = 0; //servo angle at which door opens
const long OPEN_DOOR_TIME_THRESHOLD = 3000; //max time a door can still be open before trigger alarm
/*************************************/
/*    IR constants        */
const long IR_COMMAND_VALUE = 16580863; //value should be received by remote
/*************************************/
/*    Alarm constants     */
const long ACTIVE_ALARM_THRESHOLD = 2.5 * OPEN_DOOR_TIME_THRESHOLD; //max time an alarm should ring before closing door
const long ALARM_HIGH_SIGNAL_TIME = 200; //time an alarm have high signal
const long ALARM_LOW_SIGNAL_TIME = ALARM_HIGH_SIGNAL_TIME *1.5; //time an alarm have low signal
/*************************************/
/*    Ultrasonic constants     */
const int ULTRASONIC_MAX_VALUE = 300; //ultrasonic max range
/*************************************/
/*    Reset button constants     */
const int RESET_BUTTON_HOLD_TIME = 2000; //time a user should hold to reset
/*************************************/

/***************Variables*******************/

/*    Servo variables         */
Servo entry_servo; //servo variable
bool door_is_open = false;
unsigned long last_open_time = 0; //last time the door was open

/*     IR variables           */
IRrecv ir_recv(PIN_IR); //IR receiver variable
decode_results ir_result; //variable to hold reading from IR

/*    Alarm variables         */
unsigned long last_alarm_high_time = 0; //variable to hold time high time interval
unsigned long last_alarm_low_time = 0; //variable to hold time low time interval
bool alarm_signal_high = false; //variable to hold alarm state

/*    Ultrasonic variables    */
int ultrasonic_reading; //variable to hold ultrasonic reading
long ultrasonic_duration; //utility variable to calculate ultrasonic reading
bool car_in_range = false; //if car in range of ultrasonic

/*    Reset variables    */
int reset_button_state = 0; //variable to hold reset button state
long int start_pressing_time = 0; //variable to calculate reset button holding time
/***************Functions*******************/

// A function to send signal to servo to open the door
void open_door()
{
	entry_servo.write(ENTRY_SERVO_OPEN_VALUE);
	delay(SERVO_SETUP_TIME);
}

// A function to send signal to servo to close the door
void close_door()
{
	entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
	delay(SERVO_SETUP_TIME);
}

// A function to setup servo at begining
void setup_entry_servo()
{
	entry_servo.attach(PIN_ENTRY_SERVO);
	entry_servo.write(ENTRY_SERVO_CLOSED_VALUE);
	door_is_open = false;
	delay(SERVO_SETUP_TIME);
}

// A function to manage alarm sound
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

// A function to stop alarm from ringing
void stop_door_alarm()
{
	digitalWrite(PIN_SPEAKER, LOW);
	if (alarm_signal_high)
		last_alarm_high_time = 0;
	alarm_signal_high = false;
	delay(10);
}

// A function to check if door is open for more than specific time
bool is_open_overdue()
{
	if (door_is_open && (millis() - last_open_time > OPEN_DOOR_TIME_THRESHOLD))
		return true;

	return false;
}

// A function to check if alarm is rining for more than specific time
bool is_alarm_overdue()
{
	if (door_is_open && (millis() - last_open_time > ACTIVE_ALARM_THRESHOLD))
		return true;

	return false;
}

// A function to read ultrasonic reading
int read_ultrasonic()
{
	digitalWrite(PIN_ULTRASONIC_TRIGGER, LOW);
	delayMicroseconds(2);
	digitalWrite(PIN_ULTRASONIC_TRIGGER, HIGH);
	delayMicroseconds(10);
	digitalWrite(PIN_ULTRASONIC_TRIGGER, LOW);

	ultrasonic_duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH);

	// divide by two as signal time is doubled (go and back)
	// constant calculated from calibrating sensor
	return (ultrasonic_duration / 2) *0.0446;
}

// A function to reset program
void reset()
{
	stop_door_alarm();
	close_door();
	door_is_open = false;
	last_open_time = 0;
	last_alarm_high_time = 0;
	last_alarm_low_time = 0;
	alarm_signal_high = false;
	car_in_range = false;
}

void setup()
{
	Serial.begin(9600);
	setup_entry_servo();
	pinMode(PIN_SPEAKER, OUTPUT);
	pinMode(PIN_ULTRASONIC_TRIGGER, OUTPUT);
	pinMode(PIN_ULTRASONIC_ECHO, INPUT);
	pinMode(PIN_RESET_BUTTON, INPUT);
	ir_recv.enableIRIn();
}

void loop()
{

	// if reset button is pressed
	reset_button_state = digitalRead(PIN_RESET_BUTTON);
	if (reset_button_state == 1)
	{
		reset();
	}

	ultrasonic_reading = read_ultrasonic();

	// if a car detected in range
	if (ultrasonic_reading < ULTRASONIC_MAX_VALUE)
	{
		if (door_is_open)
		{
			car_in_range = true;
		}
	}
	else
	{
		// ultrasonic read that no object in range
		// so when it has read before that there were an object
		// that means the car was passing and it successfully passed
		// so close the door
		if (car_in_range)
		{
			stop_door_alarm();
			close_door();
			car_in_range = false;
			door_is_open = false;
		}
	}

	// if door was open for a long time (user opened door and didn't pass nor close door)
	if (is_open_overdue())
	{
		play_door_alarm();
	}

	// if alarm was ringing for a long time, close the door
	// unless a car was in servo path
	if (is_alarm_overdue() && !car_in_range)
	{
		stop_door_alarm();
		close_door();
		car_in_range = false;
		door_is_open = false;
	}

	// check if a signal was sent from IR
	if (ir_recv.decode(&ir_result))
	{
		Serial.println("Received from IR");
		Serial.println(ir_result.value);

		// if the value received is the wanted signal
		if (ir_result.value == IR_COMMAND_VALUE)
		{
			if (door_is_open)
			{
				stop_door_alarm();
				close_door();
				car_in_range = false;
				door_is_open = false;
			}
			else
			{
				open_door();
				door_is_open = true;
				last_open_time = millis();
			}
		}
		
		// Receive the next value
		// required to reset IR sensor
		ir_recv.resume();	
	}

	delay(50);
}