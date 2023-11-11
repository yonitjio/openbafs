; toolchange from [current_extruder] to [next_extruder], layer [layer_num]
{if current_extruder != next_extruder}
    M220 B ; save feedrate
    M220 S100 ; set feedrate

    G90 ; absolute movement
    M83 ; E relative
    M106 S0 ; turn off fan
    G92 E0 ; reset extruder position
    G1 E0.5 F360 ; retract
    G4 ; clear buffer movement
    G1 X210 F12000 ; move to purge area
    G1 X220 F2000
    G1 X230 F1000
    G4 ; clear buffer movement

    ; FILAMENT TIP SHAPING
    G1 E-0.0500 F10 ; retract
    G1 E-0.0500 F20 ; retract 0.1
    G4 S{filament_cost[current_extruder]} ; delay
    G1 E-0.0500 F30 ; retract
    G1 E-0.0500 F40 ; retract 0.2
    G4 S{filament_cost[current_extruder]} ; delay
    G1 E-0.0500 F50 ; retract
    G1 E-0.0500 F60 ; retract 0.3
    G4 S{filament_cost[current_extruder]} ; delay
    G1 E-0.0500 F50 ; retract
    G1 E-0.0500 F40 ; retract 0.4
    G4 S{filament_cost[current_extruder]} ; delay
    G1 E-0.0500 F30 ; retract
    G1 E-0.0500 F20 ; retract 0.5
    G4 S{filament_density[current_extruder]} ; hack -> Use filament density for the delay

    ; COOLING AND PARK FILAMENT
    G92 E0 ; reset extruder position
    G1 E-10.0000 F54000 ; park   -10
    G1 E-20.0000 F48000 ; park   -30
    G1 E-20.0000 F36000 ; park   -50
    G1 E-10.0000 F5000 ; park    -60
    G1 E10.0000 F60 ; cooling   -50
    G1 E-10.0000 F60 ; cooling  -60
    G1 E10.0000 F60 ; cooling   -50
    G1 E-10.0000 F60 ; cooling  -60
    G1 E40.0000 F60 ; cooling   -20
    G1 E-50.0000 F2400 ; cooling  -70
    G4 ; clear buffer movement

    ; TOOLCHANGE
    M117 Changing tool
    {if layer_num < 1}
        M104 S[first_layer_temperature[next_extruder]] ; set the extruder temperature - first layer
    {else}
        M104 S[temperature[next_extruder]] ; set the extruder temperature - other layers
    {endif}
    T[next_extruder] ; toolchange

    ; LOAD
    ; M117 Load and purge
    G92 E0 ; reset extruder position
    G1 E10 F500 ; load   10
    G1 E10 F2000 ; load  20
    G1 E20 F4000 ; load  40
    G1 E10 F2000 ; load  50
    G1 E10 F1000 ; load  60
    G1 E10 F500 ; load  70

    G1 E-[retract_length_toolchange[next_extruder]] F2400 ; retract to avoid leaks

    M220 R ; restore feedrate
    M400 ;
    {
        local drLength = 0 + (retract_length_toolchange[next_extruder] + retract_restart_extra_toolchange[next_extruder]) ;
    }

    G1 E{drLength} F2400 ; deretract

    G4 ; clear buffer movement
    M106 S{max_fan_speed[next_extruder] / 100 * 255} ; turn on fan
{else}
    ; no extruder change
{endif}