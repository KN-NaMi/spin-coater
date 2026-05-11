#include <SimpleFOC.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "pinout.h"

HardwareSerial Serial2(PA3, PA2);

// --- MOTOR CONFIG (8 POLES = 4 PAIRS) ---
#define POLE_PAIRS 4
#define POWER_SUPPLY_VOLTAGE 12.0
#define MOTOR_VOLTAGE_LIMIT 8.0 // Increased for more power
#define MAX_TARGET_RPM 20000.0

BLDCMotor motor = BLDCMotor(POLE_PAIRS);
BLDCDriver3PWM driver = BLDCDriver3PWM(DRIVER_IN1, DRIVER_IN2, DRIVER_IN3, DRIVER_EN);
HallSensor sensor = HallSensor(HALL_U, HALL_V, HALL_W, POLE_PAIRS);
LiquidCrystal_I2C lcd(0x27, 16, 4);

void doA() { sensor.handleA(); }
void doB() { sensor.handleB(); }
void doC() { sensor.handleC(); }

void setup()
{
// Free PB3/PB4 for Hall Sensors
#if defined(STM32F1xx) || defined(ARDUINO_ARCH_STM32F1)
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_SWJ_CFG_Msk) | AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
#endif

	Serial2.begin(115200);
	delay(2000);

	SimpleFOCDebug::enable(&Serial2);
	Serial2.println("\n--- CALIBRATION BOOST MODE ---");

	pinMode(BUTTON_PIN, INPUT_PULLDOWN);
	pinMode(POT_PIN, INPUT);
	pinMode(LED_YELLOW, OUTPUT);
	pinMode(LED_RED, OUTPUT);

	Wire.begin();
	lcd.init();
	lcd.backlight();
	lcd.print("Calibrating...");

	sensor.pullup = Pullup::USE_INTERN;
	sensor.init();
	sensor.enableInterrupts(doA, doB, doC);
	motor.linkSensor(&sensor);

	driver.voltage_power_supply = POWER_SUPPLY_VOLTAGE;
	driver.init();
	motor.linkDriver(&driver);

	// --- MOTOR SETTINGS ---
	motor.foc_modulation = FOCModulationType::SpaceVectorPWM; // Better torque
	motor.controller = MotionControlType::velocity;

	// PID Tuning - lowered P for more stable startup
	motor.PID_velocity.P = 0.1f;
	motor.PID_velocity.I = 2.0f;

	motor.voltage_limit = MOTOR_VOLTAGE_LIMIT;
	motor.velocity_limit = MAX_TARGET_RPM * 0.10472f;

	// --- ⚠️ CALIBRATION BOOST ⚠️ ---
	// If the motor still doesn't move 90 degrees, increase 8.0 to 10.0
	motor.voltage_sensor_align = 8.0;
	// Move slower and longer during calibration to ensure hall states change
	motor.velocity_index_search = 1.0;

	motor.init();
	motor.useMonitoring(Serial2);

	Serial2.println("Starting calibration now. WATCH THE MOTOR.");
	Serial2.println("It should move significantly (at least 1/4 turn).");

	int initResult = motor.initFOC();

	if (initResult == 0)
	{
		Serial2.println("❌ FOC FAILED. Motor didn't move enough.");
		lcd.clear();
		lcd.print("FAILED TO MOVE");
		while (1)
			;
	}
	else
	{
		Serial2.println("✅ FOC SUCCESS!");
		lcd.clear();
		lcd.print("FOC SUCCESS!");
	}
}

void loop()
{
	motor.loopFOC();

	static uint32_t lastDisplayUpdate = 0;

	int potValue = analogRead(POT_PIN);
	float target_rpm = map(potValue, 0, 1023, 0, MAX_TARGET_RPM);
	float target_rads = target_rpm * 0.1047198f;

	if (digitalRead(BUTTON_PIN))
	{
		digitalWrite(LED_RED, HIGH);
		motor.enable();
		motor.move(target_rads);
	}
	else
	{
		digitalWrite(LED_RED, LOW);
		target_rpm = 0;
		motor.move(0);
		motor.disable();
	}

	if (millis() - lastDisplayUpdate >= 250)
	{
		lastDisplayUpdate = millis();
		digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
		float current_rpm = motor.shaft_velocity * 9.549297f;
		float power_voltage = motor.voltage.q;

		char buf[20];
		lcd.setCursor(0, 0);
		snprintf(buf, sizeof(buf), "Cur: %5d RPM ", (int)current_rpm);
		lcd.print(buf);
		lcd.setCursor(0, 1);
		snprintf(buf, sizeof(buf), "Tgt: %5d RPM ", (int)target_rpm);
		lcd.print(buf);
		lcd.setCursor(0, 3);
		int volts = (int)abs(power_voltage);
		int fractional = (int)(abs(power_voltage) * 100) % 100;
		snprintf(buf, sizeof(buf), "Pow: %s%d.%02d V   ", (power_voltage < 0 ? "-" : ""), volts, fractional);
		lcd.print(buf);
	}
}