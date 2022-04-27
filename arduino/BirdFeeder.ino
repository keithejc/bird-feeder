/*
 Name:		BirdFeeder.ino
 Created:	2/6/2022 2:02:38 PM
 Author:	keith
*/

// the setup function runs once when you press reset or power the board
#include <Sequence.h>
#include <EasyButtonVirtual.h>
#include <EasyButtonTouch.h>
#include <EasyButtonBase.h>
#include <EasyButton.h>
#include <Servo.h>
constexpr unsigned long SERIAL_BAUD_RATE = 115200;
constexpr int SERVO_PIN = 9;
constexpr uint8_t TRIGGER_IN_PIN = 8;
#define IR_PIN1 A0
#define IR_PIN2 A1
#define IR_PIN3 A2

constexpr unsigned long sampleInterval = 10;
constexpr int DelayAfterFood = 250;
constexpr int numCalSamples = 6000;//sampleInterval * numCalSamples == ms
constexpr int rateThresholdAddition = 30;

Servo servo;
EasyButton button(TRIGGER_IN_PIN, 40, true, true);//40ms debaounce, pull-up, actve low

//fill with analog pins for ir inputs
constexpr int NumIrPins = 3;
uint8_t irPins[NumIrPins] = { IR_PIN1, IR_PIN2, IR_PIN3 };
int rateThreshold[] = { 40, 40, 40 }; //one for each ir input

int lastIrValues[NumIrPins];
unsigned long lastSampleTime;
unsigned long timeOfFoodDelivery;
constexpr uint32_t ChangeModeButtonPressedTime = 2000;

bool dispenseBait = false;
constexpr unsigned long BaitPeriod = 1800000; // 1/2hr

//single button press - do a food delivery
void buttonPress()
{
	DeliverFood();
	Serial.println("Button press");
}

//long press - change the deliver bait mode
void onButtonChangeMode()
{
	dispenseBait = !dispenseBait;

	Serial.print("Change mode. Bait delivery = ");
	Serial.println(dispenseBait);

	if (dispenseBait)
	{
		DeliverFood();
		Shake();
		Shake();
	}
	Shake();
}

void setup() {
	Serial.begin(SERIAL_BAUD_RATE);

	servo.attach(SERVO_PIN);

	//setup input
	button.begin();
	button.onPressed(buttonPress);
	button.onPressedFor(ChangeModeButtonPressedTime, onButtonChangeMode);

	//stop the servo
	ScrewStop();

	Serial.println("Initial IR Values:");
	for (size_t i = 0; i < NumIrPins; i++)
	{
		lastIrValues[i] = analogRead(irPins[i]);
		Serial.print(i);
		Serial.print(" : ");
		Serial.println(lastIrValues[i]);
	}

	lastSampleTime = millis();

	Calibrate();
}

void Calibrate()
{
	//for 10 seconds get the max rate for each ir detector
	int maxRate[NumIrPins] = { 0,0 };

	Serial.println("Calibrating");

	unsigned long startOfCal = millis();
	int numSamples = 0;
	while (numSamples < (numCalSamples))
	{
		if (millis() - lastSampleTime >= sampleInterval)   // wait for dt milliseconds
		{
			lastSampleTime = millis();

			for (size_t i = 0; i < NumIrPins; i++)
			{
				int irValue = analogRead(irPins[i]);

				//Serial.println(irValue);

				int rate = lastIrValues[i] - irValue;
				lastIrValues[i] = irValue;
				maxRate[i] = max(maxRate[i], abs(rate));
			}

			numSamples++;
		}
	}
	for (size_t i = 0; i < NumIrPins; i++)
	{
		Serial.print("Max variation ");
		Serial.print(i);
		Serial.print(" : ");
		Serial.println(maxRate[i]);

		rateThreshold[i] = maxRate[i] + rateThresholdAddition;
	}
}

void ScrewSlowForward(int time)
{
	servo.write(75);
	delay(time);
}

void ScrewSlowBackward(int time)
{
	servo.write(75);
	delay(time);
}

void ScrewFastForward(int time)
{
	servo.write(0);
	delay(time);
}

void ScrewFastBackward(int time)
{
	servo.write(180);
	delay(time);
}

void ScrewStop()
{
	servo.write(90);
}

void DeliverFood()
{
	for (size_t i = 0; i < 1; i++)
	{
		ScrewSlowForward(150);
		ScrewFastBackward(50);
	}
	ScrewStop();
	Shake();

	timeOfFoodDelivery = millis();
}

void Shake()
{
	for (size_t i = 0; i < 5; i++)
	{
		ScrewFastForward(60);
		ScrewFastBackward(50);
	}
	ScrewStop();
}

bool DetectIR()
{
	bool detected = false;

	if (millis() - lastSampleTime >= sampleInterval)   // wait for dt milliseconds
	{
		lastSampleTime = millis();

		for (size_t i = 0; i < NumIrPins; i++)
		{
			int irValue = analogRead(irPins[i]);

			//	Serial.print(i);
			//	Serial.print(" : ");
			//	Serial.println(irValue);

			int rate = lastIrValues[i] - irValue;
			lastIrValues[i] = irValue;

			if (abs(rate) > rateThreshold[i])
			{
				Serial.print(i);
				Serial.print(" : ");
				Serial.print(rate);
				Serial.print(" > ");
				Serial.print(rateThreshold[i]);
				Serial.print(" : ");
				Serial.println(millis());

				detected = true;
				break;
			}
		}
	}

	return detected;
}

// the loop function runs over and over again until power down or reset
void loop() {
	//if (IsIRTriggered() && !InTrigger)
	//{
	//	Serial.println("Trigger");

	//	InTrigger = true;
	//	DeliverFood();
	//	Shake();
	//	delay(DelayAfterFood);
	//}

	//if (!IsIRTriggered())
	//{
	//	InTrigger = false;
	//}

	//int an = analogRead(IR_PIN);
	//int val = map(an, 0, 1023, 0, 100);
	//servo.write(val);                  // sets the servo position according to the scaled value
	//Serial.print("an ");
	//Serial.println(an);
	//Serial.print("servo ");
	//Serial.println(val);

	/*delay(10);
	int val = analogRead(irPins[0]);
	Serial.print(val);
	Serial.print(" ");
	val = analogRead(irPins[1]);
	Serial.println(val);*/

	//don't deliver food if close to the last time. Gives time for ir levels to go back to steady state
	if (DetectIR() && (millis() - timeOfFoodDelivery > DelayAfterFood))
	{
		Serial.println("IR detected");
		DeliverFood();
	}

	//deliver bait periodically
	if (dispenseBait && (millis() - timeOfFoodDelivery > BaitPeriod))
	{
		Serial.println("Bait delivery");
		DeliverFood();
	}

	//read the button
	button.read();
}