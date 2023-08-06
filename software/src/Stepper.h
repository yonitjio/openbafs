#ifndef Stepper_h
#define Stepper_h

#include <AccelStepper.h>

class Stepper : public AccelStepper {
    private:
		int microSteps = 16;
		int stepsPerRotation = 200;
		float distancePerRotation = 34.56;
		float anglePerRotation = 360;
		/*
		ts = microsteps * stepsperrotation = 360 = 34.55752 mm
		1 degree = degreeperrotation / 360
		1 step = stepsperrotation / 360
		1 mm = distanceperrotation / 360
		steps = distance * stepperrotation / distanceperrotation				
		*/

    public:
		Stepper(
			uint8_t interface = AccelStepper::DRIVER, 
			uint8_t pin1 = 2, 
			uint8_t pin2 = 3, 
			uint8_t pin3 = 4, 
			uint8_t pin4 = 5, 
			bool enable = true);

		void setMicroStep(int value);

		void setStepsPerRotation(int value);

		void setDistancePerRotation(float value);

		float getCurrentPositionDistance();

		long moveToDistance(float value);

        long moveRelative(float value);
    
        long runRelative(float value);

		long runToNewDistance(float value);

		void setAnglePerRotation(float value);

		void moveToAngle(float value);

		void runToNewAngle(float value);
};

#endif