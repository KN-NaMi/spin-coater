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
	//pinMode(PB10, INPUT_PULLUP); // SCL
	//pinMode(PB11, INPUT_PULLUP); // SDA

	attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, FALLING);
	pinMode(POT_PIN, INPUT); // Set PA1 as Analog input

	digitalWrite(DEBUG_LED_RED_PIN, LOW);
	delay(1);
	digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	delay(5);
	digitalWrite(DEBUG_LED_RED_PIN, LOW);

	lcd.init();
	lcd.backlight();
}

void loop()
{


	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("RPM:");
	lcd.setCursor(4, 0);
	lcd.print("rpm");
	lcd.setCursor(0, 1);
	lcd.print("PWM:");
	lcd.print("200");
	delay(1000);
	digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	digitalWrite(DEEBUG_LED_YELLOW_PIN, HIGH);
	delay(1000);
	digitalWrite(DEBUG_LED_RED_PIN, LOW);
	digitalWrite(DEEBUG_LED_YELLOW_PIN, LOW);
}