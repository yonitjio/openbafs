; toolchange from [current_extruder] to [next_extruder], layer [layer_num]
{if current_extruder != next_extruder}
    M900 K0 ; disable linear advance
    G60 S0 ; save position in slot 0
    M220 B ; save feedrate
    M220 S100 ; set feedrate

    G90 ; absolute movement
    M83 ; E relative
    M106 S0 ; turn off fan
    G92 E0 ; reset extruder position
    G1 E-0.5 F2100 ; retract
    M400 ; clear buffer movement
    G1 X210 F12000 ; move to purge area
    G1 X220 F2000
    G1 X230 F1000
    M400 ; clear buffer movement

    ; FILAMENT TIP SHAPING
    G1 E0.5 F600 ; deretract
    G1 E{filament_cost[current_extruder]} F200 ; hack -> Use filament cost for deretract
    G4 S{filament_density[current_extruder]} ; hack -> Use filament density for the delay
    M400 ; clear buffer movement

    ; COOLING AND PARK FILAMENT
    M117 Cooling and parking
    G92 E0 ; reset extruder position
    G1 E-15.0000 F18000 ; park
    G1 E-35.0000 F5400 ; park
    G1 E-10.0000 F2700 ; park
    G1 E-5.0000 F1620 ; park
    G1 E20.0000 F361 ; cooling
    G1 E-20.0000 F558 ; cooling
    G1 E-10.0000 F2000 ; cooling
    G1 E-10 F4200 ; park
    M400 ; clear buffer movement

    ; TOOLCHANGE
    M117 Changing tool
    {if layer_num < 1}
        M104 S[first_layer_temperature[next_extruder]] ; set the extruder temperature - first layer
    {else}
        M104 S[temperature[next_extruder]] ; set the extruder temperature - other layers
    {endif}
    T[next_extruder] ; toolchange

    ; LOAD
    M117 Load and purge
    G92 E0 ; reset extruder position
    G1 E12.4000 F180 ; load
    G1 E43.4000 F3000 ; load
    G1  E6.2000 F2100 ; load

    ; PURGE
    ; hack -> use filament_minimal_purge_on_wipe_tower as base for purging
    ; hack -> use filament_spool_weight for addition factor
    {
        local frad = 0 + (filament_diameter[current_extruder]/2);
        local farea = 0 + (3.14 * (frad * frad)* extrusion_multiplier[current_extruder]);
        local epurge = 0 + (filament_minimal_purge_on_wipe_tower[current_extruder] / farea);

        local fspeed = 0 + (filament_max_volumetric_speed[current_extruder] / farea);
        local fpurge = 0 + ((filament_max_volumetric_speed[current_extruder] > 0) ? fspeed * 60 : 200);

        local purgeFactor = 60 * ((filament_spool_weight[current_extruder] - filament_spool_weight[next_extruder]) / 100);
        local finalPurge = 0 + (epurge + purgeFactor);
    }
    ; Purge Factor: {purgeFactor}
    ; Purge: {epurge}
    G1 E{finalPurge} F{fpurge}

    G92 E0 ; reset extruder position

    G1 E-[retract_length_toolchange[next_extruder]] F3200 ; retract to avoid leaks

    ; WIPE
    G1 X220 F2000
    G1 X227 F2000
    G1 X220 F2000
    G1 X227 F2000
    G1 X220 F2000
    G1 X227 F2000

    G90; absolute movement;
    M83 ; E relative

    M220 R ; restore feedrate
    G61 S0 X Y Z F12000; restore position

    G1 E[retract_length_toolchange[next_extruder]] F3200 ; deretract

    M400 ; clear buffer movement
{else}
    ; no extruder change
{endif}