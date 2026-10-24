set free_dir [file dirname [file normalize [info script]]]
source [file join $free_dir viewdraw_runtime.tcl]
source [file join $free_dir cut_view_manager.tcl]
proc fb_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc bgerror {message} { puts stderr "CUT_VIEW_FREE_BODY_FAIL $message\n$::errorInfo"; exit 1 }
if {[catch {
    set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
    vinit FreeBody -width 950 -height 700
    scvcutview free read [file join $free_dir .. native fixtures stress_heat.vtu]
    scvcutview free add p 1 0 0 -.25
    scvcutview free add q 0 1 0 -.25
    scvcutview free enable q 0
    scvcutview free field point heat_flux 1 -1
    scvcutview free display
    scvcutmanager free
    set ::CutViewManager::selected p; ::CutViewManager::refresh 1
    .scvcutmanager.freebody invoke
    set ::CutViewFreeBody::field stress
    .scvcutfreebody.actions.apply invoke
    fb_assert {[dict get [scvcutview free physicsstate p] enabled]} "Free body not enabled: $::CutViewFreeBody::status"
    set before [scvcutview free physicsreport viewcut/p]
    fb_assert {[dict get $before overlays]<=2 && [dict get $before samples]>=1} "Invalid default resultant overlay count"
    fb_assert {abs([lindex [dict get $before sumPoint] 1]-.25)<1e-6} "Centroid sum point"
    set ::CutViewManager::selected q; ::CutViewManager::refresh 1
    set ::CutViewFreeBody::normalSign -1
    .scvcutfreebody.actions.apply invoke
    set after [scvcutview free physicsreport viewcut/p]
    fb_assert {abs([lindex [dict get $after force] 0]+[lindex [dict get $before force] 0])<1e-6} "Reverse physical normal"
    fb_assert {![dict get [scvcutview free physicsstate q] configured]} "Open options silently retargeted"
    set ::CutViewFreeBody::resolution 1; set ::CutViewFreeBody::tangent {1 0 0}
    .scvcutfreebody.actions.apply invoke
    fb_assert {[scvcutview free physicsreport viewcut/p] eq $after} "Invalid tangent published"
    set ::CutViewFreeBody::tangent {0 1 0}
    set ::CutViewFreeBody::units {0 1 1 1 1}; .scvcutfreebody.actions.apply invoke
    fb_assert {[scvcutview free physicsreport viewcut/p] eq $after} "Invalid units published"
    set ::CutViewFreeBody::units {1 1 1 1 1}; set ::CutViewFreeBody::resolution 0
    set ::CutViewFreeBody::enabled 0; .scvcutfreebody.actions.apply invoke
    fb_assert {![dict get [scvcutview free physicsstate p] enabled] && [catch {scvcutview free physicsreport viewcut/p}]} "Disable retained reports"
    scvcutview free physicsenabled p 1
    set before [scvcutview free snapshot]
    set ::CutViewFreeBody::output [file join $output report.csv]
    .scvcutfreebody.export.save invoke
    fb_assert {[file size $::CutViewFreeBody::output]>100} "CSV missing"
    scvcutview free sessionsave [file join $output free.json]
    scvcutview free physicsenabled p 0
    scvcutview free sessionload [file join $output free.json]
    fb_assert {[dict get [scvcutview free physicsstate p] enabled]} "Session lost free-body state"
    fb_assert {[scvcutview free snapshot] eq $before} "Physical options changed cut state"
    scvcutfreebody free p
    fb_assert {$::CutViewFreeBody::normalSign==-1 && $::CutViewFreeBody::sumPoint eq "centroid"} "Reopen lost options"
    set active 0
    foreach f [scvcutview free fields] { if {[dict get $f active] && [dict get $f name] eq "heat_flux"} { set active 1 } }
    fb_assert {$active} "Physical field changed contour binding"
    ::CutViewManager::close; scvcutview free clear
} message options]} { puts stderr "CUT_VIEW_FREE_BODY_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_FREE_BODY_PASS
exit 0
