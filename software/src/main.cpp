#include <Arduino.h>
#include "pinout.h"

// Setup Serial2 on RX=PA3, TX=PA2
HardwareSerial debugSerial(PA3, PA2);

// Global pointer to our manually managed Timer 1
HardwareTimer *HT = NULL;

float test_angle = 0;
float set_voltage = 3.0f;  // Safe starting voltage (adjust up to 5V if motor is stiff)
float max_voltage = 12.0f; // Your power supply voltage

// Non-blocking timer variables for logging
unsigned long lastLogTime = 0;
const unsigned long logInterval = 500;

// LED Blinky timer
unsigned long lastBlinkTime = 0;
bool ledState = false;

void setup()
{
  debugSerial.begin(115200);

  debugSerial.println("\n==================================================");
  debugSerial.println("       Remapped 6-PWM Hardware Controller         ");
  debugSerial.println("==================================================");

  // Setup Yellow LED pin as a visual heartbeat
  pinMode(LED_YELLOW, OUTPUT);

  // 1. Instantiate Timer 1 Hardware first
  debugSerial.println("[INFO] Allocating HardwareTimer(TIM1)...");
  HT = new HardwareTimer(TIM1);

  // 2. Configure 3-Phase complementary pins (High on PA8/9/10, Low on PA7/PB0/PB1)
  // This initializes the GPIO modes first.
  debugSerial.println("[INFO] Configuring TIM1 Pin Modes...");
  HT->setMode(1, TIMER_OUTPUT_COMPARE_PWM1, PA8); // Phase A High
  HT->setMode(1, TIMER_OUTPUT_COMPARE_PWM1, PA7); // Phase A Low (Remapped Complement)

  HT->setMode(2, TIMER_OUTPUT_COMPARE_PWM1, PA9); // Phase B High
  HT->setMode(2, TIMER_OUTPUT_COMPARE_PWM1, PB0); // Phase B Low (Remapped Complement)

  HT->setMode(3, TIMER_OUTPUT_COMPARE_PWM1, PA10); // Phase C High
  HT->setMode(3, TIMER_OUTPUT_COMPARE_PWM1, PB1);  // Phase C Low (Remapped Complement)

  // 3. Set Frequency to 25kHz
  HT->setOverflow(25000, HERTZ_FORMAT);

  // 4. Inject hardware Dead-Time (~1.6 microseconds) to protect MOSFETs
  debugSerial.println("[INFO] Injecting hardware Dead-Time (~1.6 microseconds)...");
  TIM1->BDTR &= ~TIM_BDTR_DTG;
  TIM1->BDTR |= 120;

  // 5. CRITICAL FIX: Enable AFIO and Apply Timer 1 Partial Remap AFTER configuring pins!
  // This ensures the library doesn't overwrite our hardware remap.
  debugSerial.println("[INFO] Routing TIM1 outputs to PA7/PB0/PB1...");
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_TIM1_PARTIAL();

  // 6. Start the Timer Clock
  debugSerial.println("[INFO] Starting TIM1 Hardware Clock...");
  HT->resume();

  debugSerial.println("[SUCCESS] Timer 1 is running in Remapped 6-PWM Mode!");
  debugSerial.println("==================================================\n");
}

void loop()
{
  // Rotate the electrical angle slowly (incrementing this number speeds it up)
  test_angle = fmod(test_angle + 0.1, TWO_PI);

  // Generate 3-phase shifted sine waves (120-degrees / 2*PI/3 rad apart)
  float pwmA = set_voltage * (sin(test_angle) + 1.0f) / 2.0f;
  float pwmB = set_voltage * (sin(test_angle + 2.094395f) + 1.0f) / 2.0f;
  float pwmC = set_voltage * (sin(test_angle + 4.188790f) + 1.0f) / 2.0f;

  // Convert target voltages into exact Duty Cycle percentages (0 to 100%)
  float dcA = (pwmA / max_voltage) * 100.0f;
  float dcB = (pwmB / max_voltage) * 100.0f;
  float dcC = (pwmC / max_voltage) * 100.0f;

  // Apply Duty Cycles directly to STM32 Timer 1 hardware registers
  HT->setCaptureCompare(1, dcA, PERCENT_COMPARE_FORMAT);
  HT->setCaptureCompare(2, dcB, PERCENT_COMPARE_FORMAT);
  HT->setCaptureCompare(3, dcC, PERCENT_COMPARE_FORMAT);

  unsigned long currentMillis = millis();

  // Non-blocking Yellow LED Blink (Every 250ms)
  // If this stops blinking, the code has frozen.
  if (currentMillis - lastBlinkTime >= 250)
  {
    lastBlinkTime = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_YELLOW, ledState ? HIGH : LOW);
  }

  // Throttled logging block (runs twice per second)
  if (currentMillis - lastLogTime >= logInterval)
  {
    lastLogTime = currentMillis;

    float angleDegrees = test_angle * (180.0f / PI);

    // --- Serial Output ---
    debugSerial.println("----------------------------------------");
    debugSerial.print("Angle: ");
    debugSerial.print(angleDegrees, 0);
    debugSerial.println(" deg");
    debugSerial.print("Duty Cycles: ");
    debugSerial.print("U=");
    debugSerial.print(dcA, 1);
    debugSerial.print("% | ");
    debugSerial.print("V=");
    debugSerial.print(dcB, 1);
    debugSerial.print("% | ");
    debugSerial.print("W=");
    debugSerial.print(dcC, 1);
    debugSerial.println("%");
  }

  delay(20);
}