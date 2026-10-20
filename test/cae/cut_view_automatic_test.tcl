set auto_dir [file dirname [file normalize [info script]]]
source [file join $auto_dir viewdraw_runtime.tcl]
source [file join $auto_dir cut_view_manager.tcl]
proc auto_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    file mkdir $::env(CUTVIEW_OUTPUT_DIR)
    vinit AutomaticCuts -width 900 -height 650
    scvcutview automatic read [file join $auto_dir .. native fixtures deformed_hex.vtu]
    set snapshot {{"version":1,"cuts":[{"id":"crack","kind":"crack","field":{"name":"temperature","nodal":true,"component":0,"step":-1},"threshold":0,"min":0,"max":3,"sign":1,"visibility":3,"enabled":true}]}}
    scvcutview automatic automaticsync solver $snapshot
    scvcutview automatic display; vaxo; scvcutview automatic fit
    scvcutmanager automatic
    auto_assert {[dict get [lindex [scvcutview automatic list] 0] sourceOrigin] eq "xfem"} "Missing XFEM origin"
    foreach widget {.scvcutmanager.actions.delete .scvcutmanager.actions.copy .scvcutmanager.controls.above .scvcutmanager.controls.translate} {
        auto_assert {[$widget cget -state] eq "disabled"} "Protected widget $widget was enabled"
    }
    foreach command {{remove crack} {rename crack other} {copy crack other} {isovalue crack .2} {visibility crack 4}} {
        auto_assert {[catch {scvcutview automatic {*}$command}]} "Protected operation accepted: $command"
    }
    set state [scvcutview automatic list]
    auto_assert {[catch {scvcutview automatic automaticsync solver [string map {temperature missing} $snapshot]}]
        && [scvcutview automatic list] eq $state} "Bad metadata changed scene"
    scvcutview automatic sessionsave [file join $::env(CUTVIEW_OUTPUT_DIR) automatic.json]
    scvcutview automatic sessionload [file join $::env(CUTVIEW_OUTPUT_DIR) automatic.json]
    auto_assert {[dict get [lindex [scvcutview automatic list] 0] provider] eq "solver"} "Session lost provider"
    scvcutview automatic automaticsync solver {{"version":1,"cuts":[]}}
    auto_assert {[llength [scvcutview automatic list]]==0} "Missing sources were retained"
    set density [string map {crack density {"threshold":0} {"threshold":0.5} {"max":3} {"max":6}} $snapshot]
    scvcutview automatic automaticsync optimization $density
    scvcutmanager automatic
    auto_assert {[.scvcutmanager.controls.translate cget -state] eq "normal"} "Optimization slider was disabled"
    scvcutview automatic isovalue density .75
    scvcutview automatic sessionsave [file join $::env(CUTVIEW_OUTPUT_DIR) density.json]
    scvcutview automatic sessionload [file join $::env(CUTVIEW_OUTPUT_DIR) density.json]
    auto_assert {[dict get [lindex [scvcutview automatic list] 0] value]==.75} "Session lost optimization slider"
    ::CutViewManager::close; scvcutview automatic clear
} message options]} { puts stderr "CUT_VIEW_AUTOMATIC_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_AUTOMATIC_PASS
exit 0
