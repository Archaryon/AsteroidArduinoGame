#include <LiquidCrystal.h>
#include "LedControl.h"
#include <EEPROM.h>
/*
  For LCD:
  RS is connected to A0
  E is connected to A1
  D4 is connected to 7
  D5 is connected to 6
  D6 is connected to 5
  D7 is connected to 4

  For MAX7219:
  pin 12 is connected to the DataIn
  pin 11 is connected to the CLK
  pin 10 is connected to LOAD
*/

#define V0_PIN 13  // PWN instead of POTENTIOMETER
#define PIN_LEFT 2
#define PIN_RIGHT 3

LiquidCrystal lcd(A0, A1, 7, 6, 5, 4);
LedControl lc = LedControl(12, 11, 10, 1);

struct Player {
  int score;
};

Player savedHighScore = {
  0
};

/* delay between updates of the display */
unsigned long delayTime = 1000;

int buttonStateLeft;
int buttonStateRight;
int score;

volatile unsigned long buttonPressed;
int buttonDelay = 150;

volatile bool gameOver = false;
volatile bool gameStart = false;

int tick;
int tickCounter = 1;
unsigned long now;
int ship;
int columns[] = {0, 0, 0, 0, 0, 0, 0, 0};
int randomInt;

void setup() {
  gameOver = false;
  score = 0;
  tick = 300;
  tickCounter = 1;
  ship = 3;
  now = millis();
  buttonPressed = millis();

  for (int i = 0; i < 8; i++)
    columns[i] = 0;

  EEPROM.get(0, savedHighScore);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  pinMode(V0_PIN, OUTPUT);
  analogWrite(V0_PIN, 90);

  // the zero refers to the MAX7219 number, it is zero for 1 chip
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, 1); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen

  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_LEFT), left, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_RIGHT), right, FALLING);
  lcd.clear();
}

void left()
{
  if (millis() - buttonPressed > buttonDelay)
  {
    if (ship != 0)
      ship--;
    else
      ship = 7;

    lc.clearDisplay(0);
    buttonPressed = millis();
  }

  if (gameOver == true) {
    gameOver = false;
    setup();
  }
}

void right()
{
  if (millis() - buttonPressed > buttonDelay)
  {
    if (ship != 7)
      ship++;
    else
      ship = 0;

    lc.clearDisplay(0);
    buttonPressed = millis();
  }

  if (gameOver == true) {
    gameOver = false;
    setup();
  }
}

void loop() {

  if (gameStart == false) {
    buttonStateLeft = digitalRead(PIN_LEFT);
    buttonStateRight = digitalRead(PIN_RIGHT);
    lcd.print("PressAnyForStart");
    lcd.setCursor(0, 1);
    lcd.print("HighScore = ");
    lcd.setCursor(12, 1);
    lcd.print(savedHighScore.score);
    while (buttonStateLeft == HIGH && buttonStateRight == HIGH)
    {
      buttonStateLeft = digitalRead(PIN_LEFT);
      buttonStateRight = digitalRead(PIN_RIGHT);
    }
    gameStart = true;
    lcd.clear();
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Score = ");
    lcd.setCursor(8, 0);
    lcd.print(score + 1);
    if (millis() - now > tick) { //do every tick

      // score is: how many ticks you survived
      score++;
      now = millis();
      if (tickCounter == 1) { //every 4th tick
        // make game faster over time
        tick = tick / 1.02;

        // randomly choose column
        randomInt = random(0, 8);

        // if no asteroid exists in column, create one in row 1.
        if (columns[randomInt] == 0) {
          columns[randomInt] = 1;
        }
      }

      if (tickCounter != 4)
        tickCounter++;
      else
        tickCounter = 1;

      // do for every column
      for (int i = 0; i < 8; i++) {

        if (columns[i] == 10) // delete asteroids when out of display
          columns[i] = 0;

        if (columns[i] != 0) // make asteroids fall down
          columns[i]++;
      }

      lc.clearDisplay(0);
    }

    // ship
    lc.setLed(0, ship, 7, true);

    // asteroids
    for (int i = 0; i < 8; i++) {
      if (columns[i] > 0)
        lc.setLed(0, i,  columns[i] - 2, true);
				
      lc.setLed(0, i, columns[i] - 3, true);
    }
    // Collision with asteroid
    if (columns[ship] == 10 or columns[ship] == 9) {
      lc.clearDisplay(0);

      // GameOver Animation
      for (int i = 0; i < 4; i++) {
        lc.setLed(0, ship + i, 7, true);
        lc.setLed(0, ship - i, 7, true);
        lc.setLed(0, ship + i, 7 - i, true);
        lc.setLed(0, ship - i, 7 - i, true);
        lc.setLed(0, ship, 7 - 1.5 * i   , true);

        // GameOver Sound
        unsigned long time = millis();
        while (millis() - time <= 300)  {
          tone(8, random(0, 1000));
          tone(9, random(0, 1000));
        }

        lc.clearDisplay(0);
        noTone(8);
        noTone(9);
      }

      delay(500);
      gameOver = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GAME IS OVER");
      lcd.setCursor(0, 1);
      lcd.print("YourScore = ");
      lcd.setCursor(12, 1);
      lcd.print(score);
      if (score > savedHighScore.score)
        EEPROM.put(0, score);

      while (gameOver == true) {
      }
    }
  }
}
