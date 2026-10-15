# Explicit display/physical sources, exact region IDs and atomic native migration.
set trace_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $trace_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
proc trace_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc trace_near {actual expected message} { if {abs($actual-$expected) > 2e-5} { error "$message: $actual != $expected" } }
proc trace_values {expected} {
    foreach value [dict get [scvcutview trace_model eval cut] section values] { trace_near [lindex $value end] $expected "Display trace source" }
}
proc trace_flux {expected} {
    trace_near [dict get [scvcutview trace_model heatflow cut pressure 0 0 0] heatFlow] $expected "Physical trace source"
}
proc trace_run {} {
    scvcutview trace_model read [file join $::trace_dir .. native fixtures recovery_hex.vtu]
    scvcutview trace_model add cut 1 0 0 -1
    scvcutview trace_model field cell pressure 0
    scvcutview trace_model visibility cut 2
    trace_values 7; trace_flux 7
    scvcutview trace_model trace cut display positive
    trace_values 9; trace_flux 7
    scvcutview trace_model trace cut physical positive
    trace_flux 9
    scvcutview trace_model trace cut display negative
    trace_values 7; trace_flux 9
    scvcutview trace_model trace cut physical region 9223372036854775807 {-9223372036854775808 9223372036854775807}
    set physical [scvcutview trace_model trace cut physical]
    trace_assert {[dict get $physical region] eq "9223372036854775807" && [lindex [dict get $physical regions] 0] eq "-9223372036854775808"} "Exact int64 binding lost"
    set stable [scvcutview trace_model snapshot]
    trace_assert {[string first {"version":7} $stable] >= 0} "Trace JSON version"
    foreach arguments {{physical region 1.5 {10 20}} {physical region 0 {10 20}} {physical region 10 {10 10}} {physical region 10 {10}} {physical region 9223372036854775808 {10 20}} {physical bad} {bad lowest}} {
        trace_assert {[catch {scvcutview trace_model trace cut {*}$arguments}]} "Invalid trace arguments accepted"
        trace_assert {[scvcutview trace_model snapshot] eq $stable} "Invalid trace edit changed definitions"
    }
    set invalid [string map [list {"region":9223372036854775807} {"region":0}] $stable]
    trace_assert {[catch {scvcutview trace_model restore $invalid}] && [scvcutview trace_model snapshot] eq $stable} "Unresolved JSON trace was not atomic"
    scvcutview trace_model trace cut physical lowest
    scvcutview trace_model restore $stable
    trace_flux 9
    scvcutview trace_model cellmode cut projected
    scvcutview trace_model recovery 100 0 none
    trace_flux 9
    scvcutview trace_model recovery 0 1 material
    trace_flux 9
    scvcutview trace_model cellmode cut constant
    scvcutview trace_model tensor mises xx-yy-zz-xy-xz-yz tensor
    scvcutview trace_model field cell stress 0
    set force [scvcutview trace_model freebody cut stress 0 1 2 3 4 5 0 0 0]
    trace_near [lindex [dict get $force force] 0] 9 "Original positive stress force"
    trace_near [lindex [dict get $force moment] 1] 4.5 "Original positive stress moment Y"
    trace_near [lindex [dict get $force moment] 2] -4.5 "Original positive stress moment Z"
    scvcutview trace_model tensor pressure xx-yy-zz-xy-xz-yz engineering
    set force [scvcutview trace_model freebody cut stress 0 1 2 3 4 5 0 0 0]
    trace_near [lindex [dict get $force force] 0] 9 "Tensor display changed physical stress"
    scvcutview trace_model field cell pressure 0
    scvcutview trace_model move cut -1 0 0 1
    trace_values 9
    trace_flux -9
    if {[info exists ::env(CUTVIEW_TRACE_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit Driver1/Viewer1/CutTrace -width 800 -height 600
        vsetbg -color WHITE
        scvcutview trace_model display
        scvcutview trace_model handle cut 0
        vaxo; vfit
        foreach policy {negative positive} {
            scvcutview trace_model trace cut display $policy
            set path [file join $::env(CUTVIEW_OUTPUT_DIR) ${policy}.png]
            vdump $path
            set pixels [scvcutimagestats $path]
            trace_assert {[dict get $pixels colored] > 100} "Blank trace cloud"
            set checksum($policy) [dict get $pixels checksum]
        }
        trace_assert {$checksum(negative) ne $checksum(positive)} "Trace source did not change cloud pixels"
    }
    scvcutview trace_model clear
    puts "CUT_VIEW_TRACE_PASS"
}
if {[catch {trace_run} message]} { puts stderr "CUT_VIEW_TRACE_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
