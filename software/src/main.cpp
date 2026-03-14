#include "lcd.h"
#include "pid.h"

#define HALL_PIN PA0 // Pin czujnika Halla
#define BUTTON_PIN PB9
#define DEBUG_LED_RED_PIN PB5
#define DEEBUG_LED_YELLOW_PIN PB4
#define ROTATE_LEFT_PIN PB8 // turn on/off motor
#define POT_PIN PA4			// Potentiometer connected to PA1 (Analog)
#define LCD_UPDATE_MS 500	// Odświeżanie LCD
#define RPM_TIMEOUT_MS 200	// timeout na pokazanie 0 RPM
#define RPM_UPDATE_MS 100   // co ile zmienic moc silnika
#define MCP40D18_ADDRESS 0x2e
#define HAL_INTERRUPT_DEBOUNCE_US 5000

typedef uint32_t rpm_t;

struct lcd lcd(0x27, 16, 4);

volatile uint32_t last_revolution_detection = 0;
volatile uint32_t revolution_period = 0;
volatile bool revolution_completed = false;
volatile uint32_t last_revolution_ms = 0;
struct rpm_probe {
	uint32_t rpm;
	uint8_t probe;
};
const struct rpm_probe rpm_probe_nums[] = {
/*    RPM      probes */
	{ 0,       4 },
	{ 500,     16 },
	{ 2000,    24 },
	{ 4000,    32 },
	{ 6000,    48 },
	{ 8000,    64 },
};

bool led_state = 0;

uint32_t period_buf[64];
#define PERIOD_BUFF_SIZE (sizeof(period_buf)/sizeof(*period_buf))
uint8_t buf_index = 0;

struct pid_controller pid;

void
hall_isr()
{
	uint32_t now = micros();
	uint32_t diff = now - last_revolution_detection;
	last_revolution_detection = now;

	if (diff > HAL_INTERRUPT_DEBOUNCE_US)
	{
		revolution_period = diff;
		revolution_completed = true;
		last_revolution_ms = millis();
	}
}

void
setup()
{
	Wire.begin();

	pinMode(HALL_PIN, INPUT_PULLUP);
	pinMode(BUTTON_PIN, INPUT_PULLDOWN);
	pinMode(ROTATE_LEFT_PIN, OUTPUT);
	pinMode(DEBUG_LED_RED_PIN, OUTPUT);
	pinMode(DEEBUG_LED_YELLOW_PIN, OUTPUT);
	pinMode(PB6, INPUT_PULLUP); // SCL
	pinMode(PB7, INPUT_PULLUP); // SDA

	attachInterrupt(digitalPinToInterrupt(HALL_PIN), hall_isr, FALLING);
	pinMode(POT_PIN, INPUT); // Set PA1 as Analog input
	memset(period_buf, 0, sizeof(period_buf));

	digitalWrite(DEBUG_LED_RED_PIN, LOW);
	delay(1);
	digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	delay(5);
	digitalWrite(DEBUG_LED_RED_PIN, LOW);

	init_lcd(&lcd, 16, 4);

	pid = init_pid((double)RPM_UPDATE_MS / 1000, 0.002, 0.01, 0.0005);
}

void
setPot(int step)
{
	// 1. Safety Clamp: The chip only accepts 0-127
	if (step > 127)
	{
		step = 127;
	}
	if(step < 0) {
		step = 0;
	}
	step = 127 - step;

	// 2. Start Communication
	Wire.beginTransmission(MCP40D18_ADDRESS);

	// 3. Send Command Byte (REQUIRED for MCP40D18)
	// 0x00 tells the chip: "Write Data to Wiper Register"
	Wire.write(0x00);

	// 4. Send Data Byte (The Resistance Value)
	Wire.write((uint8_t)step);

	// 5. Stop Communication
	Wire.endTransmission();
}

uint32_t
to_2_sig_digits(uint32_t num)
{
	if(num < 100)     return num;
	if(num < 1000)    return num - num%10;
	if(num < 10000)   return num - num%100;
	if(num < 100000)  return num - num%1000;
	if(num < 1000000) return num - num%10000;
}

