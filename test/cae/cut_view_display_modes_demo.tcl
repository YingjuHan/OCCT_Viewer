# Interactive cloud / wireframe / cloud+edges, with optional VTK/VTU input.
# set ::cut_modes_file D:/workspace/OpenCASCADE/OCCT_Viewer/build/large-vtk/hex_150.vtu
# source D:/workspace/OpenCASCADE/OCCT_Viewer/test/cae/cut_view_display_modes_demo.tcl
set cut_modes_dir [file dirname [file normalize [info script]]]
source [file join $cut_modes_dir viewdraw_runtime.tcl]
source [file join $cut_modes_dir cut_view_manager.tcl]
if {![info exists ::cut_modes_file]} {
    set ::cut_modes_file [file normalize [file join $cut_modes_dir .. native fixtures sine_plate_6x6.vtu]]
}
if {![file isfile $::cut_modes_file]} { error "Missing mesh: $::cut_modes_file" }
vinit CutDisplayModes -width 1100 -height 800
vsetbg -color WHITE
catch {scvcutview cut_modes clear}
scvcutview cut_modes read $::cut_modes_file
lassign [scvcutview cut_modes bounds] xmin ymin zmin xmax ymax zmax
scvcutview cut_modes add cut 0 1 0 [expr {-($ymin+$ymax)/2.0}]
# Thin edges preserve cloud pixels on dense grids at the initial zoom.
if {![info exists ::cut_modes_width]} { set ::cut_modes_width 1 }
foreach portion {0 1 2} { scvcutview cut_modes cutstyle defaults $portion 0 $::cut_modes_width 2 }
set cut_modes_field ""
foreach field [scvcutview cut_modes fields] {
    if {[dict get $field mapping] ne "point"} { continue }
    if {$cut_modes_field eq "" || [dict get $field name] eq "temperature"} { set cut_modes_field $field }
    if {[dict get $field name] eq "temperature"} { break }
}
if {$cut_modes_field ne ""} {
    set command [list scvcutview cut_modes field point [dict get $cut_modes_field name] 0]
    if {[dict get $cut_modes_field step] >= 0} { lappend command [dict get $cut_modes_field step] }
    {*}$command
}
scvcutview cut_modes displaymode cloud_edges
scvcutview cut_modes visibility cut 7
scvcutview cut_modes renderbackend auto
scvcutview cut_modes display
vviewparams -proj 1 1 0.8 -up 0 0 1
scvcutview cut_modes fit
scvcutview cut_modes visibility cut 3
scvcutview cut_modes handle cut 1 0
scvcutmanager cut_modes
puts {Display in Manager: cloud / wireframe / cloud_edges. Drag the plane or use the position slider.}
puts {scvcutview cut_modes displaymode wireframe}
puts {scvcutview cut_modes displaymode cloud}
puts {scvcutview cut_modes displaymode cloud_edges}
puts {Masks: 2=section only, 3=negative+section, 6=positive+section, 5=uncut body, 7=uncut body+section.}
puts [scvcutview cut_modes renderinfo]
