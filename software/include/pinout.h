#pragma once

// --- Motor Driver Pins (OPTIMIZED FOR TIM1 PWM) ---
#define DRIVER_IN1 PA10
#define DRIVER_IN2 PA9
#define DRIVER_IN3 PA8
#define DRIVER_EN PB15

// --- Hall Effect Sensor Pins ---
#define HALL_U PB3
#define HALL_V PB4
#define HALL_W PB5

// --- User Interface & I2C ---
#define BUTTON_PIN PA5
#define POT_PIN PA4
#define I2C2_SCL PB6
#define I2C2_SDA PB7

// --- Status LEDs ---
#define LED_YELLOW PB8
#define LED_RED PB9