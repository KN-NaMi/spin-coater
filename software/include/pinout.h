#pragma once

// --- Motor Driver Pins ---

#define DRIVER_UH PA8
#define DRIVER_UL PB13
#define DRIVER_VH PA9
#define DRIVER_VL PB14
#define DRIVER_WH PA10
#define DRIVER_WL PB15

// --- Hall Effect Sensor Pins ---
#define HALL_U PA15
#define HALL_V PA11
#define HALL_W PA12

// --- User Interface & I2C ---
#define BUTTON_PIN PC15
#define POT_PIN PA0
#define I2C_SCL
#define I2C_SDA

// --- Status LEDs ---
#define LED_YELLOW PC13
#define LED_RED PC14

// --- Debug UART ---
#define UART_TX PA2
#define UART_RX PA3