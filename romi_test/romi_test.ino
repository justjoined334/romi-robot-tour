#include <Arduino.h>
#include "Chassis.h"
#include "Romi32U4Buttons.h"
#include "Romi32U4.h"


// encoder count targets, tune by turning 16 times and changing numbers untill offset is 0
#define NIGHTY_LEFT_TURN_COUNT -716
#define NIGHTY_RIGHT_TURN_COUNT 716
#define BUZZER_PIN 6

// F and B go forward/backwards 50 cm by default, but other distances can be easily specified by adding a number after the letter
// S and E go the start/end distance
// L and R are left and right
// targetTime is target time (duh)
char moves[200] = "R R R R L L L L l l l l r r r r";
double targetTime = 12;
double endDist = 41;
double startDist = -16;
double driftConst = 94.4; // also the default value (P.S. you don't really need to press C if in a rush!)
double turnConst = 0.04; // default value (P.S. very inaccurate)
int turnTimes = 0;
double turnTotal = 0.0;
bool initialized = false;


// parameters are wheel diam, encoder counts, wheel track (tune these to your own hardware)
// default values of 7, 1440, 14 can't go wrong
Chassis chassis(6.994936972, 1440, 14.0081);
Romi32U4ButtonA buttonA;
Romi32U4ButtonC buttonC;

// define the states (I LOVE state machines) (I made the state machine for Jacob's flappy bird in desmos)
// this state machine is not actually useful in any way
enum ROBOT_STATE { ROBOT_IDLE,
                   ROBOT_MOVE,
                   MOVING };
ROBOT_STATE robotState = ROBOT_IDLE;


// a helper function to stop the motors
void idle(void) {
  Serial.println("idle()");
  chassis.idle();
  robotState = ROBOT_IDLE;
}

/*
 * This is the standard setup function that is called when the board is rebooted
 * It is used to initialize anything that needs to be done once.
 */
void setup() {
  // This will initialize the Serial at a baud rate of 115200 for prints
  // Be sure to set your Serial Monitor appropriately
  Serial.begin(115200);
  // Serial1 is used to receive data from K210

  // initialize the chassis (which also initializes the motors)
  chassis.init();
  Serial.println("hello??");
  
  idle();

  chassis.setMotorPIDcoeffs(5, 0.5);
}

void glissando(int fStart, int fEnd, int durationMs, uint8_t volume = 4) {
  pinMode(BUZZER_PIN, OUTPUT);

  unsigned long start = micros();
  unsigned long durationUs = (unsigned long)durationMs * 1000UL;

  while ((micros() - start) < durationUs) {
    unsigned long t = micros() - start;

    // Linear frequency interpolation
    float frac = (float)t / (float)durationUs;
    float f = fStart + (fEnd - fStart) * frac;

    // Convert freq to period (us)
    unsigned int periodUs = (unsigned int)(1000000.0f / f);
    if (periodUs < 20) periodUs = 20; // safety clamp

    // Duty cycle controls loudness
    unsigned int onUs = volume * 3;   // tweak: smaller = quieter
    if (onUs > periodUs - 5) onUs = periodUs - 5;

    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(onUs);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(periodUs - onUs);
  }
}

void turnLeft() {
  chassis.turnFor(89, 60);
  delay(100);
}

void turnRight() {
  chassis.turnFor(-89, 60);
  delay(100);
}

void left(float seconds) {
  turnTotal += chassis.turnWithTimePosPid(NIGHTY_LEFT_TURN_COUNT, seconds, driftConst);
  turnTimes += 1;
  turnConst = turnTotal / turnTimes;
}

void right(float seconds) {
  turnTotal += chassis.turnWithTimePosPid(NIGHTY_RIGHT_TURN_COUNT, seconds, driftConst);
  turnTimes += 1;
  turnConst = turnTotal / turnTimes;
}

void righty(float seconds){
  chassis.newTurningRight(seconds, turnConst, driftConst);
}

void lefty(float seconds){
  chassis.newTurningLeft(seconds, turnConst, driftConst);
}

