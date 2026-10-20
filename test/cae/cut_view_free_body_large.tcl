# Performance-only tensor mapping from displacement, NOT a physical stress result.
set fb_large_dir [file dirname [file normalize [info script]]]
proc fb_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $fb_large_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale 0
    source [file join $fb_large_dir cut_view_deformation_demo.tcl]
    fb_large_assert {[dict get [scvcutview cut_modes info] cells]>=1000000} "Requires million-cell input"
    scvcutview cut_modes visibility cut 2
    set json {{"cut":"cut","field":{"name":"displacement","nodal":true,"component":0,"step":-1},"components":[0,1,2,0,1,2,0,1,2],"sumPoint":"centroid","arrowScale":0.000001}}
    scvcutview cut_modes physics benchmark $json
    set begin [clock microseconds]
    foreach value {.49 .51 .47 .5} {
        scvcutview cut_modes move cut 1 0 0 [expr {-$value}]
        set report [scvcutview cut_modes physicsreport benchmark]
        fb_large_assert {[dict get $report overlays]<=2} "Default per-triangle AIS arrows"
        fb_large_assert {[dict get $report samples]>=10000} "Large section not integrated"
    }
    set cache [scvcutview cut_modes cacheinfo]
    fb_large_assert {[dict get $cache resultReserved]<=[dict get $cache resultLimit]} "Result budget exceeded"
    puts "CUT_FREE_BODY_LARGE_STATE elapsed_ms [expr {([clock microseconds]-$begin)/1000.0}] $report $cache"; flush stdout
    scvcutview cut_modes physicsenabled cut 0
    fb_large_assert {[catch {scvcutview cut_modes physicsreport benchmark}]} "Disabled large report retained"
    ::CutViewManager::close; scvcutview cut_modes clear
} message options]} { puts stderr "CUT_FREE_BODY_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_FREE_BODY_LARGE_PASS
exit 0
