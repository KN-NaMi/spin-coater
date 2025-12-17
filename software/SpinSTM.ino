#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define PULSES_PER_REV 1      
#define MEASURE_INTERVAL 1000 

LiquidCrystal_I2C lcd(0x27,PB6,PB7);

volatile unsigned long pulseCount = 0;
unsigned long lastMeasureTime = 0;

void hallInterrupt() {
  pulseCount++;
}

void setup() {
  pinMode(PA0, INPUT);

  attachInterrupt(digitalPinToInterrupt(PA0), hallInterrupt, FALLING);
  
  lastMeasureTime = millis();

  lcd.init();
  lcd.backlight();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float rpm = (pulses * (60000.0 / MEASURE_INTERVAL)) / PULSES_PER_REV;

    lcd.setCursor(1,0);
    lcd.print(rpm);
    lcd.print("    ");
  
    lastMeasureTime = currentTime;
  }
}
