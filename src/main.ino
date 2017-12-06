// Libraries
#include "Adafruit_ST7735.h" //LCD
#include "Adafruit_MPR121.h" //proximity sensor
#include "MPU6050.h"
#include "elapsedMillis.h"
#include "math.h"

// Gyroscope
MPU6050 mpu;
// Proximity sensor
#define proximityPin D3
Adafruit_MPR121 cap = Adafruit_MPR121();
// LCD
#define CS A2
#define RST D4
#define DC D5
Adafruit_ST7735 display = Adafruit_ST7735(CS, DC, RST);
// Speaker
#define speaker A7
// Button
#define buttonPin A0

// Variables
int temp = 20;
int roll;
int pitch;
int water;
int blink;
int count = 3;
int change = 0;
int waterML = 0;
int newWater = 0;
bool alreadyStarted = false;
bool alreadyReset = false;

elapsedMillis timeElapsed;

// Colors
#define blackColor display.Color565(0, 0, 0)
#define whiteColor display.Color565(255, 255, 255)
#define redColor display.Color565(255, 0, 0)
#define darkBlueColor display.Color565(0, 40, 122)
int tempColor = display.Color565(0, 0, 128);

// States
enum kettle{STANDBY, NOTICE, HEAT, READY, BLOOM, FIRSTPOUR, SECONDPOUR, END};
kettle state = STANDBY;


void setup() {

  Wire.begin();
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  pinMode(speaker, OUTPUT);
  pinMode(proximityPin, INPUT);
  display.initR(INITR_GREENTAB);
  display.fillScreen(blackColor);
  display.setRotation(2);
}

void loop() {
  switch (state) {
    case STANDBY:
      display.setCursor(5, 100);
      display.setTextSize(3);
      display.print("STANDBY");
      if (digitalRead(buttonPin) == HIGH) {
        display.fillScreen(blackColor);
        state = NOTICE;
        delay(2000);
      };
    break;

    case NOTICE:
      detectPresence();
    break;

    case HEAT:
      heating();
    break;

    case READY:
      idle();
    break;

    case BLOOM:
      countdown();
      if (!alreadyStarted) {
        timeElapsed = 0;
        alreadyStarted = !alreadyStarted;
      }
      if (elapsedTime() >= 26) {
        count = 3;
        state = FIRSTPOUR;
      }
      drawEmpty(40);
      drawPouringWater(trackRoll());
      writeVolume();
      drawSuggested(40, 0);
      delay(1000);
    break;

    case FIRSTPOUR:
      countdown(); //fine
      resetPour();
      if (elapsedTime() >= 86) {
        count = 3;
        alreadyReset = false;
        state = SECONDPOUR;
      }
      drawEmpty(80); //fine
      drawPouringWater(trackRoll()); //fine
      writeVolume(); // fine
      drawSuggested(80, 30); // fine
      delay(1000);
    break;

    case SECONDPOUR:
      countdown();
      resetPour();
      elapsedTime();
      drawEmpty(80);
      drawPouringWater(trackRoll());
      writeVolume();
      drawSuggested(80, 90);
      delay(1000);
      if (waterML >= 200 && trackRoll() <= 10 && timeElapsed >= 130000) {
        state = END;
      }
    break;

    case END:
      writeEnd();
      System.reset();
    break;
  }

}

//--------------------- NOTICE ------------------------//
void detectPresence() {
  float reading = pulseIn(proximityPin, HIGH) / 147;
  Serial.println(reading);
  if (reading < 10) {
    greeting();
    chime1();
    delay(5000);
    fillingWater();
    delay(5000); //change time later
    state = HEAT;
  }
}

void greeting() {
  display.fillScreen(darkBlueColor);
  display.setTextWrap(true);
  display.setTextSize(3);
  display.setCursor(30, 10);
  display.print("Good");
  display.setCursor(5, 50);
  display.print("Morning");
  display.setCursor(10, 90);
  display.print("Claire");
}

