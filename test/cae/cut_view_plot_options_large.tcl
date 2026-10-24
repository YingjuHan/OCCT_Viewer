# Explicit million-cell acceptance; not included in default CTest.
set plot_large_dir [file dirname [file normalize [info script]]]
proc plot_large_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set ::cut_deformation_file [file normalize [file join $plot_large_dir .. .. build large-vtk-deformed hex_150.vtu]]
    if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} { set ::cut_deformation_file $::env(CUTVIEW_BENCHMARK_FILE) }
    set ::cut_deformation_scale .1
    source [file join $plot_large_dir cut_view_deformation_demo.tcl]
    plot_large_assert {[dict get [scvcutview cut_modes info] cells] >= 1000000} "Requires at least one million cells"
    set baseline [scvcutview cut_modes cacheinfo]
    set initialSource [dict get [scvcutview cut_modes stats] preparations]
    foreach mode {cloud wireframe cloud_edges} {
        scvcutview cut_modes displaymode $mode
        foreach mask {2 3 7} {
            scvcutview cut_modes visibility cut $mask
            set currentMode [dict get [scvcutview cut_modes cutstyle cut 0] mode]
            scvcutview cut_modes cutstyle plot 0 0 1 $currentMode .2 .4 .8 .8 .1 .2 0 0 0
            ::CutViewManager::refresh
            update
            set beforeOpen [scvcutview cut_modes renderinfo]
            .scvcutmanager.options invoke
            plot_large_assert {$::CutViewOptions::status eq ""} "Opening options failed"
            plot_large_assert {[scvcutview cut_modes renderinfo] eq $beforeOpen} "Opening options rendered or changed the backend"
            update
            .scvcutoptions.tabs select 1
            .scvcutoptions.scope set "Selected cut override"
            event generate .scvcutoptions.scope <<ComboboxSelected>>
            update
            set ::CutViewOptions::values(1,override) 1
            set ::CutViewOptions::values(1,r) .2
            set ::CutViewOptions::values(1,g) .4
            set ::CutViewOptions::values(1,b) .8
            set ::CutViewOptions::values(1,fieldColor) 0
            set ::CutViewOptions::values(1,inheritPlot) 1
            .scvcutoptions.actions.apply invoke
            plot_large_assert {$::CutViewOptions::status eq ""} "Applying options failed: $::CutViewOptions::status"
            set initial [scvcutview cut_modes renderinfo]
            plot_large_assert {[dict get $initial backend] eq "gpu_plane"} "Style-only update lost the GPU path"
            lassign [scvcutview cut_modes bounds] xmin ymin zmin xmax ymax zmax
            for {set frame 0} {$frame < 4} {incr frame} {
                scvcutview cut_modes move cut 0 1 0 [expr {-($ymin+(.501+.0007*$frame)*($ymax-$ymin))}]
                set stats [scvcutview cut_modes renderinfo]
                plot_large_assert {[dict get $stats body_builds] == [dict get $initial body_builds]
                    && [dict get $stats submitted_body_bytes] == 0} "Options broke resident-body drag"
            }
            set ::CutViewOptions::values(1,override) 0
            .scvcutoptions.actions.apply invoke
            plot_large_assert {$::CutViewOptions::status eq "" && ![dict get [scvcutview cut_modes cutstyle cut 1] override]} "Reset override failed"
            .scvcutoptions.actions.close invoke
            set budget [scvcutview cut_modes cacheinfo]
            plot_large_assert {[dict get $budget sourceReserved] == [dict get $baseline sourceReserved]
                && [dict get $budget resultReserved] <= [dict get $baseline resultReserved]+1048576} "Options grew retained geometry budget"
            puts "CUT_PLOT_OPTIONS_LARGE_STATE $mode mask $mask $budget"; flush stdout
        }
    }
    # Independent rename must not rebuild the deformed source or edit pending geometry.
    set plane [dict get [lindex [scvcutview cut_modes list] 0] plane]
    set ::CutViewManager::positionText nonsense
    .scvcutmanager.actions.rename invoke
    set ::CutViewRename::name {Large deformed {cut}}
    .scvcutrename.apply invoke
    plot_large_assert {![winfo exists .scvcutrename] && [dict get [lindex [scvcutview cut_modes list] 0] plane] eq $plane} "Rename changed plane"
    plot_large_assert {[dict get [scvcutview cut_modes stats] preparations] == $initialSource} "Options or rename rebuilt the full source"
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vdump [file join $::env(CUTVIEW_OUTPUT_DIR) plot-options-large.png]
    }
    puts "CUT_PLOT_OPTIONS_LARGE_CACHE [scvcutview cut_modes cacheinfo]"
    ::CutViewManager::close
    scvcutview cut_modes clear
} message options]} {
    puts stderr "CUT_PLOT_OPTIONS_LARGE_FAIL $message\n[dict get $options -errorinfo]"
    exit 1
}
puts CUT_PLOT_OPTIONS_LARGE_PASS
exit 0
