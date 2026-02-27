#include <LiquidCrystal_I2C.h>

#define HALL_PIN PA0 // Pin czujnika Halla
#define BUTTON_PIN PB9
#define DEBUG_LED_RED_PIN PB5
#define DEEBUG_LED_YELLOW_PIN PB4
#define ROTATE_LEFT_PIN PB8 // turn on/off motor
#define POT_PIN PA1			// Potentiometer connected to PA1 (Analog)
#define LCD_UPDATE_MS 200	// Odświeżanie LCD
#define RPM_TIMEOUT_MS 500	// Timeout dla 0 RPM

LiquidCrystal_I2C lcd(0x27, 16, 2); // na PB6 PB7
const uint8_t MCP40D18_ADDRESS = 0x2E;

volatile uint32_t lastMicros = 0;
volatile uint32_t period = 0;
volatile bool newData = false;
bool led_state = 0;

const uint8_t MAX_SAMPLES = 4; // max 4 próbki
uint32_t periodBuf[MAX_SAMPLES];
uint8_t bufIndex = 0;
uint8_t sampleCount = 4;
uint32_t lastPulseMs = 0;

void hallISR()
{
	uint32_t now = micros();
	uint32_t diff = now - lastMicros;
	lastMicros = now;

	if (diff > 0)
	{
		period = diff;
		newData = true;
		lastPulseMs = millis();
	}
}

void setup()
{
	pinMode(HALL_PIN, INPUT_PULLUP);
	pinMode(BUTTON_PIN, INPUT_PULLDOWN);
	pinMode(ROTATE_LEFT_PIN, OUTPUT);
	pinMode(DEBUG_LED_RED_PIN, OUTPUT);
	pinMode(DEEBUG_LED_YELLOW_PIN, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, FALLING);
	pinMode(POT_PIN, INPUT); // Set PA1 as Analog input

	digitalWrite(DEBUG_LED_RED_PIN, LOW);
	delay(1);
	digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	delay(5);
	digitalWrite(DEBUG_LED_RED_PIN, LOW);

	lcd.init();
	lcd.backlight();

	// 3. VALIDATE ADDRESS 0x28 Wire.beginTransmission(pot_address);
	byte error = Wire.endTransmission();

	if (error == 0)
	{
		// SUCCESS: Address is valid!
		// Turn Yellow LED ON and keep it ON
		digitalWrite(DEEBUG_LED_YELLOW_PIN, HIGH);
	}
	else
	{
		// FAILURE: Address not found
		// Turn Red LED ON and freeze
		digitalWrite(DEBUG_LED_RED_PIN, HIGH);
		while (1)
		{
		}
	}
}

void setPot(uint8_t step)
{
	// 1. Safety Clamp: The chip only accepts 0-127
	if (step > 127)
	{
		step = 127;
	}

	// 2. Start Communication
	Wire.beginTransmission(MCP40D18_ADDRESS);

	// 3. Send Command Byte (REQUIRED for MCP40D18)
	// 0x00 tells the chip: "Write Data to Wiper Register"
	Wire.write(0x00);

	// 4. Send Data Byte (The Resistance Value)
	Wire.write(step);

	// 5. Stop Communication
	Wire.endTransmission();
}

void loop()
{
	static uint32_t lastLcdUpdate = 0;

	if (digitalRead(BUTTON_PIN) == HIGH)
	{
		digitalWrite(ROTATE_LEFT_PIN, HIGH);
	}
	else
	{
		digitalWrite(ROTATE_LEFT_PIN, LOW);
	}

	if (led_state)
	{
		led_state = 0;
		digitalWrite(DEBUG_LED_RED_PIN, LOW);
	}
	else
	{
		led_state = 1;
		digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	}

	// --- POTENTIOMETER TO PWM LOGIC ---
	int potValue = analogRead(POT_PIN);
	int pwmValue = map(potValue, 0, 1023, 0, 127);
	setPot(pwmValue);

	if (newData)
	{
		noInterrupts();
		uint32_t p = period;
		newData = false;
		interrupts();

		periodBuf[bufIndex++] = p;
		if (bufIndex >= MAX_SAMPLES)
			bufIndex = 0;
	}

	uint32_t now = millis();

	if (now - lastLcdUpdate >= LCD_UPDATE_MS)
	{
		lastLcdUpdate = now;
		uint32_t rpm = 0;

		if (now - lastPulseMs < RPM_TIMEOUT_MS)
		{
			uint32_t lastPeriod = periodBuf[(bufIndex + MAX_SAMPLES - 1) % MAX_SAMPLES];
			if (lastPeriod > 0)
			{
				uint32_t estimatedRPM = 60000000UL / lastPeriod;

				if (estimatedRPM >= 100)
				{
					if (estimatedRPM > 5000)
						sampleCount = 2;
					else
						sampleCount = 4;

					uint64_t sum = 0;
					for (uint8_t i = 0; i < sampleCount; i++)
					{
						uint8_t index = (bufIndex + MAX_SAMPLES - 1 - i) % MAX_SAMPLES;
						sum += periodBuf[index];
					}
					uint32_t avgPeriod = sum / sampleCount;
					rpm = 60000000UL / avgPeriod;
				}
				else
				{
					rpm = estimatedRPM;
				}
			}
		}

		lcd.init();
		lcd.backlight();
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("RPM:");
		lcd.setCursor(4, 0);
		lcd.print(rpm);
		lcd.setCursor(0, 1);
		lcd.print("PWM:");
		lcd.print(pwmValue);
	}
}