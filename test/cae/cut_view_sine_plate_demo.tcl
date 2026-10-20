# 36 unstructured hexes / 98 nodes. temperature = z; no fixture generation needed.
set sine_demo_dir [file dirname [file normalize [info script]]]
source [file join $sine_demo_dir viewdraw_runtime.tcl]
source [file join $sine_demo_dir cut_view_manager.tcl]
set sine_demo_file [file normalize [file join $sine_demo_dir .. native fixtures sine_plate_6x6.vtu]]
if {[info exists ::sine_plate_file]} { set sine_demo_file $::sine_plate_file }
vinit SinePlate -width 1100 -height 800
vsetbg -color WHITE
catch {scvcutview sine_plate clear}
scvcutview sine_plate read $sine_demo_file
# The default Y section crosses the full sine profile and has a varying field.
scvcutview sine_plate add cut 0 1 0 -0.5
scvcutview sine_plate field point temperature 0
scvcutview sine_plate visibility cut 7
scvcutview sine_plate renderbackend auto
scvcutview sine_plate display
vviewparams -proj 1 1 0.8 -up 0 0 1
scvcutview sine_plate fit
scvcutview sine_plate visibility cut 3
scvcutview sine_plate handle cut 1 0
scvcutmanager sine_plate
puts {Sine plate ready: model=sine_plate cut=cut; masks 2=section, 3=negative+section, 6=positive+section, 7=both+section.}
puts {Horizontal split: scvcutview sine_plate move cut 0 0 1 -0.225}
puts {Full sine profile: scvcutview sine_plate move cut 0 1 0 -0.5}
