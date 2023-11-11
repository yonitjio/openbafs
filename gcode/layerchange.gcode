G92 E0 ; reset extruder position
G1 E-[retract_length_toolchange[current_extruder]] F3200 ; retract to avoid leaks
G1 X230 Y200 F12000 ; move back, home x
G4 ; clear buffer movement

M240 ; camera
G4 P1000 ; delay

{
    local drLength = 0 + (retract_length_toolchange[current_extruder] + retract_restart_extra_toolchange[current_extruder]) ;
}

G1 E1 F3200 ; deretract
G92 E0 ; reset extruder position
