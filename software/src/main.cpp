#include <Arduino.h>
#include <pinout.h>

const uint16_t DELAY = 10000;

#define AL DRIVER_IN1L
#define AH DRIVER_IN1H
#define BL DRIVER_IN2L
#define BH DRIVER_IN2H
#define CL DRIVER_IN3L
#define CH DRIVER_IN3H

void allOff()
{
  // PORTD = 0x01010100;
  digitalWrite(AL, HIGH);
  digitalWrite(AH, LOW);
  digitalWrite(BL, HIGH);
  digitalWrite(BH, LOW);
  digitalWrite(CL, HIGH);
  digitalWrite(CH, LOW);
  // delayMicroseconds(DELAY);
}

void step1()
{
  allOff();
  digitalWrite(AH, HIGH);
  digitalWrite(BL, LOW);
}

void step2()
{
  allOff();
  digitalWrite(AH, HIGH);
  digitalWrite(CL, LOW);
}

void step3()
{
  allOff();
  digitalWrite(BH, HIGH);
  digitalWrite(CL, LOW);
}

void step4()
{
  allOff();
  digitalWrite(BH, HIGH);
  digitalWrite(AL, LOW);
}

void step5()
{
  allOff();
  digitalWrite(CH, HIGH);
  digitalWrite(AL, LOW);
}

void step6()
{
  allOff();
  digitalWrite(CH, HIGH);
  digitalWrite(BL, LOW);
}

void setup()
{

  pinMode(AL, OUTPUT);
  digitalWrite(AL, LOW);

  pinMode(AH, OUTPUT);
  digitalWrite(AH, LOW);

  pinMode(BL, OUTPUT);
  digitalWrite(BL, LOW);

  pinMode(BH, OUTPUT);
  digitalWrite(BH, LOW);

  pinMode(CL, OUTPUT);
  digitalWrite(CL, LOW);

  pinMode(CH, OUTPUT);
  digitalWrite(CH, LOW);

  pinMode(LED_RED, OUTPUT);

  allOff();
}

int led_multiplier = 0;
int led_red_status = 0;

void loop()
{
  allOff();
  int sensorValue = analogRead(A0);
  int stepDelay = map(sensorValue, 0, 1023, 5000, 1);
  if (led_multiplier > 1000)
  {
    led_multiplier = 0;
    if (led_red_status == 0)
    {
      led_red_status = 1;
      digitalWrite(LED_RED, 1);
    }
    else
    {
      led_red_status = 0;
      digitalWrite(LED_RED, 0);
    }
  }
  led_multiplier++;

  step1();
  delayMicroseconds(stepDelay);
  step2();
  delayMicroseconds(stepDelay);
  step3();
  delayMicroseconds(stepDelay);
  step4();
  delayMicroseconds(stepDelay);
  step5();
  delayMicroseconds(stepDelay);
  step6();
  delayMicroseconds(stepDelay);
}