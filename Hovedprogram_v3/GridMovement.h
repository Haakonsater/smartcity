/*
 * Her igjen er en del av koden hentet direkte fra eksempelkoden "MazeSolver".
 * Den koden er allerede kommentert, komenterer koden der endringer er gjort
 */

#pragma once

#include <Wire.h>

uint16_t speedLimit = 200;

// The delay to use between first detecting an intersection and
// starting to turn.  During this time, the robot drives
// straight.  Ideally this delay would be just long enough to get
// the robot from the point where it detected the intersection to
// the center of the intersection.
const uint16_t intersectionDelay = 130;

// Motor speed when turning.  400 is the max speed.
const uint16_t turnSpeed = 200;

// Motor speed when turning during line sensor calibration.
const uint16_t calibrationSpeed = 200;

// This line sensor threshold is used to detect intersections.
const uint16_t sensorThreshold = 200;

// The number of line sensors we are using.
const uint8_t numSensors = 5;

// For angles measured by the gyro, our convention is that a
// value of (1 << 29) represents 45 degrees.  This means that a
// uint32_t can represent any angle between 0 and 360.
const int32_t gyroAngle45 = 0x20000000;

int16_t lastError = 0;


// Variabel som endres etter vanskelighetsgrad på bannen
// Settes til 1 når løypen er vanskelig
int stateFollowLine = 0;

uint16_t time_now;

uint16_t lineSensorValues[numSensors];

// Takes calibrated readings of the lines sensors and stores them
// in lineSensorValues.  Also returns an estimation of the line
// position.
uint16_t readSensors()
{
  return lineSensors.readLine(lineSensorValues);
}

// Returns true if the sensor is seeing a line.
// Make sure to call readSensors() before calling this.
bool aboveLine(uint8_t sensorIndex)
{
  return lineSensorValues[sensorIndex] > sensorThreshold;
}


// Turns according to the parameter dir, which should be 'L'
// (left), 'R' (right), 'S' (straight), or 'B' (back).  We turn
// most of the way using the gyro, and then use one of the line
// sensors to finish the turn.  We use the inner line sensor that
// is closer to the target line in order to reduce overshoot.
void turn(char dir)
{
  if (dir == 'S')
  {
    // Don't do anything!
    return;
  }

  turnSensorReset();

  uint8_t sensorIndex;

  switch(dir)
  {
  case 'B':
    // Turn left 125 degrees using the gyro.
    motors.setSpeeds(-turnSpeed, turnSpeed);
    while((int32_t)turnAngle < turnAngle45 * 3)
    {
      turnSensorUpdate();
    }
    sensorIndex = 1;
    break;

  case 'L':
    // Turn left 45 degrees using the gyro.
    motors.setSpeeds(-turnSpeed, turnSpeed);
    while((int32_t)turnAngle < turnAngle45)
    {
      turnSensorUpdate();
    }
    sensorIndex = 1;
    break;

  case 'R':
    // Turn right 45 degrees using the gyro.
    motors.setSpeeds(turnSpeed, -turnSpeed);
    while((int32_t)turnAngle > -turnAngle45)
    {
      turnSensorUpdate();
    }
    sensorIndex = 3;
    break;

  default:
    // This should not happen.
    return;
  }

  // Turn the rest of the way using the line sensors.
  while(1)
  {
    readSensors();
    if (aboveLine(sensorIndex))
    {
      // We found the line again, so the turn is done.
      break;
    }
  }
}

// En funksjon som stopper opp motorene. Denne brukse før og 
// etter at bilen skal snu siden retningen på motorene endres.
void pauseMotors(){   
  motors.setSpeeds(0,0);  
  time_now = millis(); 
  while(millis() < time_now + intersectionDelay){   
  }
}

void followLine()
{
  while(1)
  {
    // Get the position of the line.
    uint16_t position = readSensors();

    // Our "error" is how far we are away from the center of the
    // line, which corresponds to position 2000.
    int16_t error = (int16_t)position - 2000;

    // Compute the difference between the two motor power
    // settings, leftSpeed - rightSpeed.
    int16_t speedDifference = error / 4;

    // Get individual motor speeds.  The sign of speedDifference
    // determines if the robot turns left or right.
    int16_t leftSpeed = (int16_t)speedLimit + speedDifference;
    int16_t rightSpeed = (int16_t)speedLimit - speedDifference;

    // Constrain our motor speeds to be between 0 and straightSpeed.
    leftSpeed = constrain(leftSpeed, 0, (int16_t)speedLimit);
    rightSpeed = constrain(rightSpeed, 0, (int16_t)speedLimit);

    motors.setSpeeds(leftSpeed, rightSpeed);

    // We use the inner four sensors (1, 2, 3, and 4) for
    // determining whether there is a line straight ahead, and the
    // sensors 0 and 5 for detecting lines going to the left and
    // right.
    //
    // This code could be improved by skipping the checks below
    // if less than 200 ms has passed since the beginning of this
    // function.  Maze solvers sometimes end up in a bad position
    // after a turn, and if one of the far sensors is over the
    // line then it could cause a false intersection detection.

    if(!aboveLine(0) && !aboveLine(1) && !aboveLine(2) && !aboveLine(3) && !aboveLine(4))
    {
      // There is no line visible ahead, and we didn't see any
      // intersection.  Must be a dead end.
      break;
    }

    // Når vanskelighetsgraden på banen øker trenger den å detektere 
    // en linje på begge de ytterste sensorene for å gå videre i programmet
    
    if (stateFollowLine == 0){
      if(aboveLine(0) || aboveLine(4))
      {
        // Found an intersection or a dark spot.
        break;
      }
    }
    else if (stateFollowLine == 1){
      if(aboveLine(0) && aboveLine(4)){
        break;
      }
    }
  }
}

