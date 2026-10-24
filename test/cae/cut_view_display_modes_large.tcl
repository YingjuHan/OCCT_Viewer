# Million-cell acceptance for the exact user-facing demo and manager startup.
# Run DRAW -i -f with optional CUTVIEW_BENCHMARK_FILE / CUTVIEW_OUTPUT_DIR.
set modes_large_dir [file dirname [file normalize [info script]]]
proc modes_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_modes_file [file normalize [file join $modes_large_dir .. .. build large-vtk hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_modes_file $::env(CUTVIEW_BENCHMARK_FILE) }
    source [file join $modes_large_dir cut_view_display_modes_demo.tcl]
    modes_large_assert {[dict get [scvcutview cut_modes info] cells] >= 1000000} "Large acceptance requires at least one million cells"
    set budget [scvcutview cut_modes cacheinfo]
    set reserved [dict get $budget resultReserved]
    lassign [scvcutview cut_modes bounds] xmin ymin zmin xmax ymax zmax
    for {set pass 0} {$pass < 3} {incr pass} {
        set revision [dict get [scvcutview cut_modes presentation cut] revision]
        scvcutmanager cut_modes
        update
        modes_large_assert {[dict get [scvcutview cut_modes presentation cut] revision] == $revision} "Manager reopen invalidated the active field"
        foreach mode {wireframe cloud cloud_edges} {
            scvcutview cut_modes displaymode $mode
            modes_large_assert {[dict get [scvcutview cut_modes renderinfo] backend] eq "gpu_plane"} "Mode left GPU path"
        }
        # Explicit rebind remains supported, including switching field mapping.
        set builds [dict get [scvcutview cut_modes renderinfo] body_builds]
        foreach field [scvcutview cut_modes fields] {
            if {[dict get $field mapping] eq "cell" && [dict get $field components] == 1} {
                scvcutview cut_modes field cell [dict get $field name] 0 [dict get $field step]
                break
            }
        }
        scvcutview cut_modes field point temperature 0
        scvcutview cut_modes field point temperature 0
        modes_large_assert {[dict get [scvcutview cut_modes renderinfo] body_builds] == $builds} "Field edit rebuilt body geometry"
        modes_large_assert {[dict get [scvcutview cut_modes cacheinfo] resultReserved] == $reserved} "Field/mode edits grew resident reservations"
    }
    foreach mask {2 3 7} {
        scvcutview cut_modes visibility cut $mask
        set initial [scvcutview cut_modes renderinfo]
        for {set frame 0} {$frame < 5} {incr frame} {
            scvcutview cut_modes move cut 0 1 0 [expr {-($ymin+(.501+.0007*$frame)*($ymax-$ymin))}]
            set stats [scvcutview cut_modes renderinfo]
            modes_large_assert {[dict get $stats submitted_body_bytes] == 0
                && [dict get $stats body_builds] == [dict get $initial body_builds]} "Warm plane move uploaded body"
        }
    }
    puts "CUT_MODES_LARGE_CACHE [scvcutview cut_modes cacheinfo]"
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vdump [file join $::env(CUTVIEW_OUTPUT_DIR) display-modes-large.png]
    }
    ::CutViewManager::close
    scvcutview cut_modes clear
} message options]} { puts stderr "CUT_MODES_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_MODES_LARGE_PASS
exit 0
