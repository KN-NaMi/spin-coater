#pragma once

// --- Motor Driver Pins ---
// U H L -> Phases of the motor
// XH XL -> Low side or high side driver
// Mind that Low side (if i recall correctly) has inverted logic
#define DRIVER_UH PA8
#define DRIVER_UL PB13
#define DRIVER_VH PA9
#define DRIVER_VL PB14
#define DRIVER_WH PA10
#define DRIVER_WL PB15

// --- Hall Effect Sensor Pins ---
// Need to be configured with internal pull-up to work
// Also, if phases don't line up swap any two connections of the motor in the screw terminal
#define HALL_U PA15
#define HALL_V PA11
#define HALL_W PA12

// --- User Interface & I2C ---
#define BUTTON_PIN PA1 // The button needs pull-down to work
#define POT_PIN PA0
// I2C is I2C1
#define I2C_SCL PB8 // I haven't tested I2C but it should work. If not, try adding internal pull-up's for both pins or swapping them
#define I2C_SDA PB9

// --- Status LEDs ---
#define LED_YELLOW PC13
#define LED_RED PC14

// --- Debug UART ---
// For UART2
#define UART_TX PA2
#define UART_RX PA3