// This should be called after followSegment to drive to the
// center of an intersection.  It also uses the line sensors to
// detect left, straight, and right exits.
void driveToIntersectionCenter(bool * foundRight, bool * foundStraight, bool * foundLeft)
{
  *foundRight = 0;
  *foundStraight = 0;
  *foundLeft = 0;

  // Drive stright forward to get to the center of the
  // intersection, while simultaneously checking for left and
  // right exits.
  //
  // readSensors() takes approximately 2 ms to run, so we use
  // it for our loop timing.  A more robust approach would be
  // to use millis() for timing.
  motors.setSpeeds(speedLimit, speedLimit);
  for(uint16_t i = 0; i < intersectionDelay / 2; i++) ///// / 2
  {
    readSensors();
    if(aboveLine(0))
    {
      *foundLeft = 1;
    }
    if(aboveLine(4))
    {
      *foundRight = 1;
    }
  }

  readSensors();

  // Check for a straight exit.
  if(aboveLine(1) || aboveLine(2) || aboveLine(3))
  {
    *foundStraight = 1;
  }
}

// This function decides which way to turn during the learning
// phase of maze solving.  It uses the variables found_left,
// found_straight, and found_right, which indicate whether there
// is an exit in each of the three directions, applying the
// left-hand-on-the-wall strategy.
char turnRight(bool foundRight)
{
  // Make a decision about how to turn.  The following code
  // implements a left-hand-on-the-wall strategy, where we always
  // turn as far to the left as possible.
  if(foundRight) { return 'R'; }
}

char turnLeft(bool foundLeft)
{
  // Make a decision about how to turn.  The following code
  // implements a left-hand-on-the-wall strategy, where we always
  // turn as far to the left as possible.
  if(foundLeft) { return 'L'; }
}

char driveStraight(bool foundStraight)
{
  // Make a decision about how to turn.  The following code
  // implements a left-hand-on-the-wall strategy, where we always
  // turn as far to the left as possible.
  if(foundStraight) { return 'S'; }
}

char turnAround(bool foundLeft, bool foundStraight, bool foundRight)
{
  // Make a decision about how to turn.  The following code
  // implements a left-hand-on-the-wall strategy, where we always
  // turn as far to the left as possible.
  if(!foundRight && !foundLeft && !foundStraight) { return 'B'; }
}

/*
 * Under er en del like funksjoner som følger en linje fram til den
 * møter en teipbit. Den kjører videre til midten av krysset og tar 
 * en gitt retning
 */

void goRight()
{

  while(1)
  {
    // Navigate current line segment until we enter an intersection.
    followLine();

    bool foundRight, foundStraight, foundLeft;
    driveToIntersectionCenter(&foundRight, &foundStraight, &foundLeft);

    char dir = turnRight(foundRight);

    pauseMotors();
    
    // Make the turn.
    turn(dir);

    pauseMotors();


    break;
  }

}

void goLeft()
{

  while(1)
  {
    // Navigate current line segment until we enter an intersection.
    followLine();

    bool foundRight, foundStraight, foundLeft;
    driveToIntersectionCenter(&foundRight, &foundStraight, &foundLeft);

    char dir = turnLeft(foundLeft);

    pauseMotors();
    // Make the turn.
    turn(dir);
    
    pauseMotors();


    break;
    
  }

}

void goStraight()
{

  while(1)
  {
    // Navigate current line segment until we enter an intersection.
    followLine();

    bool foundRight, foundStraight, foundLeft;
    driveToIntersectionCenter(&foundRight, &foundStraight, &foundLeft);

    char dir = driveStraight(foundStraight);
    
    // Make the turn.
    turn(dir);

    motors.setSpeeds(0,0);

    break;
    
  }
}

void deadEnd()
{

  while(1)
  {
    // Navigate current line segment until we enter an intersection.
    followLine();

    
    bool foundRight, foundStraight, foundLeft;
    driveToIntersectionCenter(&foundRight, &foundStraight, &foundLeft);

    char dir = turnAround(foundLeft, foundStraight, foundRight);

    pauseMotors();
    // Make the turn.
    turn(dir);

    pauseMotors();
    
    break;
  }
}

//stopper bilen når den har funnet en ende
void foundEnd()
{

  while(1)
  {
    // Navigate current line segment until we enter an intersection.
    followLine();

    motors.setSpeeds(0,0);
    break;
    
  } 
}


// Calibrates the line sensors by turning left and right, then
// shows a bar graph of calibrated sensor readings on the display.
// Returns after the user presses A.
static void lineSensorSetup()
{
  // Delay so the robot does not move while the user is still
  // touching the button.
  delay(1000);

  // We use the gyro to turn so that we don't turn more than
  // necessary, and so that if there are issues with the gyro
  // then you will know before actually starting the robot.

  turnSensorReset();

  // Turn to the left 90 degrees.
  motors.setSpeeds(-calibrationSpeed, calibrationSpeed);
  while((int32_t)turnAngle < turnAngle45 * 2)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Turn to the right 90 degrees.
  motors.setSpeeds(calibrationSpeed, -calibrationSpeed);
  while((int32_t)turnAngle > -turnAngle45 * 2)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Turn back to center using the gyro.
  motors.setSpeeds(-calibrationSpeed, calibrationSpeed);
  while((int32_t)turnAngle < 0)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Stop the motors.
  motors.setSpeeds(0, 0);
  
}

void gridMovementSetup()
{
  // Configure the pins used for the line sensors.
  lineSensors.initFiveSensors();

  // Calibrate the gyro and show readings from it until the user
  // presses button A.
  turnSensorSetup();

  // Calibrate the sensors by turning left and right, and show
  // readings from it until the user presses A again.
  lineSensorSetup();
}
