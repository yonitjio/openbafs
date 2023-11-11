#define SERVOINPUT_SUPPRESS_WARNINGS

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SoftwareSerial.h>
#include <GCodeParser.h>
#include "Stepper.h"

#define SERVO_CNT 4
#define SERVO_SDA_PIN 4
#define SERVO_SCL_PIN 5
#define SERVO_MIN_MS 600
#define SERVO_MAX_MS 2400
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates
#define SERVO_OSC_FREQ 25800000

#define STEPPER_EN_PIN 9
#define STEPPER_STEP_PIN 10
#define STEPPER_DIR_PIN 11
#define SMALL_FEED_DISTANCE 5

// Stepper
const byte micro_step = 16;
const int step_rotation = 200;
const int dist_rotation = 34.56;

const int retract_distance[] = {107, 107, 107, 107};
const int feed_distance[] = {120, 120, 120, 120};
const int slower_margin[] = {10, 10, 10, 10};

float step_accelleration = 40000;
float step_speed = 40000;
float step_max_speed = 40000;
float step_slower_speed = 1000;

// speed is steps/second
// 1.8deg stepper => 200 steps/revolution, 16 microsteps => 3200 steps/rev
// mk8 brass gear diameter => 11mm
// 1 revolution => 34.56mm
// speed 4000 steps/sec => 1000/3200 rev/sec = 0.3125 rev/sec => 0.3125 * 34.56 mm/sec = 10.8 mm/sec
// speed 12000 steps/sec => 12000/3200 = 3.75 rev/sec => 3.75 * 34.56 mm/sec = 129.6 mm/sec

Stepper stepper(AccelStepper::DRIVER, STEPPER_STEP_PIN, STEPPER_DIR_PIN);

// #define DEBUG
// #define DEBUG_SELECTION
#define DEBUG_PRINTER_SERIAL
#define DEBUG_GCODE
// #define DEBUG_STEPPER

// Filament Sensor
#define FILAMENT_SENSOR_PIN 2

// Camera Remote
#define CAM_PIN 3
#define CAM_IDLE 305000
unsigned long cam_last_active = millis();

// Software Serial
#define SERIAL_RX_PIN 12
#define SERIAL_TX_PIN 13

SoftwareSerial printer_serial(SERIAL_RX_PIN, SERIAL_TX_PIN); // RX, TX
GCodeParser GCode = GCodeParser();

int selection = -1;
int last_selection = -1;

int mode = 0; //0: operational, 1: configure, 2: default serial as gcode receiver

// Servo
Adafruit_PWMServoDriver servoController = Adafruit_PWMServoDriver();

const byte servo_on_deg[] = {14, 162, 32, 152};
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
void triggerCamera();
void wakeupBluetooth();
void servoWrite(uint8_t i, uint16_t value);
void servoOff(uint8_t i);
void servoOn(uint8_t i);

void setup() {
  Serial.begin(9600);

  // Stepper
  stepper.setEnablePin(STEPPER_EN_PIN);
  stepper.setPinsInverted(true, false, true);
  stepper.setMinPulseWidth(1);

  stepper.setAcceleration(step_accelleration);
  stepper.setMaxSpeed(step_max_speed);
  stepper.setSpeed(step_speed);

  stepper.setMicroStep(micro_step);
  stepper.setDistancePerRotation(dist_rotation);
  stepper.setStepsPerRotation(step_rotation);

  // Servo
  servoController.begin();
  servoController.setOscillatorFrequency(SERVO_OSC_FREQ);
  servoController.setPWMFreq(SERVO_FREQ);
  delay(10);

  // Filament Sensor
  pinMode(FILAMENT_SENSOR_PIN, INPUT_PULLUP);

  // Camera Remote
  pinMode(CAM_PIN, OUTPUT);
  digitalWrite(CAM_PIN, HIGH);
  cam_last_active = millis();

  // Printer Serial
  printer_serial.begin(9600);
  printer_serial.println("Ok");
}

void feedFilament(){
  servoOn(last_selection);
  delay(250);

  stepper.enableOutputs();
  stepper.setCurrentPosition(0);
  stepper.runRelative(getSmallFeedDistance(last_selection));
  stepper.setSpeed(step_speed);
  stepper.disableOutputs();
  delay(250);

  servoOff(last_selection);
  delay(250);
}

