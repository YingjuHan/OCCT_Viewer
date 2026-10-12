# N04 import outcome routing and failed-import live-model preservation.
set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set fixtures $::env(VTK_IMPORT_FIXTURES)
proc import_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}

if {[catch {
    scvcutview imported read [file join $fixtures tetra.vtu]
    set report [scvcutview imported importreport]
    import_assert {[dict get $report succeeded] && [dict get $report backend] eq "portable"} "Portable DRAW report"
    import_assert {[dict get $report points] == 4 && [dict get $report cells] == 1 && [dict get $report fields] == 2} "Published DRAW counts"
    import_assert {[dict get $report errors] eq {} && [llength [dict get $report attempts]] == 1} "Portable DRAW diagnostics"
    set before [scvcutview imported info]
    import_assert {[catch {scvcutview imported replace [file join $fixtures invalid.vtu]}]} "Invalid replace accepted"
    import_assert {[scvcutview imported info] eq $before && [scvcutview imported importreport] eq $report} "Failed cut replacement mutated live model/report"
    if {$::env(VTK_IMPORT_HAS_NATIVE)} {
        set copied [file join $fixtures {polyline with spaces.vtp}]
        file copy -force [file join $fixtures polyline.vtp] $copied
        scvcutview lines read $copied
        set fallback [scvcutview lines importreport]
        import_assert {[dict get $fallback source] eq $copied} "DRAW report source quoting"
        import_assert {[dict get $fallback succeeded] && [dict get $fallback backend] eq "native-vtk"} "Native DRAW outcome"
        import_assert {[dict get $fallback errors] eq {} && [llength [dict get $fallback attempts]] == 2} "Native DRAW attempts"
        set first [lindex [dict get $fallback attempts] 0]
        import_assert {![dict get $first succeeded] && [llength [dict get $first errors]] > 0} "Rejected DRAW attempt lost"
    } else {
        import_assert {[catch {scvcutview lines read [file join $fixtures polyline.vtp]}]} "OFF DRAW silently accepted Lines"
    }
    if {[info exists ::env(VTK_IMPORT_VISUAL)]} {
        set output $::env(VTK_IMPORT_OUTPUT)
        file mkdir $output
        vinit View1 -width 640 -height 480
        vbackground -color white
        scvreadmesh live [file join $fixtures tetra.vtu] -display 1
        vfit
        vrepaint
        set info_before [scvmeshinfo live]
        vdump [file join $output before.png]
        import_assert {[dict get [scvcutimagestats [file join $output before.png]] visible] > 500} "Ordinary Viewer reference image is blank"
        foreach file {absent.vtu invalid.vtu missing.vtm} {
            import_assert {[catch {scvreadmesh live [file join $fixtures $file] -display 1}]} "DRAW accepted invalid replacement"
            import_assert {[scvmeshinfo live] eq $info_before} "DRAW replacement changed original mesh"
            vrepaint
            vdump [file join $output after-$file.png]
            set delta [scvcutimagecompare [file join $output before.png] [file join $output after-$file.png]]
            import_assert {[dict get $delta changed] == 0} "Failed DRAW import removed or changed displayed old mesh"
        }
        scvreadmesh live [file join $fixtures tetra.vtu] -display 1
        vrepaint
        vdump [file join $output replaced.png]
    }
} reason]} {
    puts "VTK_IMPORT_DRAW_FAIL: $reason"
    exit 1
}
puts "VTK_IMPORT_DRAW_PASS"
exit 0
