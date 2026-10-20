set auto_large_dir [file dirname [file normalize [info script]]]
source [file join $auto_large_dir viewdraw_runtime.tcl]
proc al_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set input [file join $auto_large_dir .. .. build large-vtk-deformed hex_150.vtu]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set input $::env(CUTVIEW_BENCHMARK_FILE) }
    vinit AutomaticLarge -width 900 -height 650
    scvcutview auto_large read $input
    al_assert {[dict get [scvcutview auto_large info] cells]>=1000000} "Requires million-cell data"
    scvcutview auto_large field point temperature 0
    set start [clock microseconds]
    foreach threshold {450.25 450.75} {
        set metadata [format {{"version":1,"cuts":[{"id":"density","kind":"density","field":{"name":"temperature","nodal":true,"component":0,"step":-1},"threshold":%s,"min":0,"max":900,"sign":1,"visibility":2,"enabled":true}]}} $threshold]
        scvcutview auto_large automaticsync benchmark $metadata
        scvcutview auto_large display
        puts "CUT_AUTOMATIC_LARGE_STATE $threshold [scvcutview auto_large cacheinfo]"; flush stdout
    }
    scvcutview auto_large automaticsync benchmark {{"version":1,"cuts":[]}}
    al_assert {[llength [scvcutview auto_large list]]==0} "Automatic lifecycle leaked definitions"
    puts "CUT_AUTOMATIC_LARGE_BATCH_MS [expr {([clock microseconds]-$start)/1000.0}]"
    scvcutview auto_large clear
} message options]} { puts stderr "CUT_AUTOMATIC_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_AUTOMATIC_LARGE_PASS
exit 0