void switchFilament(){
  if (selection >= SERVO_CNT) {
    return;
  }

  if (last_selection != selection) {
    if (last_selection > -1) {
      stepper.enableOutputs();

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning on "); Serial.print(last_selection); Serial.println(".");
      #endif

      servoOn(last_selection);
      delay(250);

      #ifdef DEBUG_STEPPER
        Serial.print(" Retracting: ");  Serial.print(last_selection); Serial.print(", "); Serial.print(retract_distance[last_selection]);
      #else
        Serial.print(" Retracting: "); Serial.println(last_selection);
      #endif

      stepper.setCurrentPosition(0);
      stepper.runRelative(-getRetractDistance(last_selection));
      stepper.setSpeed(step_speed);
      delay(250);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning off "); Serial.print(last_selection); Serial.println(".");
      #endif
      servoOff(last_selection);
      delay(250);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning on "); Serial.print(selection); Serial.println(".");
      #endif
      servoOn(selection);
      delay(250);

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
      delay(250);

      #ifdef DEBUG_SELECTION
        Serial.print(" Turning off "); Serial.print(selection); Serial.println(".");
      #endif

      servoOff(selection);
      delay(250);
    }

    last_selection = selection;

    stepper.disableOutputs();
  }
}

void wakeupBluetooth(){
  digitalWrite(CAM_PIN, LOW);
  delay(50);
  digitalWrite(CAM_PIN, HIGH);
  cam_last_active = millis();
}

void triggerCamera(int d){
  Serial.println("Triggering Camera");
  digitalWrite(CAM_PIN, LOW);
  delay(d);
  digitalWrite(CAM_PIN, HIGH);
  cam_last_active = millis();
}

void processGCode(){
  if (!GCode.blockDelete){
    if (GCode.HasWord('C')) { // C-Codes
      int cCodeNumber = (int)GCode.GetWordValue('C');
      #ifdef DEBUG_GCODE
        Serial.print(" C: "); Serial.print(cCodeNumber); Serial.println(".");
      #endif
      switch (cCodeNumber){
        case 0: { // feed a little bit of filament
          feedFilament();
          if (mode == 0)
            printer_serial.println("ok");
          else
            Serial.println("ok");
        }
        break;
        default: {
            Serial.println(" Unknown C code.");
        }
        break;
      }
    } else if (GCode.HasWord('M')) { // M-Codes
      int mCodeNumber = (int)GCode.GetWordValue('M');
      #ifdef DEBUG_GCODE
        Serial.print(" M: "); Serial.print(mCodeNumber); Serial.println(".");
      #endif
      switch (mCodeNumber){
        case 709: { // Reset
          selection = -1;
          last_selection = -1;
          mode = 0;
          #ifdef DEBUG_GCODE
            Serial.println("BAFSD Reset.");
          #endif
        }
        break;
        case 412: { // Check filament
          int state = digitalRead(FILAMENT_SENSOR_PIN);
          #ifdef DEBUG_GCODE
            Serial.print(" Filament: "); Serial.print(state); Serial.println(".");
          #endif
          if (state == LOW){  // low = filament present, high = filament not present
            if (mode == 0)
              printer_serial.println("ok");
            else
              Serial.println("ok");
          } else {
            if (mode == 0)
              printer_serial.println("no");
            else
              Serial.println("no");
          }
        }
        break;
        case 240: { // Camera
          int duration = (int)GCode.GetWordValue('D');
          triggerCamera(duration);
        }
        break;
        default: {
            Serial.println(" Unknown M code.");
        }
        break;
      }
    } else if (GCode.HasWord('T')) { // T-Codes
      int tCodeNumber = (int)GCode.GetWordValue('T');
      #ifdef DEBUG_GCODE
        Serial.print(" T: "); Serial.print(tCodeNumber); Serial.println(".");
      #endif
      selection = tCodeNumber;
      switchFilament();
      if (mode == 0)
        printer_serial.println("ok");
      else
        Serial.println("ok");
  }
  }
}

