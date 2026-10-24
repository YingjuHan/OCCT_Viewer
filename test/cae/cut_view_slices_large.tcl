set sl_dir [file dirname [file normalize [info script]]]
source [file join $sl_dir viewdraw_runtime.tcl]
proc sl_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set input [file normalize [file join $sl_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set input $::env(CUTVIEW_BENCHMARK_FILE) }
    vinit SlicesLarge -width 1000 -height 750
    scvcutview slices read $input
    sl_assert {[dict get [scvcutview slices info] cells]>=1000000} "Requires million-cell input"
    scvcutview slices add p 1 0 0 -75.25
    scvcutview slices field point temperature 0
    scvcutview slices display
    set start [clock microseconds]
    scvcutview slices slicing p range -20 20 3
    scvcutview slices sliceenable p_range 1
    set before [scvcutview slices snapshot]
    # Synthetic tensor mapping measures performance only, not physical stress.
    set request {{"cut":"p","field":{"name":"displacement","nodal":true,"component":0,"step":-1},"components":[0,1,2,0,1,2,0,1,2],"sumPoint":"centroid"}}
    set csv [scvcutview slices slicescan p_range $request 1]
    sl_assert {[llength [split [string trim $csv] \n]]==4} "Missing/partial scan results"
    sl_assert {[scvcutview slices snapshot] eq $before} "Scan changed scene"
    set budget [scvcutview slices cacheinfo]
    sl_assert {[dict get $budget resultReserved]<=[dict get $budget resultLimit]} "Scan exceeded budget"
    scvcutview slices sliceremove p_range
    puts "CUT_SLICES_LARGE_STATE elapsed_ms [expr {([clock microseconds]-$start)/1000.0}] $budget"; flush stdout
    sl_assert {[llength [scvcutview slices list]]==1} "Collection leaked"
    scvcutview slices clear
} message options]} { puts stderr "CUT_SLICES_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_SLICES_LARGE_PASS
exit 0
