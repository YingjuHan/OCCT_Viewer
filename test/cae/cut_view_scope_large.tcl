set scope_large_dir [file dirname [file normalize [info script]]]
proc sl_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file join $scope_large_dir .. .. build large-vtk-deformed hex_150.vtu]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale .1
    source [file join $scope_large_dir cut_view_deformation_demo.tcl]
    sl_assert {[dict get [scvcutview cut_modes info] cells]>=1000000} "Requires million-cell source"
    scvcutview cut_modes visibility cut 3
    scvcutview scope_large share cut_modes
    vinit Driver1/Viewer1/ScopeLarge -width 800 -height 600
    lassign [scvcutview scope_large bounds] xmin ymin zmin xmax ymax zmax
    scvcutview scope_large add b 0 0 1 [expr {-($zmin+$zmax)/2}]
    scvcutview scope_large visibility b 3
    scvcutview scope_large field point temperature 0
    scvcutview scope_large display; vaxo; scvcutview scope_large fit
    sl_assert {[scvcutview scope_large viewid]!=[scvcutview cut_modes viewid]} "Views were not distinct"
    set second [scvcutview scope_large snapshot]
    set budgetA [scvcutview cut_modes cacheinfo]; set budgetB [scvcutview scope_large cacheinfo]
    set before [scvcutview cut_modes renderinfo]
    set start [clock microseconds]
    foreach offset {.501 .502 .503 .504} {
        scvcutview cut_modes move cut 0 1 0 [expr {-($ymin+$offset*($ymax-$ymin))}]
        set now [scvcutview cut_modes renderinfo]
        sl_assert {[dict get $now backend] eq "gpu_plane" && [dict get $now body_builds]==[dict get $before body_builds]
            && [dict get $now submitted_body_bytes]==0} "Scoped resident-body path regressed"
        sl_assert {[scvcutview scope_large snapshot] eq $second && [scvcutview scope_large cacheinfo] eq $budgetB} "A edit mutated B cut/cache state"
    }
    sl_assert {[dict get [scvcutview cut_modes cacheinfo] sourceReserved]==[dict get $budgetA sourceReserved]} "Scoped edits rebuilt source"
    puts "CUT_SCOPE_LARGE_BATCH_MS [expr {([clock microseconds]-$start)/1000.0}]"
    puts "CUT_SCOPE_LARGE_A [scvcutview cut_modes cacheinfo]"
    puts "CUT_SCOPE_LARGE_B [scvcutview scope_large cacheinfo]"
    ::CutViewManager::close; scvcutview scope_large clear; scvcutview cut_modes clear
} message options]} { puts stderr "CUT_SCOPE_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_SCOPE_LARGE_PASS
exit 0