void loop() {
  unsigned long now = millis();
  if (now - cam_last_active > CAM_IDLE) {
    wakeupBluetooth();
  }
  if (mode == 0) {
    if (printer_serial.available()) {
      if (GCode.AddCharToLine(printer_serial.read())) {
        #ifdef DEBUG_PRINTER_SERIAL
          Serial.print(" Received: "); Serial.println(GCode.line);
        #endif
        GCode.ParseLine();
        processGCode();
      }
    }
  }
  else if (mode == 2) {
    if (Serial.available()) {
      if (GCode.AddCharToLine(Serial.read())) {
        #ifdef DEBUG_PRINTER_SERIAL
          Serial.print(" Received: "); Serial.println(GCode.line);
        #endif
        GCode.ParseLine();
        processGCode();
      }
    }
    return;
  }

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
        servoOff(n);
        delay(250);
      }
      Serial.println("Entering M0 mode.");
    } else if (input_string.equalsIgnoreCase("m1")) {
      mode = 1;
      Serial.println("Entering M1 mode.");
    } else if (input_string.equalsIgnoreCase("m2")) {
      mode = 2;
      Serial.println("Entering M2 mode.");
    } else if (input_string.equalsIgnoreCase("rs")) {

      double pulselength;
      pulselength = 1000000; // 1,000,000 us per second

      uint16_t prescale = servoController.readPrescale();

      prescale += 1;
      pulselength *= prescale;
      pulselength /= SERVO_OSC_FREQ;

      uint16_t pwm = servoController.getPWM(selection);
      uint16_t pulse = pwm * pulselength;

      unsigned long camIdle = millis() - cam_last_active;

      Serial.print(" Mode: "); Serial.println(mode);
      Serial.print(" Selection: "); Serial.println(selection);
      Serial.print(" pwm: "); Serial.println(pwm);
      Serial.print(" pulse: "); Serial.println(pulse);
      Serial.print(" pulseLength: "); Serial.println(pulselength);
      Serial.print(" Cam Idle: "); Serial.println(camIdle);

      if (pulse < SERVO_MIN_MS) {
        pulse = SERVO_MIN_MS;
      }

      if (pulse > SERVO_MAX_MS) {
        pulse = SERVO_MAX_MS;
      }
      uint16_t deg = map(pulse, SERVO_MIN_MS, SERVO_MAX_MS, 0, 180);

      Serial.print(" deg: "); Serial.println(deg);
      Serial.print(" filament: "); Serial.println(digitalRead(FILAMENT_SENSOR_PIN));
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
        case 'c': // camera
          {
            int d_val = p1.toInt();
            triggerCamera(d_val);
          }
        break;
        case 'e': // small feed
          {
            long t = millis();
            feedFilament();
            t = millis() - t;
            Serial.print("time: "); Serial.println(t);
          }
          break;
        case 'f': // switch filament p1 selection
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
            #ifdef DEBUG
              long steps = stepper.runRelative(dis_val);
            #else
              stepper.runRelative(dis_val);
            #endif
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
        case 's': // set servo degree p1 = servo index, p2 = degree
          {
            int s_val = p1.toInt();
            int d_val = p2.toInt();

            #ifdef DEBUG
              Serial.print("s_val: "); Serial.print(p1); Serial.print( " d_val: "); Serial.println(p2);
            #endif

            s_val = constrain(s_val, 0, SERVO_CNT);
            d_val = constrain(d_val, 0, 180);

            Serial.print("Moving "); Serial.println(s_val);
            servoWrite(s_val, d_val);
          }
          break;
        case 't': // test servos and stepper
          {
              for(int n = 0; n < SERVO_CNT; n++)
              {
                servoWrite(n, 100);
                delay(500);
                servoOff(n);
                delay(250);
              }

              stepper.enableOutputs();
              stepper.setCurrentPosition(0);
              stepper.runRelative(100);
              stepper.setSpeed(step_speed);
              stepper.disableOutputs();

          }
          break;
        case 'z': // tests
          {
              for(int n = 0; n < SERVO_CNT; n++)
              {
                servoWrite(n, 100);
                delay(500);
                servoOff(n);
                delay(250);
              }

              stepper.enableOutputs();
              stepper.setCurrentPosition(0);
              stepper.runRelative(100);
              stepper.setSpeed(step_speed);
              stepper.disableOutputs();

          }
          break;
        default:
          break;
      }
      command = "";
    }
  }
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

void servoWrite(uint8_t i, uint16_t value)
{
  if(value < SERVO_MIN_MS)
  {  // treat values less than 544 as angles in degrees (valid values in microseconds are handled as microseconds)
    if(value < 0) value = 0;
    if(value > 180) value = 180;
    value = map(value, 0, 180, SERVO_MIN_MS,  SERVO_MAX_MS);
  }
  servoController.writeMicroseconds(i, value);
}

void servoOn(uint8_t i)
{
  servoWrite(i, servo_on_deg[i]);
}

void servoOff(uint8_t i)
{
  servoWrite(i, servo_off_deg[i]);
  delay(250);
  servoController.setPWM(i, 0, 0);
}
