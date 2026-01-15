#include <Romi32U4Motors.h>  // Include the Romi motor control library
#include <Romi32U4.h>
#include <LSM6.h>
#include <Wire.h>
//      Combined average turn time: 1888 ms
//      Average time for forward motion: 3773 ms

Romi32U4Motors motors;
Romi32U4Encoders encoders;
int ratio = 1437;
float turning = 3349;
static uint8_t lastDisplayTime = 0;
LSM6 imu;
char report[120];
float forwardRightMotorSpeed = 79.6;
float gyroDriftConstant = 118.29;

void setup() {
  Wire.begin();
  if (!imu.init()){
    // Failed to detect the LSM6.
    ledRed(1);
    while(1){
      Serial.println(F("Failed to detect the LSM6."));
      delay(100);
    }
  }
  imu.enableDefault();
  // Set the gyro full scale to 1000 dps because the default
  // value is too low, and leave the other settings the same.
  imu.writeReg(LSM6::CTRL2_G, 0b10001000);
  // Set the accelerometer full scale to 16 g because the default
  // value is too low, and leave the other settings the same.
  imu.writeReg(LSM6::CTRL1_XL, 0b10000100);
  Serial.begin(9600);  // initialize Serial for debugging
  delay(1000);
}
void driftTest(float initSpeed){ // 40:39.4
  float speed = initSpeed;
  float dR, dL;
  int loops = 0;

  int16_t countsLeft = encoders.getCountsLeft();
  int16_t countsRight = encoders.getCountsRight();
  Serial.print("Initial counts: ");
  Serial.print(countsLeft);
  Serial.print(" -- ");
  Serial.println(countsRight);

  // Start motors: left is constant, right will be adjusted
  motors.setLeftSpeed(initSpeed);
  motors.setRightSpeed(speed);
  delay(100);

  lastDisplayTime = millis();
  // Get and reset initial encoder counts
  dL = encoders.getCountsAndResetLeft();
  dR = encoders.getCountsAndResetRight();
  Serial.print("Initial drift counts: ");
  Serial.print(dL);
  Serial.print(" -- ");
  Serial.println(dR);
  delay(200);

  // Adjust right motor speed until the difference between counts is less than or equal to 10
  while (loops < 100){
    if ((uint8_t)(millis() - lastDisplayTime) >= 200) {
      lastDisplayTime = millis();
      dL = encoders.getCountsAndResetLeft();
      dR = encoders.getCountsAndResetRight();
      Serial.print("Drift counts: ");
      Serial.print(dL);
      Serial.print(" -- ");
      Serial.println(dR);
      if (dR > dL)
        speed -= 0.1;  // decrease right motor speed
      else
        speed += 0.1;  // increase right motor speed
      
      // Update the right motor speed with the new value
      motors.setRightSpeed(speed);
      Serial.print("Adjusted speed: ");
      Serial.println(speed);
      loops += 1;
    }
  }
  motors.setRightSpeed(0);
  motors.setLeftSpeed(0);
  while (true){
    Serial.println(speed);
  }
}
void driftTestCW(float initSpeed){ // 40:-42
  float speed = initSpeed;
  float dR, dL;
  int loops = 0;

  int16_t countsLeft = encoders.getCountsLeft();
  int16_t countsRight = encoders.getCountsRight();
  Serial.print("Initial counts: ");
  Serial.print(countsLeft);
  Serial.print(" -- ");
  Serial.println(countsRight);

  // Start motors: left is constant, right will be adjusted (reversed)
  motors.setLeftSpeed(initSpeed);
  motors.setRightSpeed(-speed);
  delay(100);

  lastDisplayTime = millis();
  // Get and reset initial encoder counts
  dL = encoders.getCountsAndResetLeft();
  dR = encoders.getCountsAndResetRight();
  Serial.print("Initial drift counts: ");
  Serial.print(dL);
  Serial.print(" -- ");
  Serial.println(dR);
  delay(200);

  while (loops < 100){
    if ((uint8_t)(millis() - lastDisplayTime) >= 200) {
      lastDisplayTime = millis();
      dL = encoders.getCountsAndResetLeft();
      dR = encoders.getCountsAndResetRight();
      Serial.print("Drift counts: ");
      Serial.print(dL);
      Serial.print(" -- ");
      Serial.println(dR);
      if (-dR > dL)
        speed -= 0.1;  // decrease right motor speed
      else
        speed += 0.1;  // increase right motor speed
      
      // Update the right motor speed with the new value
      motors.setRightSpeed(-speed);
      Serial.print("Adjusted speed: ");
      Serial.println(speed);
      loops += 1;
    }
  }
  motors.setRightSpeed(0);
  motors.setLeftSpeed(0);
  while (true){
    Serial.println(speed);
  }
}
void driftTestACW(float initSpeed){ // -40:42.6
  float speed = initSpeed;
  float dR, dL;
  int loops = 0;

  int16_t countsLeft = encoders.getCountsLeft();
  int16_t countsRight = encoders.getCountsRight();
  Serial.print("Initial counts: ");
  Serial.print(countsLeft);
  Serial.print(" -- ");
  Serial.println(countsRight);

  // Start motors: left is reversed, right is forward
  motors.setLeftSpeed(-initSpeed);
  motors.setRightSpeed(speed);
  delay(100);

  lastDisplayTime = millis();
  // Get and reset initial encoder counts
  dL = encoders.getCountsAndResetLeft();
  dR = encoders.getCountsAndResetRight();
  Serial.print("Initial drift counts: ");
  Serial.print(dL);
  Serial.print(" -- ");
  Serial.println(dR);
  delay(200);

  while (loops < 100){
    if ((uint8_t)(millis() - lastDisplayTime) >= 200) {
      lastDisplayTime = millis();
      dL = encoders.getCountsAndResetLeft();
      dR = encoders.getCountsAndResetRight();
      Serial.print("Drift counts: ");
      Serial.print(dL);
      Serial.print(" -- ");
      Serial.println(dR);
      if (dR > -dL)
        speed -= 0.1;  // decrease right motor speed
      else
        speed += 0.1;  // increase right motor speed
      
      // Update the right motor speed with the new value
      motors.setRightSpeed(speed);
      Serial.print("Adjusted speed: ");
      Serial.println(speed);
      loops += 1;
    }
  }
  motors.setRightSpeed(0);
  motors.setLeftSpeed(0);
  while (true){
    Serial.println(speed);
  }
}
void gyroDrift(){
  int counts = 0;
  float total = 0;
  while (counts < 1000){
    imu.read();
    total += imu.g.z + gyroDriftConstant;
    counts += 1;
    Serial.println(counts);
    delay(10);
  }
  Serial.print(total / counts);
}
void goCounts(int number){ // 3266 for 50cm
  float dR = 0;
  float dL = 0;
  static uint8_t lastDisplayTimeCounts = 0;
  motors.setLeftSpeed(80);
  motors.setRightSpeed(forwardRightMotorSpeed);
  float avgCounts = 0;
  while (avgCounts < number){
    if ((uint8_t)(millis() - lastDisplayTimeCounts) >= 50) {
      lastDisplayTimeCounts = millis();
      dL = encoders.getCountsLeft();
      dR = encoders.getCountsRight();
      avgCounts = (dL + dR) / 2.0;
    }
  }
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);
  encoders.getCountsAndResetLeft();
  encoders.getCountsAndResetRight();
}
void left(){  
  motors.setLeftSpeed(-40);
  motors.setRightSpeed(42.6);
  float gyroAngleZ = 0;
  unsigned long prevTime = millis();  // Initialize once before the loop
  while (gyroAngleZ < 90){
    imu.read();
    unsigned long currTime = millis();          // Get current time inside loop
    float dt = (currTime - prevTime) / 1000.0;    // Compute elapsed time (in seconds)
    prevTime = currTime;                          // Update prevTime for next iteration
    float rawGyroZ = imu.g.z + gyroDriftConstant;
    float gyroZ_dps = rawGyroZ * (90.0 / 2571.0);
    gyroAngleZ += gyroZ_dps * dt;
    if (gyroAngleZ > 75){
      motors.setLeftSpeed(-20);
      motors.setRightSpeed(20);
    }
  }
  motors.setLeftSpeed(20);
  motors.setRightSpeed(-20);
  delay(100);
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);
}
void right(){
  motors.setLeftSpeed(40);
  motors.setRightSpeed(-42);
  float gyroAngleZ = 0;
  unsigned long prevTime = millis();
  while (gyroAngleZ > -90){
    imu.read();
    unsigned long currTime = millis();
    float dt = (currTime - prevTime) / 1000.0;
    prevTime = currTime;
    float rawGyroZ = imu.g.z + gyroDriftConstant;
    float gyroZ_dps = rawGyroZ * (90.0 / 2571.0);
    gyroAngleZ += gyroZ_dps * dt;
    //Serial.print("Gyro Angle Z: ");
    //Serial.println(gyroAngleZ, 2);
    if (gyroAngleZ < -75){
      motors.setLeftSpeed(20);
      motors.setRightSpeed(-20);}
  }
  motors.setLeftSpeed(-20);
  motors.setRightSpeed(20);
  delay(100);
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);
}
void oldForw(){
  goCounts(turning);
}
void forw(){
  float dR, dL;
  int loops = 0;

  int16_t countsLeft = encoders.getCountsLeft();
  int16_t countsRight = encoders.getCountsRight();

  motors.setLeftSpeed(80);
  motors.setRightSpeed(forwardRightMotorSpeed);
  delay(100);

  lastDisplayTime = millis();
  dL = encoders.getCountsAndResetLeft();
  dR = encoders.getCountsAndResetRight();
  loops += (dL + dR) / 2;
  delay(200);

  // Adjust right motor speed until the difference between counts is less than or equal to 10
  while (loops < (turning)){
    if ((uint8_t)(millis() - lastDisplayTime) >= 10) {
      lastDisplayTime = millis();
      dL = encoders.getCountsAndResetLeft();
      dR = encoders.getCountsAndResetRight();
      loops += (dL + dR) / 2;
      //Serial.println(loops);
      Serial.println(dR - dL);
      if (dR == dL)
        ;
      else if (dR > dL)
        forwardRightMotorSpeed -= 0.2;  // decrease right motor speed
      else
        forwardRightMotorSpeed += 0.2;  // increase right motor speed
      
      // Update the right motor speed with the new value
      motors.setRightSpeed(forwardRightMotorSpeed);
    }
  }
  motors.setRightSpeed(-40);
  motors.setLeftSpeed(-40);
  delay(50);
  motors.setRightSpeed(0);
  motors.setLeftSpeed(0);
  // while (true){
  //   Serial.println(forwardRightMotorSpeed);
  // }
}
void goCountsBack(int number){ // 3266 for 50cm
  float dR = 0;
  float dL = 0;
  static uint8_t lastDisplayTimeCounts = 0;
  motors.setLeftSpeed(-80);
  motors.setRightSpeed(-forwardRightMotorSpeed);
  float avgCounts = 0;
  while (avgCounts < number){
    if ((uint8_t)(millis() - lastDisplayTimeCounts) >= 50) {
      lastDisplayTimeCounts = millis();
      dL = encoders.getCountsLeft();
      dR = encoders.getCountsRight();
      avgCounts = (dL + dR) / 2.0;
    }
  }
  motors.setLeftSpeed(0);
  motors.setRightSpeed(0);
  encoders.getCountsAndResetLeft();
  encoders.getCountsAndResetRight();
}
void loop() {
  goCounts(222222);
  delay(500);
  left();
  forw();
  forw();
  forw();
  delay(1000);
  right();
  delay(1000);
  forw();
  forw();
  forw();
  delay(1000);
  right();
  delay(1000);
  forw();
  forw();
  forw();
  forw();
  delay(1000);
  right();
  delay(1000);
  forw();
  forw();
  forw();
  delay(1000);
  right();
  delay(1000);
  forw();
  delay(1000);
  right();
  delay(400);
  forw();
  forw();
  left();
  forw();
  while(true){
    Serial.println(forwardRightMotorSpeed);
  }
  delay(500000);
}

