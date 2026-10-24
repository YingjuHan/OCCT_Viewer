# A02 semantic and actual Manager-widget acceptance.
set placement_dir [file dirname [file normalize [info script]]]
proc placement_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc placement_cut {} {
    foreach cut [scvcutview cut_view_demo list] { if {[dict get $cut id] eq "x"} { return $cut } }
    error "Missing x"
}
proc placement_pump {} { update idletasks; update }
proc bgerror {message} { puts stderr "CUT_VIEW_PLACEMENT_FAIL $message\n$::errorInfo"; exit 1 }
if {[catch {
    source [file join $placement_dir cut_view_demo.tcl]
    set model cut_view_demo
    # Key-in permits arbitrary tangential pivot positions and a custom axis2.
    .scvcutmanager.controls.definition invoke
    foreach key {origin normal axis2} vector {{.4 .3 .2} {1 0 0} {0 0 1}} {
        foreach i {0 1 2} { set ::CutViewPlacement::values($key,$i) [lindex $vector $i] }
    }
    .scvcutplacement.body.apply invoke
    placement_pump
    placement_assert {$::CutViewPlacement::status eq ""} "Key-in failed: $::CutViewPlacement::status"
    foreach actual [dict get [placement_cut] origin] expected {.4 .3 .2} { placement_assert {abs($actual-$expected) < 1e-12} "Pivot lost: [placement_cut]" }
    set old [scvcutview $model snapshot]
    set ::CutViewPlacement::values(axis2,0) 1
    .scvcutplacement.body.apply invoke
    placement_assert {$::CutViewPlacement::status ne "" && [scvcutview $model snapshot] eq $old} "Nonorthogonal axes changed state"
    .scvcutplacement.body.close invoke
    # Oblique guides crossing five/six box edges used to overflow the SDK fan.
    set beforeOblique [scvcutview $model snapshot]
    scvcutview $model placement x .76704980842911874 -.083524904214559395 -.083524904214559395 1 1 1 -1 2 -1
    scvcutview $model absrotate x 2 20
    scvcutview $model mouse move 400 300
    scvcutview $model restore $beforeOblique
    foreach sensitivity {10 100 1} {
        .scvcutmanager.controls.sensitivity set $sensitivity
        event generate .scvcutmanager.controls.sensitivity <<ComboboxSelected>>
        placement_pump
        placement_assert {[scvcutview $model snapshot] eq $old} "Sensitivity changed geometry"
        set width [expr {[.scvcutmanager.controls.translate cget -to]-[.scvcutmanager.controls.translate cget -from]}]
        if {$sensitivity > 1} { placement_assert {abs($width-1.0/$sensitivity) < 1e-6} "Precision range wrong" }
    }
    # Inactive selection: numeric edits remain valid, both sliders are disabled.
    scvcutview $model enable x 0
    ::CutViewManager::refresh
    placement_assert {[.scvcutmanager.controls.translate cget -state] eq "disabled" && [.scvcutmanager.controls.rotate cget -state] eq "disabled"} "Inactive sliders enabled"
    set ::CutViewManager::positionText .05
    .scvcutmanager.controls.apply invoke
    placement_assert {$::CutViewManager::status eq "" && ![dict get [placement_cut] enabled]} "Numeric editing activated cut"
    scvcutview $model enable x 1
    ::CutViewManager::refresh
    set old [scvcutview $model snapshot]
    ::CutViewManager::sliderBegin
    ::CutViewManager::translate .15
    ::CutViewManager::sliderAbort
    placement_assert {[scvcutview $model snapshot] eq $old} "Translation Escape lost full frame"
    .scvcutmanager.controls.motion set Rotate
    event generate .scvcutmanager.controls.motion <<ComboboxSelected>>
    set ::CutViewManager::axis 2
    set ::CutViewManager::positionText 90
    .scvcutmanager.controls.apply invoke
    placement_pump
    set cut [placement_cut]
    placement_assert {abs([lindex [dict get $cut plane] 1]-1) < 1e-6 && abs([lindex [dict get $cut plane] 3]+.3) < 1e-6 && [dict get $cut offset] == 0} "Absolute pivot rotation did not ignore translation"
    set rotated [scvcutview $model snapshot]
    ::CutViewManager::rotate 90
    placement_assert {[scvcutview $model snapshot] eq $rotated} "Angle accumulated"
    ::CutViewManager::sliderBegin rotate
    ::CutViewManager::rotate 45
    ::CutViewManager::sliderAbort
    placement_assert {[scvcutview $model snapshot] eq $rotated} "Rotation Escape lost full frame"
    set handle [scvcutview $model handleinfo]
    foreach a [dict get $handle origin] b {.4 .3 .2} { placement_assert {abs($a-$b) < 1e-6} "Gizmo ignored definition origin" }
    # From CSYS uses the selected normal and the next cyclic tangent axis.
    scvcutview $model datum custom .2 .3 .4 0 1 0 -1 0 0 0 0 1
    .scvcutmanager.controls.definition invoke
    set ::CutViewPlacement::method "From CSYS"; set ::CutViewPlacement::datum custom; set ::CutViewPlacement::normalAxis 2
    .scvcutplacement.body.apply invoke
    placement_pump
    placement_assert {$::CutViewPlacement::status eq ""} "CSYS failed"
    set cut [placement_cut]
    placement_assert {[dict get $cut normal] eq "-1 0 0" && [dict get $cut axis2] eq "0 0 1"} "CSYS axes wrong"
    .scvcutplacement.body.close invoke
    # Full session owns the complete definition frame and absolute motion.
    scvcutview $model absrotate x 3 25
    scvcutview $model translate x .08
    set saved [scvcutview $model snapshot]
    set output [file join $placement_dir .. .. build cut-placement-session.json]
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { file mkdir $::env(CUTVIEW_OUTPUT_DIR); set output [file join $::env(CUTVIEW_OUTPUT_DIR) placement-session.json] }
    scvcutview $model sessionsave $output
    scvcutview $model absrotate x 2 70
    scvcutview $model sessionload $output
    ::CutViewManager::refresh
    placement_pump
    placement_assert {[scvcutview $model snapshot] eq $saved} "Full session lost placement: before $saved after [scvcutview $model snapshot]"
    ::CutViewManager::close
    scvcutview $model clear
} message options]} {
    puts stderr "CUT_VIEW_PLACEMENT_FAIL $message\n[dict get $options -errorinfo]"
    exit 1
}
puts CUT_VIEW_PLACEMENT_PASS
exit 0
