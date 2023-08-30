#define SERVOINPUT_SUPPRESS_WARNINGS

#include <Servo.h>
#include <ServoInput.h>
#include "Stepper.h"

#define SERVO_CNT 4
#define SERVO_01 3
#define SERVO_02 6
#define SERVO_03 9
#define SERVO_04 11
#define SERVO_MIN_PULSE 540
#define SERVO_MAX_PULSE 2400

#define STEPPER_EN 4
#define STEPPER_STEP 10
#define STEPPER_DIR 7
#define SMALL_FEED_DISTANCE 2

// Stepper
const byte micro_step = 16;
const int step_rotation = 200;
const int dist_rotation = 34.56;

const int retract_distance[] = {95, 95, 95, 95};
const int feed_distance[] = {98, 98, 100, 100};
const int slower_margin[] = {10, 10, 10, 10};

int step_accelleration = 12000;
int step_speed = 12000;
int step_max_speed = 12000;
int step_slower_speed = 4000;

Stepper stepper(AccelStepper::DRIVER, STEPPER_STEP, STEPPER_DIR);

#define DEBUG
#define DEBUG_SELECTION
#define DEBUG_SERVO_INPUT
// #define DEBUG_STEPPER

// Servo Input
const byte servo_input_pin = 2;
ServoInputPin<servo_input_pin> servoInput(SERVO_MIN_PULSE, SERVO_MAX_PULSE);

int selection = -1;
int last_selection = -1;
int last_servo_input_pulse = -1;

int mode = 0; //0: operational, 1: configure
// bool first_run = true;

// Servo
int delta = abs((SERVO_MAX_PULSE - SERVO_MIN_PULSE) / SERVO_CNT);

Servo servo[SERVO_CNT];
const byte servo_pin[] = {SERVO_01, SERVO_02, SERVO_03, SERVO_04};
const byte servo_on_deg[] = {32, 149, 29, 157};
const byte servo_off_deg[] = {90, 90, 90, 90};

String input_string;

void onRising();
void processPWM();
void switchFilament();
void feedFilament();
String getValue(String data, char separator, int index);
int mapSelection(int s);
int getFeedDistance(int sel);
int getSmallFeedDistance(int sel);
int getSlowerMargin(int sel);
int getRetractDistance(int sel);

