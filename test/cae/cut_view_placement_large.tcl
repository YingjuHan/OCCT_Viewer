# Million-cell gate for the complete placement API. Release, without GDB.
set placement_large_dir [file dirname [file normalize [info script]]]
proc placement_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $placement_large_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale .1
    source [file join $placement_large_dir cut_view_deformation_demo.tcl]
    placement_large_assert {[dict get [scvcutview cut_modes info] cells] >= 1000000} "Requires million-cell input"
    lassign [scvcutview cut_modes bounds] x0 y0 z0 x1 y1 z1
    set pivot [list [expr {($x0+$x1)/2}] [expr {($y0+$y1)/2}] [expr {($z0+$z1)/2}]]
    scvcutview cut_modes placement cut {*}$pivot 0 1 0 1 0 0
    set preparations [dict get [scvcutview cut_modes stats] preparations]
    set budget [scvcutview cut_modes cacheinfo]
    foreach mode {cloud wireframe cloud_edges} {
        scvcutview cut_modes displaymode $mode
        foreach mask {2 3 7} {
            scvcutview cut_modes visibility cut $mask
            set initial [scvcutview cut_modes renderinfo]
            set begin [clock microseconds]
            for {set frame 0} {$frame < 4} {incr frame} {
                scvcutview cut_modes absrotate cut 2 [expr {2.0+.3*$frame}]
                scvcutview cut_modes translate cut [expr {(.001+.0003*$frame)*($y1-$y0)}]
                set stats [scvcutview cut_modes renderinfo]
                placement_large_assert {[dict get $stats backend] eq "gpu_plane" && [dict get $stats submitted_body_bytes] == 0
                    && [dict get $stats body_builds] == [dict get $initial body_builds]} "Placement rebuilt or uploaded body"
            }
            set current [scvcutview cut_modes cacheinfo]
            placement_large_assert {[dict get $current sourceReserved] == [dict get $budget sourceReserved]
                && [dict get $current resultReserved] <= [dict get $budget resultReserved]+1048576} "Placement grew retained budget"
            puts "CUT_PLACEMENT_LARGE_STATE $mode mask $mask elapsed_ms [expr {([clock microseconds]-$begin)/1000.0}] $current"; flush stdout
        }
    }
    placement_large_assert {[dict get [scvcutview cut_modes stats] preparations] == $preparations} "Motion rebuilt source"
    ::CutViewManager::close; scvcutview cut_modes clear
} message options]} {
    puts stderr "CUT_PLACEMENT_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1
}
puts CUT_PLACEMENT_LARGE_PASS
exit 0
