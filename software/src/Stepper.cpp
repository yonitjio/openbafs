#include "Stepper.h"

Stepper::Stepper(
	uint8_t interface, 
	uint8_t pin1, 
	uint8_t pin2, 
	uint8_t pin3, 
	uint8_t pin4, 
	bool enable) : 
	AccelStepper(interface, pin1, pin2, pin3, pin4, enable) {} 

/**
 * Set the microSteps value.
 * @param value
 */
void Stepper::setMicroStep(int value) {
	microSteps = value;
}

/**
 * Set the number of steps a step motor should do to complete one full rotation.
 * @param value
 */
void Stepper::setStepsPerRotation(int value) {
	stepsPerRotation = value;
}

/**
 * Set the distance which the step motor cover with one full rotation.
 * @param value
 */
void Stepper::setDistancePerRotation(float value) {
	distancePerRotation = value;
}

/**
 * Return the current distance from the origin.
 * @return
 */
float Stepper::getCurrentPositionDistance() {
	return currentPosition() / (microSteps * stepsPerRotation / distancePerRotation);
}

/**
 * The motor travel a certain distance.
 * @param value
 */
long Stepper::moveToDistance(float value) {
  long steps = value * microSteps * stepsPerRotation / distancePerRotation;
  moveTo(steps);
  return steps;
}

/**
 * Move the motor to a new relative distance.
 * @param value
 */
long Stepper::moveRelative(float value)
{
  long steps = value * microSteps * stepsPerRotation / distancePerRotation;
  move(steps);
  return steps;
}

/**
 * Move and run the motor to a new relative distance.
 * @param value
 */
long Stepper::runRelative(float value)
{
  long steps = value * microSteps * stepsPerRotation / distancePerRotation;
  move(steps);
  runToPosition();
  return steps;
}

/**
 * The motor goes to the new specified distance.
 * @param value
 */
long Stepper::runToNewDistance(float value) {
    long steps = moveToDistance(value);
    runToPosition();
	return steps;
}

/**
 * Set the angle per rotation property.
 * @param value
 */
void Stepper::setAnglePerRotation(float value) {
	anglePerRotation = value;
}

/**
 * The motor goes to the new specified angle.
 * @param value
 */
void Stepper::moveToAngle(float value) {
	moveTo(value * microSteps * stepsPerRotation / anglePerRotation);
}

/**
 * The motor goes to the new specified angle.
 * @param value
 */
void Stepper::runToNewAngle(float value) {
    moveToAngle(value);
    runToPosition();
}