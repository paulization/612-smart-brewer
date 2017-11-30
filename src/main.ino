// Libraries
#include "Adafruit_ST7735.h"
#include "MPU6050.h"

// Gyroscope
// Start button?
// Proximity sensor
// LCD
#define speaker D3

// Variables
int angle; //only one axis
int temp;
int currentTime;
int volume;

// MPU variables:
MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// States
enum submarine{NOTICE, HEAT, RINSE, POUR};
submarine state = NOTICE;


void setup() {

  Wire.begin();
  Serial.begin(9600);

  // The following line will wait until you connect to the Spark.io using serial and hit enter. This gives
  // you enough time to start capturing the data when you are ready instead of just spewing data to the UART.
  //
  // So, open a serial connection using something like:
  // screen /dev/tty.usbmodem1411 9600

}

void loop() {
  switch (state) {
    case NOTICE:
    break;

    case HEAT:
    break;

    case RINSE;
    break;

    case POUR;
    trackAngle();
    break;
  }

  trackAngle();
}

//--------------------- Proximity sensing ------------------------//


//--------------------- Gyro tracking ----------------------------//
int trackAngle() {

}

//--------------------- LCD visualization ------------------------//
void greeting() {

}

void drawPouringLine() {

}

void drawSuggested() {

}

//--------------------- Sound effects ----------------------------//

void chime1() {
  tone(speaker, 250, 500);
  delay(501);
}

void chime2() {
  tone(speaker, 300, 500);
  delay(501);
}