uint32_t
get_rpm()
{
	static uint32_t prev_rpm = 0;
	uint32_t rpm = 0;
	uint32_t now = millis();
	uint8_t i;
	uint8_t num_probes;
	uint64_t probes_sum;
	uint8_t probe_index;
	uint32_t avg_period;

	if (now - last_revolution_ms < RPM_TIMEOUT_MS)
	{
		probes_sum = 0;

		for(i = 0; i < sizeof(rpm_probe_nums)/sizeof(struct rpm_probe); ++i) {
			if(prev_rpm >= rpm_probe_nums[i].rpm) {
				num_probes = rpm_probe_nums[i].probe;
			} else {
				break;
			}
		}

		for (i = 0; i < num_probes; i++)
		{
			probe_index = (buf_index + PERIOD_BUFF_SIZE - 1 - i) % PERIOD_BUFF_SIZE;
			probes_sum += period_buf[probe_index];
		}
		avg_period = probes_sum / num_probes;
		if(avg_period == 0) {
			rpm = 0;
		} else {
			rpm = 60000000UL / avg_period;
		}
		prev_rpm = rpm;
	}

	return rpm;
}

void
update_lcd(rpm_t target_rpm, unsigned int pot_value, rpm_t current_rpm, int power)
{
	static uint32_t lastLcdUpdate = 0;
	uint32_t now;

	now = millis();

	if (now - lastLcdUpdate >= LCD_UPDATE_MS)
	{
		lastLcdUpdate = now;

		set_cur_lcd(&lcd, 0, 0);
		printf_lcd(&lcd, "RPM: %d", (int)to_2_sig_digits(current_rpm));
		wipe_line(&lcd);
		set_cur_lcd(&lcd, 0, 1);
		printf_lcd(&lcd, "TGT: %d (%d)", (int)to_2_sig_digits(target_rpm), (int)pot_value);
		wipe_line(&lcd);
		set_cur_lcd(&lcd, 0, 2);
		printf_lcd(&lcd, "POW: %d", (int)power);
		wipe_line(&lcd);
		set_cur_lcd(&lcd, 0, 3);
		printf_lcd(&lcd, "ERR: %d", (int)to_2_sig_digits(abs((int32_t)target_rpm-(int32_t)current_rpm)));
		wipe_line(&lcd);
		flush_lcd(&lcd);
	}
}

double
update_motor_power(rpm_t current_rpm, rpm_t target_rpm)
{
	static double power = 0;
	static uint32_t last_motor_update = 0;
	uint32_t now;

	now = millis();

	if(now - last_motor_update >= RPM_UPDATE_MS) {
		last_motor_update = now;

		power = run_pid(&pid, current_rpm, target_rpm);
		setPot(power);
	}

	return power;
}

void loop()
{
	static int prev_state = 0;
	unsigned int pot_value;
	rpm_t target_rpm;
	rpm_t current_rpm;
	double power;
	uint32_t read_period;

	// --- POTENTIOMETER TO PWM LOGIC ---
	pot_value = analogRead(POT_PIN);
	target_rpm = map(pot_value, 100, 1023, 1100, 8000);

	if (digitalRead(BUTTON_PIN) == HIGH) {
		digitalWrite(ROTATE_LEFT_PIN, HIGH);
	} else {
		digitalWrite(ROTATE_LEFT_PIN, LOW);
		target_rpm = 0;
		reset_pid(&pid);
	}

	if (led_state) {
		digitalWrite(DEBUG_LED_RED_PIN, LOW);
	} else {
		digitalWrite(DEBUG_LED_RED_PIN, HIGH);
	}
	led_state = !led_state;

	if (revolution_completed)
	{
		noInterrupts();
		read_period = revolution_period;
		revolution_completed = false;
		interrupts();

		period_buf[buf_index++] = read_period;
		if (buf_index >= PERIOD_BUFF_SIZE)
			buf_index = 0;
	}

	current_rpm = get_rpm();

	power = update_motor_power(current_rpm, target_rpm);

	update_lcd(target_rpm, pot_value, current_rpm, power);
}

