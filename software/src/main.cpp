#include <SimpleFOC.h>
#include "pinout.h"

BLDCDriver6PWM driver = BLDCDriver6PWM(DRIVER_UH, DRIVER_UL, DRIVER_VH, DRIVER_VL, DRIVER_WH, DRIVER_WL);
HardwareSerial Serial2(UART_RX, UART_TX);

void setup() {
  Serial2.begin(115200);
  delay(2000);

  driver.voltage_power_supply = 12;
  driver.pwm_frequency = 4000; // Lower frequency for testing, was higher breaking things
  driver.dead_zone = 0.05;
  driver.init();

  // Invert logic for PMOS -> Driver specific
  TIM1->CCER |= (TIM_CCER_CC1P | TIM_CCER_CC2P | TIM_CCER_CC3P);
  TIM1->CCER &= ~(TIM_CCER_CC1NP | TIM_CCER_CC2NP | TIM_CCER_CC3NP);

  driver.enable();
  Serial2.println("Starting Manual 6-Step Commutation...");
}

// Helper to add a manual safety gap
void safetyGap() {
  driver.setPwm(0, 0, 0);
  delayMicroseconds(500); // Massive 0.5ms safety gap
}

void loop() {
  float V = 6.0; // Test Voltage
  int step_delay = 5; // Milliseconds per step. Decrease to go faster.

  // State 1: U-High, V-Low
  driver.setPwm(V, 0, -1); // SimpleFOC notation: -1 can mean float/ground depending on version
  // For safety in 6-PWM, we'll use literal 0 for Low
  driver.setPwm(V, 0, 0);
  delay(step_delay);
  safetyGap();

  // State 2: U-High, W-Low
  driver.setPwm(V, 0, 0); // (Wait, in 6-step only 2 phases are active)
  // Let's use a cleaner manual approach:

  Serial2.println("Step 1"); driver.setPwm(V, 0, 0); delay(step_delay); safetyGap();
  Serial2.println("Step 2"); driver.setPwm(V, V, 0); delay(step_delay); safetyGap();
  Serial2.println("Step 3"); driver.setPwm(0, V, 0); delay(step_delay); safetyGap();
  Serial2.println("Step 4"); driver.setPwm(0, V, V); delay(step_delay); safetyGap();
  Serial2.println("Step 5"); driver.setPwm(0, 0, V); delay(step_delay); safetyGap();
  Serial2.println("Step 6"); driver.setPwm(V, 0, V); delay(step_delay); safetyGap();
}