# Million-cell material cap gate. Never use a debugger for this timing.
set material_large_dir [file dirname [file normalize [info script]]]
proc material_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $material_large_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale 0
    source [file join $material_large_dir cut_view_deformation_demo.tcl]
    material_large_assert {[dict get [scvcutview cut_modes info] cells]>=1000000} "Requires million-cell input"
    # Release the old GPU body before building a distinct reference configuration.
    scvcutview cut_modes visibility cut 2
    scvcutview cut_modes renderbackend cpu
    scvcutview cut_modes material cut 1 Current
    set reference [dict get [scvcutview cut_modes materialstats cut] referenceBuilds]
    set limit [dict get [scvcutview cut_modes cacheinfo] resultLimit]
    foreach mode {cloud wireframe cloud_edges} {
        scvcutview cut_modes displaymode $mode
        set begin [clock microseconds]
        foreach scale {.05 .1 -.05 0} { scvcutview cut_modes deform displacement $scale }
        set state [scvcutview cut_modes materialstats cut]
        set budget [scvcutview cut_modes cacheinfo]
        material_large_assert {[dict get $state referenceBuilds]==$reference} "Rebuilt million-cell reference on scale change"
        material_large_assert {[dict get $budget resultReserved]<=$limit} "Exceeded result budget"
        puts "CUT_MATERIAL_LARGE_STATE $mode elapsed_ms [expr {([clock microseconds]-$begin)/1000.0}] $state $budget"; flush stdout
    }
    ::CutViewManager::close; scvcutview cut_modes clear
} message options]} { puts stderr "CUT_MATERIAL_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_MATERIAL_LARGE_PASS
exit 0
