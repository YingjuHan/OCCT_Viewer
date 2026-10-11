# Native display/drag regression. Requires an X display; writes PNG evidence.
set ::env(CUTVIEW_KEEP_RUNNING) 1
source [file join [file dirname [info script]] cut_view.tcl]
proc cut_check_image {path minimumPixels minimumColors} {
    vdump $path
    set stats [scvcutimagestats $path]
    cut_assert {[dict get $stats colored] > $minimumPixels && [dict get $stats colors] > $minimumColors} \
        "Cut view is blank, uncolored, or has an incomplete contour"
}

proc run_cut_view_visual {} {
    global cut_view_script_dir cut_view_fixtures
    pload VISUALIZATION
    vinit CutView -width 800 -height 600
    vsetbg -color WHITE
    set output [file normalize [file join $cut_view_script_dir .. .. build cut-view-images]]
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set output [file normalize $::env(CUTVIEW_OUTPUT_DIR)] }
    file mkdir $output
    scvcutview volume read [file join $cut_view_fixtures tetra.vtu]
    scvcutview volume add x 1 0 0 -0.25
    scvcutview volume field point temperature 0
    scvcutview volume display
    vaxo
    vfit
    for {set mask 0} {$mask < 8} {incr mask} {
        scvcutview volume visibility x $mask
        set path [file join $output volume_$mask.png]
        if {$mask == 0} {
            vdump $path
            cut_assert {[dict get [scvcutimagestats $path] visible] == 0} "Hidden combination is not blank"
        } else { cut_check_image $path 500 20 }
    }
    scvcutview volume visibility x 3
    for {set frame 1} {$frame <= 9} {incr frame} {
        scvcutview volume move x 1 0 0 [expr {-$frame/10.0}]
        cut_check_image [file join $output drag_$frame.png] 500 20
    }
    scvcutview volume visibility x 2
    scvcutview volume move x 1 0 0 -0.25
    scvcutview volume field cell pressure 0
    vfit
    cut_check_image [file join $output volume_cell_contour.png] 500 0
    scvcutview volume clear

    scvcutview projected read [file join $cut_view_fixtures two_tetra.vtu]
    scvcutview projected add x 1 0 0 -0.25
    scvcutview projected field cell pressure 0
    scvcutview projected display
    vaxo
    vfit
    cut_check_image [file join $output cell_constant.png] 500 0
    scvcutview projected cellmode x projected
    cut_check_image [file join $output cell_projected.png] 500 20
    scvcutview projected add y 0 1 0 -0.25
    cut_check_image [file join $output multiple_sections.png] 500 20
    scvcutview projected clear

    scvcutview surface read [file join $cut_view_fixtures surface.vtp]
    scvcutview surface add x 1 0 0 -0.25
    scvcutview surface field point temperature 0
    scvcutview surface display
    vtop
    vfit
    cut_check_image [file join $output surface_point_contour.png] 100 20
    scvcutview surface field cell pressure 0
    cut_check_image [file join $output surface_cell_contour.png] 100 0
    scvcutview surface clear
    source [file join $cut_view_script_dir cut_view_demo.tcl]
    cut_check_image [file join $output demo.png] 500 20
    scvcutview cut_view_demo move x 1 0 0 -0.5
    source [file join $cut_view_script_dir cut_view_demo.tcl]
    cut_check_image [file join $output demo_repeat.png] 500 20
    cut_near [dict get [scvcutview cut_view_demo eval x] section area] 0.28125 "Repeated demo reset"
    scvcutview cut_view_demo clear
    puts "CUT_VIEW_VISUAL_PASS: $output"
}
if {[catch {run_cut_view_visual} message options]} {
    puts stderr "CUT_VIEW_VISUAL_FAIL: $message"
    puts stderr [dict get $options -errorinfo]
    exit 1
}
exit 0
