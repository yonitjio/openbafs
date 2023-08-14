# Basic Automatic Filament Switch

BAFS is a kind of multi material unit for 3d printer. It's very basic, don't expect fancy stuff. It only do one thing: switching filament.

Features:

1. Modular design, i.e. you decide how many ports you want, print accordingly.
2. Only 1 pwm port needed from the printer
3. Redundant design, if one of the motor port stop working, most likely it won't affect the other.
4. Cheap, it uses commonly available parts.
5. Easy to print, for the most parts, you might not need to print with support at all.

BAFS is a great and affordable way to learn and understand how multi filament / multi material 3d printing works.

Be aware this design has the same problems/challenges as similar design using single nozzle and y-splitter: jams, clogging, color bleeds, etc.

To use BAFS you need firmware support, unfortunately, currently there's no official firmware support. For custom Marlin see my other repo: [openbafs-marlin](https://github.com/yonitjio/openbafs-marlin).

Most printers don't have many servo ports, to solve this,I use one pwm port from the printer to generate pwm pulse that signal which port to use to external module. I use arduino, stepper motor shield and custom board.

Non printed parts:

- Servos (I use MG996R)
- A Stepper motor (for direct drive printers)
- M5 rod, stainless is preferable.
- M5 to M5 coupler
- M3 bolts and nuts
- M4 bolts
- Long M4 bolts and nuts
- Bearings 625zz, 623zz, u604zz
- Extruder gears
- Grease
- PC4-M6

Electronic parts:

- Arduino
- Protoshield for Arduino
- Stepdown module
- Power Supply
- Connectors
- Wires
- Connectors

These parts are only for the switch, you still need at least a y-splitter. Use whichever work for you. You can search it on thingiverse, printables, etc.

### Extrusion System
Initially BAFS was designed for bowden system, but since I changed my printer to direct drive, I also update BAFS to support direct drive.

With bowden extrusion system, BAFS also acts as the extruder, it uses the printer's stepper motor.

For printers with direct drive extrusion system, it acts as a feeder. It needs additional stepper motor.

Each version requires different firmware. Other than the stepper motor and firmware, everything else is the same.

This repo is for direct drives.

### Electronics
To build the electronics you need to have at least basic skills about arduino and electronics. It requires soldering and making wires with connectors.

It is pretty straight forward. Nothing fancy here. You just need to connect the servos, stepper motor and the printer to arduino. 

There are plenty tutorials about using servos and stepper motor with arduino.

Adjust the codes according to your setup.

#### Known Issues:
- Due to one way communication between printer and bafs, to print with starting port other than 0 (T0) needs extra manual steps. 
- Also because the way bafs communicate with the printer, if you finished a print and want to print another, you may need to reset both printer and bafs and prepare the filaments all over again. Since both retain last tool, new print is most likely to begin with T0, so bafs will do switching proses. This behavior may not be what you want.

#### Note:
- Even though it's modular, it's better to plan ahead to minimize costs, for example if you plan to use 4 filaments but want to start with 2 first, it's better to prepare the shaft and bolts for 4 filaments from the start. This is because the you need longer shaft and bolts for more ports for the filaments.
- Values in the code are for my setup. Most likely it won't work for yours. Adjust accordingly first.

#### DISCLAIMER: 
This is an experimental project. It may or may not work. Use it at your own risk.
