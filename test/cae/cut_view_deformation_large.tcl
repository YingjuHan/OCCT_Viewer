# Million-cell acceptance; default input is generated with --deformation.
set deform_large_dir [file dirname [file normalize [info script]]]
proc deform_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $deform_large_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale .1
    source [file join $deform_large_dir cut_view_deformation_demo.tcl]
    deform_large_assert {[dict get [scvcutview cut_modes info] cells] >= 1000000} "Requires at least one million cells"
    set reserved [dict get [scvcutview cut_modes cacheinfo] resultReserved]
    set sourceReserved [dict get [scvcutview cut_modes cacheinfo] sourceReserved]
    foreach scale {.1 -.1 0} {
        scvcutview cut_modes deform displacement $scale
        ::CutViewManager::refresh 1
        set state [scvcutview cut_modes deform]; set k [dict get $state effectiveScale]
        lassign [scvcutview cut_modes bounds] xmin ymin zmin xmax ymax zmax
        foreach mode {cloud wireframe cloud_edges} {
            scvcutview cut_modes displaymode $mode
            foreach mask {2 3 6 7} {
                scvcutview cut_modes visibility cut $mask
                set initial [scvcutview cut_modes renderinfo]
                for {set frame 0} {$frame < 5} {incr frame} {
                    scvcutview cut_modes move cut 0 1 0 [expr {-($ymin+(.501+.0007*$frame)*($ymax-$ymin))}]
                    set stats [scvcutview cut_modes renderinfo]
                    deform_large_assert {[dict get $stats backend] eq "gpu_plane" && [dict get $stats submitted_body_bytes] == 0
                        && [dict get $stats body_builds] == [dict get $initial body_builds]} "Deformed warm plane move uploaded body: $scale $mode $mask"
                }
                set probe [scvcutview cut_modes probe cut 1 0 1]
                lassign [dict get $probe position] x y z
                # Inverse of generator's affine displacement, using effective scale.
                set refX [expr {($x-.02*$k*$y)/(1-.0002*$k*$k)}]
                set refY [expr {($y-.01*$k*$x)/(1-.0002*$k*$k)}]
                set refZ [expr {$z/(1+.005*$k)}]
                deform_large_assert {abs([lindex [dict get $probe values] 0]-($refX+2*$refY+3*$refZ)) < .001} "Large deformed FE cloud mismatch"
            }
        }
        set budget [scvcutview cut_modes cacheinfo]
        deform_large_assert {[dict get $budget sourceReserved] == $sourceReserved} "Deformation accumulated source reservations"
        deform_large_assert {[dict get $budget resultReserved] <= $reserved+1048576} "Deformation accumulated result reservations"
        puts "CUT_DEFORMATION_LARGE_STATE scale $scale $budget"; flush stdout
    }
    # Hide the body during an epoch change, then restore its deformed edges.
    scvcutview cut_modes visibility cut 2
    scvcutview cut_modes deform displacement .1
    scvcutview cut_modes visibility cut 3
    scvcutview cut_modes move cut 0 1 0 -75.37
    deform_large_assert {[dict get [scvcutview cut_modes renderinfo] submitted_body_bytes] == 0} "Restored body uploads on next plane move"
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vdump [file join $::env(CUTVIEW_OUTPUT_DIR) deformation-large.png]
    }
    ::CutViewManager::close; scvcutview cut_modes clear
} message options]} { puts stderr "CUT_DEFORMATION_LARGE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_DEFORMATION_LARGE_PASS
exit 0
