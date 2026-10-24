# Release performance gate; requires the generated 150^3 hexahedron input.
set il_dir [file dirname [file normalize [info script]]]
proc il_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $il_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale 0
    source [file join $il_dir cut_view_deformation_demo.tcl]
    il_assert {[dict get [scvcutview cut_modes info] cells]>=1000000} "Requires million-cell input"
    scvcutview cut_modes remove cut
    set begin [clock microseconds]
    scvcutview cut_modes isoadd iso point temperature 0 450.25
    scvcutview cut_modes visibility iso 2
    set binding [scvcutview cut_modes isobinding iso]
    set first [dict get [scvcutview cut_modes eval iso] section cells]
    il_assert {$first>=20000} "Large isosurface was not generated"
    scvcutview cut_modes recovery 25 1 none
    scvcutview cut_modes field point displacement 0
    il_assert {[scvcutview cut_modes isobinding iso] eq $binding} "Cloud changed iso snapshot"
    scvcutview cut_modes isovalue iso 450.75
    set second [dict get [scvcutview cut_modes eval iso] section cells]
    il_assert {$second>=20000} "Moved isosurface was not generated"
    set cache [scvcutview cut_modes cacheinfo]
    il_assert {[dict get $cache resultReserved]<=[dict get $cache resultLimit]} "Result budget exceeded"
    puts "CUT_ISO_BINDING_LARGE_STATE elapsed_ms [expr {([clock microseconds]-$begin)/1000.0}] first {$first} second {$second} $cache"; flush stdout
    ::CutViewManager::close; scvcutview cut_modes clear
} message options]} { puts stderr "CUT_ISO_BINDING_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_ISO_BINDING_LARGE_PASS
exit 0