void chime1() {
  tone(speaker, 250, 500);
  delay(501);
  tone(speaker, 450, 800);
  delay(801);
}

void fillingWater() {
  display.fillScreen(darkBlueColor);
  display.setTextWrap(true);
  display.setTextSize(3);
  display.setCursor(2, 50);
  display.print("Filling...");
}

//--------------------- HEAT ------------------------//

void heating() {
  display.fillScreen(tempColor);
  int colorChange = map(temp, 20, 93, 0, 255);
  tempColor = display.Color565(colorChange, 0, 255 - colorChange);
  display.setCursor(7, 50);
  display.setTextSize(5);
  display.print(String(temp) + " C");
  delay(100); //change time later
  temp++;
  if (temp > 93) {
    chime2();
    state = READY;
  }
}

void chime2() {
  tone(speaker, 550, 500);
  delay(501);
  tone(speaker, 450, 500);
  delay(501);
}

//--------------------- READY ----------------------------//

void idle() {
  display.setCursor(20, 100);
  display.setTextSize(3);
  display.print("READY");
  if (digitalRead(buttonPin) == HIGH) {
    mpu.initialize();
    state = BLOOM;
  }
}

//--------------------- POUR ----------------------------//
int elapsedTime() {
  int timeElapsedSec = (timeElapsed / 1000) % 60;
  int timeElapsedMin = (timeElapsed / 1000) / 60;
  display.fillScreen(blackColor);
  display.setCursor(35, 5);
  display.setTextSize(2);
  if (timeElapsedSec < 10) {
    display.print("0" + String(timeElapsedMin) + ":0" + String(timeElapsedSec));
  } else {
    display.print("0" + String(timeElapsedMin) + ":" + String(timeElapsedSec));
  }
  return timeElapsed / 1000;
}

void drawEmpty(int i) {
  display.fillRect(15, 128 - i, 113, 200, whiteColor);
}

// must pour 2ml/sec
void drawPouringWater(int r) {
  newWater = max(0 , ((r - 20) / 3));
  water = newWater + water;
  display.fillRect(15, 128 - water, 113, 200, darkBlueColor);
}

void writeVolume() {
  waterML = waterML + newWater;
  display.setCursor(35, 25);
  display.setTextSize(2);
  display.print(String(waterML) + "ml");
}

void drawSuggested(int i, int j) { //must cover 80 pixel within 30 secs = 2.67 pixel
  int timeElapsedSuggested = (timeElapsed / 1000) - j;
  change = min(2.67 * timeElapsedSuggested, i);
  display.fillTriangle(0, 120 - change, 0, 136 - change, 15, 128 - change, redColor);
  display.fillRect(0, 127 - change, 128, 3, redColor);
}

void countdown() {
  while (count > 0) {
    display.fillScreen(blackColor);
    display.setCursor(40, 40);
    display.setTextSize(7);
    display.print(count);
    tone(speaker, 350, 300);
    count--;
    delay(1000);
  }
}

void resetPour() {
  if (!alreadyReset) {
    water = 0;
    alreadyReset = !alreadyReset;
  }
}


//------------------- END -------------------//

void writeEnd() {
  display.fillScreen(blackColor);
  display.setTextWrap(true);
  display.setTextSize(3);
  display.setCursor(35, 30);
  display.print("Bye");
  display.setCursor(15, 70);
  display.print("Claire");
  delay(1000);
}


//------------------- MPU functions-------------------//
int trackRoll(){
  double x_Buff = mpu.getAccelerationX();
  double y_Buff = mpu.getAccelerationY();
  double z_Buff = mpu.getAccelerationZ();
  roll = atan2(y_Buff , z_Buff) * 57.3;
  pitch = atan2((- x_Buff) , sqrt(y_Buff * y_Buff + z_Buff * z_Buff)) * 57.3;
  if (roll > 90) {
    roll = 0;
  } else if (roll < 0) {
    roll = roll + 180;
  }
  Serial.println(roll);
  return roll;
}
