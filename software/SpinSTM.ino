#include <LiquidCrystal_I2C.h>

#define HALL_PIN PA0          // Pin czujnika Halla
#define LCD_UPDATE_MS 200     // Odświeżanie LCD
#define RPM_TIMEOUT_MS 500    // Timeout dla 0 RPM

LiquidCrystal_I2C lcd(0x27, 16, 2);

volatile uint32_t lastMicros = 0;
volatile uint32_t period = 0;
volatile bool newData = false;

const uint8_t MAX_SAMPLES = 4;   // max 4 próbki
uint32_t periodBuf[MAX_SAMPLES];
uint8_t bufIndex = 0;
uint8_t sampleCount = 4;         
uint32_t lastPulseMs = 0;

void hallISR() {
  uint32_t now = micros();
  uint32_t diff = now - lastMicros;
  lastMicros = now;

  if (diff > 0) {
    period = diff;
    newData = true;
    lastPulseMs = millis();
  }
}

void setup() {
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, FALLING);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RPM:");
}

void loop() {
  static uint32_t lastLcdUpdate = 0;

  if (newData) {
    noInterrupts();
    uint32_t p = period;
    newData = false;
    interrupts();

    periodBuf[bufIndex++] = p;
    if (bufIndex >= MAX_SAMPLES) bufIndex = 0;
  }

  uint32_t now = millis();

  if (now - lastLcdUpdate >= LCD_UPDATE_MS) {
    lastLcdUpdate = now;
    uint32_t rpm = 0;

    if (now - lastPulseMs < RPM_TIMEOUT_MS) {
      uint32_t lastPeriod = periodBuf[(bufIndex + MAX_SAMPLES - 1) % MAX_SAMPLES];
      if (lastPeriod > 0) {
        uint32_t estimatedRPM = 60000000UL / lastPeriod;

        if (estimatedRPM >= 100) {
          if (estimatedRPM > 5000) sampleCount = 2;
          else sampleCount = 4;

          uint64_t sum = 0;
          for (uint8_t i = 0; i < sampleCount; i++) {
            uint8_t index = (bufIndex + MAX_SAMPLES - 1 - i) % MAX_SAMPLES;
            sum += periodBuf[index];
          }
          uint32_t avgPeriod = sum / sampleCount;
          rpm = 60000000UL / avgPeriod;
        } else {
          rpm = estimatedRPM;
        }
      }
    }

    lcd.setCursor(4, 0);
    lcd.print("     ");  
    lcd.setCursor(4, 0);
    lcd.print(rpm);
  }
}