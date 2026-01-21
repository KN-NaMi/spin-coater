#include <LiquidCrystal_I2C.h>

#define HALL_PIN PA0          // Pin czujnika Halla
#define LCD_UPDATE_MS 200     // Odświeżanie LCD
#define RPM_TIMEOUT_MS 500    // Timeout dla 0 RPM

LiquidCrystal_I2C lcd(0x27, 16, 2);

volatile uint32_t lastMicros = 0;
volatile uint32_t period = 0;
volatile bool newData = false;

const uint8_t MAX_SAMPLES = 4;   // przechowujemy max 4 próbki do uśredniania
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

  // Obsługa nowych impulsów
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

        // Dynamiczne uśrednianie tylko dla RPM >= 100
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
          // Dla bardzo niskich prędkości: brak średniej
          rpm = estimatedRPM;
        }
      }
    }

    // Wyświetlanie na LCD
    lcd.setCursor(4, 0);
    lcd.print("     ");   // Wyczyść poprzednią wartość
    lcd.setCursor(4, 0);
    lcd.print(rpm);
  }
}

/*

1️⃣ Początek loop()
static uint32_t lastLcdUpdate = 0;


static oznacza, że zmienna zachowuje swoją wartość między wywołaniami pętli.

lastLcdUpdate służy do kontroli częstotliwości odświeżania LCD, żeby nie odświeżać go przy każdym obrocie wału.

2️⃣ Obsługa nowych impulsów
if (newData) {
    noInterrupts();
    uint32_t p = period;
    newData = false;
    interrupts();

    periodBuf[bufIndex++] = p;
    if (bufIndex >= MAX_SAMPLES) bufIndex = 0;
}

Co tu się dzieje:

newData jest ustawiane w ISR hallISR(), kiedy czujnik Halla wykryje spadek (FALLING edge).

Wchodzimy do tej sekcji, bo pojawił się nowy impuls.

noInterrupts() / interrupts() – blokujemy przerwania na chwilę, żeby bezpiecznie skopiować wartość okresu (period) z ISR.

periodBuf[bufIndex++] = p; – zapisujemy ostatni zmierzony okres w buforze cyklicznym (ring buffer).

if (bufIndex >= MAX_SAMPLES) bufIndex = 0; – kiedy dojdziemy do końca bufora, wracamy na początek, nadpisując najstarszą wartość.

Dzięki temu zawsze mamy ostatnie MAX_SAMPLES okresów wału, które możemy wykorzystać do uśredniania.

3️⃣ Sprawdzenie czasu i odświeżanie LCD
uint32_t now = millis();

if (now - lastLcdUpdate >= LCD_UPDATE_MS) {
    lastLcdUpdate = now;

Co tu się dzieje:

now = millis() – pobieramy aktualny czas w milisekundach.

if (now - lastLcdUpdate >= LCD_UPDATE_MS) – sprawdzamy, czy minęło LCD_UPDATE_MS (200 ms) od ostatniego odświeżenia LCD.

Dzięki temu LCD nie aktualizuje się przy każdym impulsie, tylko w stałym interwale ~5 Hz, co zmniejsza migotanie.

4️⃣ Sprawdzenie, czy wał się kręci
uint32_t rpm = 0;

if (now - lastPulseMs < RPM_TIMEOUT_MS) {


lastPulseMs jest ustawiane w ISR przy każdym impulsie.

RPM_TIMEOUT_MS = 500 ms – jeśli od ostatniego impulsu minęło więcej niż 500 ms, wał stoi, więc wyświetlamy 0.

Dzięki temu nie dostaniemy "starych" wartości RPM, gdy silnik przestanie się obracać.

5️⃣ Obliczenie RPM
uint32_t lastPeriod = periodBuf[(bufIndex + MAX_SAMPLES - 1) % MAX_SAMPLES];
if (lastPeriod > 0) {
    uint32_t estimatedRPM = 60000000UL / lastPeriod;

Co tu się dzieje:

lastPeriod – ostatnia zmierzona wartość okresu impulsu w mikrosekundach (micros()).

estimatedRPM = 60 000 000 / lastPeriod

micros() daje czas w mikrosekundach, więc 60 000 000 µs = 1 minuta.

Dzieląc 60 000 000 przez okres jednego impulsu, otrzymujemy przybliżoną prędkość w RPM.

Przykład: jeśli impuls powtarza się co 1000 µs (1 ms), to RPM = 60 000 000 / 1000 = 60 000 RPM.

6️⃣ Dynamiczne uśrednianie
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

Wyjaśnienie krok po kroku:

Dla RPM ≥ 100: używamy średniej z kilku ostatnich okresów → wynik jest stabilniejszy.

5000 RPM → tylko 2 próbki, bo przy szybkich obrotach chcemy szybką aktualizację.

100–5000 RPM → 4 próbki, żeby wygładzić niewielkie wahania.

Dla RPM < 100: żadnej średniej → wyświetlamy ostatnią wartość impulsu, żeby uniknąć opóźnienia i nadmiernego wygładzania przy bardzo wolnym obrocie.

W pętli for sumujemy ostatnie sampleCount próbek z bufora cyklicznego (periodBuf).

Średnia okresu → avgPeriod → obliczamy z niej RPM.

7️⃣ Wyświetlenie RPM na LCD
lcd.setCursor(4, 0);
lcd.print("     ");   // Wyczyść poprzednią wartość
lcd.setCursor(4, 0);
lcd.print(rpm);


Najpierw czyścimy stary wynik (5 spacji).

Następnie wyświetlamy nową wartość RPM na pozycji (4,0).

Dzięki temu liczba na LCD jest zawsze aktualna i czytelna.

🔹 Podsumowanie całej pętli loop()

Zbieramy nowe impulsy z ISR i zapisujemy w buforze.

Co 200 ms sprawdzamy, czy minął czas od ostatniego odświeżenia LCD.

Sprawdzamy, czy wał się kręci (timeout 500 ms).

Obliczamy RPM:

dla wolnych obrotów (<100 RPM) – ostatnia próbka

dla szybszych obrotów – średnia z kilku próbek (dynamiczna liczba próbek w zależności od prędkości)

Wyświetlamy wynik na LCD.

*/
