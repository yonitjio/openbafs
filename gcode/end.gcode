G92 E0 ; reset extruder position
G1 E-[retract_length_toolchange[current_extruder]] F3200 ; retract to avoid leaks
G1 X230 Y200 F12000 ; move back, home x
G4 ; clear buffer movement

M240 ; camera

M104 S0 ; turn off temperature
M84     ; disable motors
