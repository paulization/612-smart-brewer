// Libraries
#include "Adafruit_ST7735.h" //LCD
#include "Adafruit_MPR121.h" //proximity sensor
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "elapsedMillis.h"

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

elapsedMillis timeElapsed;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
    mpuInterrupt = true;
}

// Colors
#define blackColor display.Color565(0, 0, 0)
#define whiteColor display.Color565(255, 255, 255)
#define redColor display.Color565(255, 0, 0)
#define darkBlueColor display.Color565(0, 40, 122)
int tempColor = display.Color565(0, 0, 255/2);

// States
enum kettle{NOTICE, HEAT, READY, POUR};
kettle state = NOTICE;


void setup() {

  Serial.begin(9600);
  mpuInit();
  pinMode(buttonPin, INPUT);
  pinMode(speaker, OUTPUT);
  pinMode(proximityPin, INPUT);
  display.initR(INITR_GREENTAB);
  display.fillScreen(blackColor);
  //display.setRotation(2);
}

void loop() {
  switch (state) {
    case NOTICE:
    detectPresence();
    break;

    case HEAT:
    heating();
    break;

    case READY:
    idle();
    break;

    case POUR:
    //elapsedTime();
    //drawBloomingWater();
    //drawSuggested();
    trackRoll();
    delay(1000);
    mpu.resetFIFO();
    break;
  }

}

//--------------------- NOTICE ------------------------//
void detectPresence() {
  float reading = pulseIn(proximityPin, HIGH) / 147;
  Serial.println(reading);
  if (reading < 25) {
    greeting();
    chime1();
    delay(3500);
    state = HEAT;
  }
}

void greeting() {
  display.fillScreen(darkBlueColor);
  display.setCursor(30, 40);
  display.setTextWrap(true);
  display.setTextSize(3);
  display.print("Good");
  display.setCursor(5, 80);
  display.print("Morning");
}

void chime1() {
  tone(speaker, 250, 500);
  tone(speaker, 300, 200);
  tone(speaker, 450, 800);
  delay(1501);
}

//--------------------- HEAT ------------------------//

void heating() {
  display.fillScreen(tempColor);
  int colorChange = map(temp, 20, 93, 0, 255);
  tempColor = display.Color565(colorChange, 0, 255 - colorChange);
  display.setCursor(7, 50);
  display.setTextSize(5);
  display.print(String(temp) + " C");
  delay(100);
  temp++;
  if (temp > 93) {
    delay(500);
    state = READY;
  }
}

//--------------------- READY ----------------------------//

void idle() {
  display.setCursor(20, 100);
  display.setTextSize(3);
  display.print("READY");
  if (digitalRead(buttonPin) == HIGH) {
    timeElapsed = 0;
    state = POUR;
  }
}

//--------------------- POUR ----------------------------//
void elapsedTime() {
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
  display.setCursor(35, 25);
  display.setTextSize(2);
  display.print("100ml");
}

void drawBloomingWater() {
  display.fillRect(0, 64, 64, 98, whiteColor);
  display.fillRect(0, 64, 2 * timeElapsed / 1000, 98, darkBlueColor);
}

void drawSuggested() {
  display.fillTriangle(0 + 2 * timeElapsed / 1000, 54, 5 + 2 * timeElapsed / 1000, 64, 10 + 2 * timeElapsed / 1000, 54, redColor);
}


//------------------- MPU DMP6 functions-------------------//

void mpuInit() {

  Wire.setSpeed(CLOCK_SPEED_400KHZ);
  Wire.begin();

  mpu.initialize();

  devStatus = mpu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  mpu.setXGyroOffset(68);
  mpu.setYGyroOffset(41);
  mpu.setZGyroOffset(19);
  mpu.setZAccelOffset(1614);

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
      // turn on the DMP, now that it's ready
      mpu.setDMPEnabled(true);

      attachInterrupt(D2, dmpDataReady, RISING);

      // set our DMP Ready flag so the main loop() function knows it's okay to use it
      dmpReady = true;

      // get expected DMP packet size for later comparison
      packetSize = mpu.dmpGetFIFOPacketSize();
  }
}

int trackRoll() {
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
      // reset so we can continue cleanly
      mpu.resetFIFO();
      Serial.println(F("FIFO overflow!"));

  // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if ((mpuIntStatus & 0x02) > 0) {
      // wait for correct available data length, should be a VERY short wait
      while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

      // read a packet from FIFO
      mpu.getFIFOBytes(fifoBuffer, packetSize);

      // track FIFO count here in case there is > 1 packet available
      // (this lets us immediately read more without waiting for an interrupt)
      fifoCount -= packetSize;

      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
      Serial.print("ROLL\t");
      Serial.println(ypr[2] * 180/M_PI);
      return ypr[2] * 180/M_PI;
  }
}
