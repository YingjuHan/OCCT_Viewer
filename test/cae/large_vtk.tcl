# Generated binary formats, source counts, FE mapping and resident plane frames.
# Small by default in CTest; set CUTVIEW_LARGE_VTK_DIR/GRID for manual large runs.
source [file join [file dirname [info script]] viewdraw_runtime.tcl]
pload MODELING VISUALIZATION
if {[catch {
    set directory $::env(CUTVIEW_LARGE_VTK_DIR)
    set largeGrid $::env(CUTVIEW_LARGE_VTK_GRID)
    set frames 3
    if {[info exists ::env(CUTVIEW_LARGE_VTK_FRAMES)]} { set frames $::env(CUTVIEW_LARGE_VTK_FRAMES) }
    vinit LargeVtk -width 900 -height 650
    vsetbg -color WHITE
    vaxo
    foreach extension {vtk vtu} {
        set fixture [file join $directory hex_$largeGrid.$extension]
        set counts [scvcutview imported read $fixture]
        if {[dict get $counts cells] != $largeGrid**3 || [dict get $counts points] != ($largeGrid+1)**3 || [dict get $counts fields] != 3} {
            error "Wrong imported counts: $fixture $counts"
        }
        set report [scvcutview imported importreport]
        if {![dict get $report succeeded] || [dict get $report backend] ne "portable"} { error "Self-contained import failed: $report" }
        scvcutview imported clear
        foreach mask {2 3 5 7} {
            set output [file join $::env(CUTVIEW_OUTPUT_DIR) $extension $mask]
            set result [scvcutfilebenchmark $fixture $frames crossing $output $mask auto]
            puts "LARGE_VTK_RESULT $extension $result"
            if {[dict get $result cells] != $largeGrid**3 || ![dict get $result original_FE] || ![dict get $result actual_redraw]} {
                error "File rendering/FE check failed"
            }
        }
        foreach mode {rotating deformed} {
            set result [scvcutfilebenchmark $fixture $frames $mode [file join $::env(CUTVIEW_OUTPUT_DIR) $extension $mode] 3 auto]
            puts "LARGE_VTK_RESULT $extension $result"
        }
    }
} message options]} { puts stderr "LARGE_VTK_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts LARGE_VTK_PASS
exit 0