void setup() {
  Serial.begin(9600);

  // Stepper
  stepper.setEnablePin(STEPPER_EN);
  stepper.setPinsInverted(true, false, true);
  stepper.setMinPulseWidth(1);

  stepper.setAcceleration(step_accelleration);
  stepper.setMaxSpeed(step_max_speed);
  stepper.setSpeed(step_speed);

  stepper.setMicroStep(micro_step);
  stepper.setDistancePerRotation(dist_rotation);
  stepper.setStepsPerRotation(step_rotation);

  // Servo
  for(int n = 0; n < SERVO_CNT; n++)
  {
    servo[n].attach(servo_pin[n], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    servo[n].write(90);
    delay(250);
  }

}

void feedFilament(){
  if (!servo[last_selection].attached())
    servo[last_selection].attach(servo_pin[last_selection], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    
  servo[last_selection].write(servo_on_deg[last_selection]);
  delay(500);

  stepper.enableOutputs();
  stepper.setCurrentPosition(0);
  stepper.runRelative(getSmallFeedDistance(last_selection));
  stepper.setSpeed(step_speed);
  stepper.disableOutputs();      
  delay(500);

  servo[last_selection].write(servo_off_deg[last_selection]);
  delay(500);
}

void switchFilament(){
  if (last_selection != selection) {
    if (last_selection > -1) {
      stepper.enableOutputs();

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning on "); Serial.print(last_selection); Serial.println(".");
      #endif
      if (!servo[last_selection].attached())
        servo[last_selection].attach(servo_pin[last_selection], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
        
      servo[last_selection].write(servo_on_deg[last_selection]);
      delay(500);

      #ifdef DEBUG_STEPPER
        Serial.print(" Retracting: ");  Serial.print(last_selection); Serial.print(", "); Serial.print(retract_distance[last_selection]);
      #else
        Serial.print(" Retracting: "); Serial.println(last_selection);
      #endif

      stepper.setCurrentPosition(0);
      stepper.runRelative(-getRetractDistance(last_selection));
      stepper.setSpeed(step_speed);
      delay(500);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning off "); Serial.print(last_selection); Serial.println(".");
      #endif
      if (!servo[last_selection].attached())
        servo[last_selection].attach(servo_pin[last_selection], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
        
      servo[last_selection].write(servo_off_deg[last_selection]);
      delay(500);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning on "); Serial.print(selection); Serial.println(".");
      #endif
      if (!servo[selection].attached())
        servo[selection].attach(servo_pin[selection], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
        
      servo[selection].write(servo_on_deg[selection]);
      delay(500);

      #ifdef DEBUG_STEPPER
        Serial.print(" Feeding: "); Serial.print(selection); Serial.print(", "); Serial.print(feed_distance[selection]);
      #else
        Serial.print(" Feeding: "); Serial.println(selection);
      #endif

      stepper.setCurrentPosition(0);
      stepper.runRelative(getFeedDistance(selection) - getSlowerMargin(selection));
      stepper.setSpeed(step_slower_speed);
      stepper.runRelative(getSlowerMargin(selection));
      stepper.setSpeed(step_speed);
      delay(500);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning off "); Serial.print(selection); Serial.println(".");
      #endif
      if (!servo[selection].attached())
        servo[selection].attach(servo_pin[selection], SERVO_MIN_PULSE, SERVO_MAX_PULSE);

      servo[selection].write(servo_off_deg[selection]);
      delay(500);
    }

    last_selection = selection;
    
    stepper.disableOutputs();      
  }
}

void doSwitch(){
  if (mode == 0) {
    int servo_input_pulse = servoInput.getPulse();

    if (last_servo_input_pulse != servo_input_pulse && abs(last_servo_input_pulse - servo_input_pulse) > 20) {
      #ifdef DEBUG_SERVO_INPUT
        Serial.print(" Pulse "); Serial.print(servo_input_pulse); Serial.print(", "); 
        Serial.print(last_servo_input_pulse); Serial.println(".");
      #endif

      int mapValue = mapSelection(servo_input_pulse);
      if (mapValue == 0) {
        selection = -1;
        last_selection = -1;
        last_servo_input_pulse = -1;
        mode = 0;
      } else if(mapValue == 1) {
        feedFilament();
      } else {
        selection = mapValue - 2;
        switchFilament();
      }
      last_servo_input_pulse = servo_input_pulse;
    }
  }
}

void loop() {
  doSwitch();
  
  String command = "";

  if (Serial.available() > 0) {
    String input_string = Serial.readStringUntil('\n');
    input_string.trim();
    Serial.println("Received: " + input_string);
    if (input_string.equalsIgnoreCase("m0")) {
      mode = 0;
      // Servo
      for(int n = 0; n < SERVO_CNT; n++)
      {
        servo[n].attach(servo_pin[n], SERVO_MIN_PULSE, SERVO_MAX_PULSE);
        servo[n].write(servo_off_deg[n]);
        delay(250);
      }
      #ifdef DEBUG
        Serial.println("Entering M0 mode.");
      #endif
    } else if (input_string.equalsIgnoreCase("m1")) {
      mode = 1;
      #ifdef DEBUG
        Serial.println("Entering M1 mode.");
      #endif
    } else if (input_string.equalsIgnoreCase("rs")) {
      Serial.print("Selection: "); Serial.print(selection); 
      Serial.print(" milis: "); Serial.print(servo[selection].readMicroseconds());
      Serial.print(" deg: "); Serial.print(servo[selection].read());
      // Serial.print(" First Run: "); Serial.print(first_run);
      Serial.println();
    }else {
      command = input_string;
    }
  }
  if (mode == 1) {
    if (command.length() > 0){
      #ifdef DEBUG
        Serial.print("command: "); Serial.println(command);
      #endif

      char cmd = getValue(command, ' ', 0)[0];
      String p1 = getValue(command, ' ', 1);
      String p2 = getValue(command, ' ', 2);

      switch (cmd)
      {
        case 'e': // small feed
          {
            long t = millis();
            feedFilament();
            t = millis() - t;
            Serial.print("time: "); Serial.println(t);
          }
          break;
        case 'f': // switch filament p1 s selection
          {
            long t = millis();
            selection = p1.toInt();
            switchFilament();
            t = millis() - t;
            Serial.print("time: "); Serial.println(t);
          }
          break;
        case 'd': // move stepper p1 = steps
          {
            long step_val = p1.toInt();

            #ifdef DEBUG
              Serial.print("step_val: "); Serial.println(p1);
            #endif

            stepper.enableOutputs();
            stepper.move(step_val);
            stepper.runToPosition();
            stepper.setSpeed(step_speed);
            stepper.disableOutputs();            
          }
          break;
        case 'p': // move stepper p1 = distance, p2 = not used
          {
            long dis_val = p1.toInt();
            long t = millis();
            #ifdef DEBUG
              Serial.print("dis_val: "); Serial.println(p1);
            #endif

            stepper.enableOutputs();
            stepper.setCurrentPosition(0);
            long steps = stepper.runRelative(dis_val);
            stepper.setSpeed(step_speed);
            stepper.disableOutputs();            

            t = millis() - t;
            #ifdef DEBUG
              Serial.print("steps: "); Serial.println(steps);
              Serial.print("time: "); Serial.println(t);
            #endif
          }
          break;
        case 'm': // select servo p1 = servo
          {
            int s_val = p1.toInt();

            #ifdef DEBUG
              Serial.print("s_val: "); Serial.println(p1);
            #endif

            s_val = constrain(s_val, 0, SERVO_CNT);
            last_selection = s_val;
            selection = s_val;
          }
          break;
        case 'a': // test mapSelection p1 = angle
          {
            int s_val = p1.toInt();
            int value = map(constrain(s_val, 0, 180), 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
            value = mapSelection(value);

            Serial.print(s_val); Serial.print(" maps to "); Serial.println(value);
          }
          break;
        case 's': // set servo degree p1 = servo index, p2 = degree
          {
            int s_val = p1.toInt();
            int d_val = p2.toInt();

            #ifdef DEBUG
              Serial.print("s_val: "); Serial.print(p1); Serial.print( " d_val: "); Serial.println(p2);
            #endif

            s_val = constrain(s_val, 0, SERVO_CNT);
            d_val = constrain(d_val, 0, 180);

            if (!servo[s_val].attached())
              servo[s_val].attach(servo_pin[s_val], SERVO_MIN_PULSE, SERVO_MAX_PULSE);

            Serial.print("Moving "); Serial.println(s_val);
            servo[s_val].write(d_val);
          }
          break;        
        default:
          break;
      }
      command = "";
    }
  }
}

int mapSelection(int s) {
  int max = SERVO_MAX_PULSE;
  int min = SERVO_MIN_PULSE;

  if (SERVO_MAX_PULSE < SERVO_MIN_PULSE) {
    max = SERVO_MIN_PULSE;
    min = SERVO_MAX_PULSE;
  }

  // int ret = map(constrain(s, min, max), min, max, 0, SERVO_CNT - 1);
  // return ret;

  float range = max - min;
  float s1 = constrain(s, min, max);
  float r = s1 - min;
  
  r = constrain(r, 0, range);

  float calc = (r/range) * (SERVO_CNT + 1); // -1 + 1 for reset + 1 for feed
  int rounded = round(calc);
  #ifdef DEBUG_SELECTION
    Serial.print("Mapping: "); Serial.print(s), Serial.print(", "); Serial.print(r); Serial.print(", "); Serial.print(calc); Serial.print(", "); Serial.println(rounded);
  #endif
  return rounded;
}

String getValue(String data, char separator, int index)
{
    int maxIndex = data.length() - 1;
    int j = 0;
    String chunkVal = "";

    for (int i = 0; i <= maxIndex && j <= index; i++)
    {
        chunkVal.concat(data[i]);

        if (data[i] == separator)
        {
            j++;

            if (j > index)
            {
                chunkVal.trim();
                return chunkVal;
            }

            chunkVal = "";
        }
        else if ((i == maxIndex) && (j < index)) {
            chunkVal = "";
            return chunkVal;
        }
    }

    return chunkVal;   
}

int getRetractDistance(int sel) {
  if (sel % 2 == 0) {
    return retract_distance[sel];
  } else {
    return -retract_distance[sel];
  }
}

int getSmallFeedDistance(int sel) {
  if (sel % 2 == 0) {
    return SMALL_FEED_DISTANCE;
  } else {
    return -SMALL_FEED_DISTANCE;
  }
}

int getFeedDistance(int sel) {
  if (sel % 2 == 0) {
    return feed_distance[sel];
  } else {
    return -feed_distance[sel];
  }
}

int getSlowerMargin(int sel) {
  if (sel % 2 == 0) {
    return slower_margin[sel];
  } else {
    return -slower_margin[sel];
  }
}
