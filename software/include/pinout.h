#pragma once

// --- Motor Driver Pins ( TIM1 Default ) ---
// --- Motor Driver Pins ( TIM1 Remapped ) ---
#define DRIVER_IN1H PA8	 // 1 High PWM
#define DRIVER_IN1L PA7	 // 1 Low  (Back to PA7)
#define DRIVER_IN2H PA9	 // 2 High
#define DRIVER_IN2L PB0	 // 2 Low  (Back to PB0)
#define DRIVER_IN3H PA10 // 3 High
#define DRIVER_IN3L PB1	 // 3 Low  (Back to PB1)

// --- Hall Effect Sensor Pins ---
#define HALL_U
#define HALL_V
#define HALL_W

// --- User Interface & I2C ---
#define BUTTON_PIN PA1
#define POT_PIN PA0
#define I2C_SCL PB6
#define I2C_SDA PB7

// --- Status LEDs ---
#define LED_YELLOW PB9
#define LED_RED PB8

// --- Debug UART ---
#define UART_RX PA3
#define UART_TX PA2