void loop() {
  if (buttonA.getSingleDebouncedPress()) {
    delay(300); // wait a little before starting to move so it doesn't hit the pencil or smth idk
    if (!initialized){
      chassis.initIMU();
    }
    robotState = ROBOT_MOVE;
  }

  if (buttonC.getSingleDebouncedPress()) {
    unsigned int halfPeriodUs = 83;   // ~6000 Hz
    const unsigned long durationUs = 200000; // 0.2 seconds
    unsigned long start = micros();
    glissando(3400, 6800, 400, 3);
    /*for (int i = 0; i < 20; i++) {
      ledYellow(1);
      delay(50);
      ledYellow(0);
      delay(50);
    }*/
    delay(1000);
    chassis.initIMU();
    initialized = true;
    delay(1000);
    driftConst = chassis.IMUinit(); //coding at its peak
    //glissando(7000, 5000, 400, 3);
    //delay(600);
    for (int i=0; i < ((int(fabs(driftConst / 100))) % 10); i++){ // hundreds
      glissando(2000, 1500, 100, 3);
      delay(100);
      }
    delay(400);
    for (int i=0; i < ((int(fabs(driftConst / 10))) % 10); i++){ // tens
      glissando(1500, 2000, 100, 3);
      delay(100);
      }
    delay(400);
    for (int i=0; i < (int(fabs(driftConst)) % 10); i++){ // ones
      glissando(5000, 4000, 100, 3);
      delay(100);
      }
    /*for (int i = 0; i < 20; i++) {
      ledRed(1); ledYellow(0); ledGreen(1);
      delay(30);
      ledRed(0); ledYellow(1); ledGreen(1);
      delay(30);
      ledRed(1); ledYellow(1); ledGreen(1);
      delay(30);
      ledRed(0); ledYellow(0); ledGreen(0);
      delay(30);
      ledRed(1); ledYellow(0); ledGreen(0);
      delay(30);
      ledRed(0); ledYellow(1); ledGreen(0);
      delay(30);}*/
    ledRed(0); ledYellow(0); ledGreen(0);  // ensure all off at end
  }

  if (robotState == ROBOT_MOVE) {
    int count = 1; // count the number of moves (turns and straights)
    for (int i = 0; i < strlen(moves); i++)
      if (isSpace(moves[i])) count++;

    // constucts *movesList, each element is pointer to the first character of a move string
    // i.e. if moves is "S R F100 B L E" then *movesList[2] is a pointer to "F" and moveslist[2] is "F100"
    char *movesList[count];
    char *ptr = NULL;

    // tokenize moves with space as delimiter, each token is one move
    byte index = 0;
    ptr = strtok(moves, " ");
    while (ptr != NULL) {
      movesList[index] = ptr;
      index++;
      ptr = strtok(NULL, " ");
    }

    int numTurns = 0;
    int numGTurns = 0;
    double totalDist = 0;
    char currentChar;
    String st;

    // count number of turns and total distance travelled
    // instead of *movesList[i] I could've just done st[0]... but pointers are cool ig
    for (int i = 0; i < count; i++) {
      currentChar = *movesList[i];
      st = movesList[i];
      if (currentChar == 'R' || currentChar == 'L') {
        numTurns++;
      } else if (currentChar == 'r' || currentChar == 'l') {
        numGTurns++;
      }
      else if (currentChar == 'F' || currentChar == 'B') {   
        if (st.length() > 1) {
          totalDist += st.substring(1).toDouble();
        } else {
          totalDist += 50;
        }
      } else if (currentChar == 'S') {
        totalDist += abs(startDist);
      } else if (currentChar == 'E') {
        totalDist += abs(endDist);
      }
    }

    double turnTime = 0.6; // target time for a turn is 0.55 seconds
    double totalTurnTime = 0.8 * numTurns; // just trust me
    double GturnTime = 1.0; // target time for a turn is 0.55 seconds
    double GtotalTurnTime = 3.0 * numGTurns; // just trust me
    double totalDriveTime = targetTime - totalTurnTime - GtotalTurnTime - 0.0029*totalDist; // this also always went over hence the 0.0029*totalDist
    double dist;
    unsigned long it = millis(); // measures initial time

    // execute the moves (this really should've been a switch case kind of thing)
    for (int i = 0; i < count; i++) {
      currentChar = *movesList[i];
      st = movesList[i];

      if (currentChar == 'R') {
        right(turnTime);
      } else if (currentChar == 'L') {
        left(turnTime);
      } else if (currentChar == 'r') {
        righty(GturnTime);
      } else if (currentChar == 'l') {
        lefty(GturnTime);
      }
      else if (currentChar == 'F' || currentChar == 'B') {      
        if (st.length() > 1) {
          dist = st.substring(1).toDouble();
        } else {
          dist = 50;
        }
        if (currentChar == 'F') {
          chassis.driveWithTime(dist, dist/totalDist * totalDriveTime);
        } else {
          chassis.driveWithTime(0-dist, dist/totalDist * totalDriveTime);
        } 
      } else if (currentChar == 'S') {
        chassis.driveWithTime(startDist, abs(startDist)/totalDist * totalDriveTime);
      } else if (currentChar == 'E') {
        chassis.driveWithTime(endDist, abs(endDist)/totalDist * totalDriveTime);
      }
    }
    unsigned long ft = millis(); // measures final time
    idle(); // go back to idling after finish
    /*while (true){
      Serial.print(String(ft-it) + " . ");
    }*/
  }
}