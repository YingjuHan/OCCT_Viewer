# Original FE double crossings and real held-threshold display synchronization.
set iso_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $iso_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
proc iso_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc iso_near {a b tolerance message} { if {abs($a-$b) > $tolerance} { error "$message: $a != $b" } }
proc bgerror {message} { puts stderr "CUT_VIEW_HIGH_ORDER_ISO_FAIL: $message\n$::errorInfo"; exit 1 }
proc iso_pump {} { set ::iso_tick 0; after 25 {set ::iso_tick 1}; vwait ::iso_tick; update idletasks }
proc iso_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]; vdump $path
    iso_assert {[dict get [scvcutimagestats $path] colored] > 200} "High-order iso cloud blank"
    set f [open $path rb]; set bytes [read $f]; close $f; return $bytes
}
proc iso_check {value} {
    set result [scvcutview high_iso eval iso]
    iso_assert {[dict get $result sourceGeometry] eq "numerical"} "Iso approximation not reported"
    iso_assert {[dict get $result isoFieldResidual] <= [dict get $result isoFieldTolerance]} "Field residual limit"
    set roots {}
    foreach sample [dict get $result section values] {
        lassign $sample x y z scalar
        iso_near [expr {$x*(1-$x)}] $value 0.000002 "Original nonmonotone FE root"
        iso_near $scalar [expr {$x+2*$y}] 0.000002 "Independent displayed FE value"
        lappend roots $x
    }
    iso_assert {[llength $roots] > 2 && [lindex [lsort -real $roots] 0] < 0.5 && [lindex [lsort -real $roots] end] > 0.5} "Double crossing missing"
    set mask [dict get $result visible]
    if {$mask & 1} { iso_near [dict get $result negative area] [expr {0.5*(1-sqrt(1-4*$value))}] 0.000002 "Lower area reference" }
    if {$mask & 4} { iso_near [dict get $result positive area] [expr {0.5*sqrt(1-4*$value)}] 0.000002 "Upper area reference" }
    return $result
}
proc run_high_order_iso {} {
    scvcutview high_iso read [file join $::env(CUTVIEW_ISO_FIXTURES) nonmonotone.vtu]
    scvcutview high_iso isooptions 0.004 0 2 6 2000000
    scvcutview high_iso isoadd iso point cut 0 0.16
    scvcutview high_iso field point temperature 0
    for {set mask 0} {$mask < 8} {incr mask} {
        scvcutview high_iso visibility iso $mask
        set result [scvcutview high_iso eval iso]
        set bit 1
        foreach part {negative section positive} {
            iso_assert {([dict get $result $part nodes] > 0) == (($mask & $bit) != 0)} "High-order iso masks"
            set bit [expr {$bit << 1}]
        }
    }
    scvcutview high_iso visibility iso 7; iso_check 0.16
    scvcutview high_iso field cell pressure 0
    foreach value [dict get [scvcutview high_iso eval iso] lineValues] { iso_near $value 7 0.000001 "Iso line CellData" }
    scvcutview high_iso cellmode iso projected
    foreach value [dict get [scvcutview high_iso eval iso] lineValues] { iso_near $value 7 0.000001 "Iso projected CellData" }
    scvcutview high_iso field point temperature 0
    set snapshot [scvcutview high_iso snapshot]
    iso_assert {[catch {scvcutview high_iso isooptions 0.004 0 0 6 2000000}]} "Invalid refinement accepted"
    iso_assert {[scvcutview high_iso snapshot] eq $snapshot} "Invalid options changed state"
    scvcutview high_iso restore $snapshot
    if {[info exists ::env(CUTVIEW_HIGH_ISO_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit HighOrderIso -width 800 -height 600; vsetbg -color WHITE; vtop
        scvcutview high_iso visibility iso 3; scvcutview high_iso display; scvcutview high_iso fit
        source [file join $::iso_dir cut_view_manager.tcl]; scvcutmanager high_iso; iso_pump
        set w .scvcutmanager.controls.translate
        set snapshot [scvcutview high_iso snapshot]
        set previous [iso_image before]
        lassign [$w coords [$w get]] px py
        event generate $w <ButtonPress-1> -x $px -y $py
        foreach target {0.12 0.10 0.08} {
            set old [scvcutview high_iso presentation iso]
            lassign [$w coords $target] x y
            event generate $w <B1-Motion> -x $x -y $y -state 256; iso_pump
            set value [$w get]; iso_check $value
            set result [scvcutview high_iso eval iso]; set presentation [scvcutview high_iso presentation iso]
            iso_assert {[dict get $presentation revision] == [dict get $result revision] && [dict get $presentation synchronizations] > [dict get $old synchronizations]} "Held threshold revision stale"
            set next [iso_image held_$target]; iso_assert {$next ne $previous} "Held threshold pixels unchanged"; set previous $next
        }
        event generate $w <Escape>; iso_pump
        iso_assert {[scvcutview high_iso snapshot] eq $snapshot} "Threshold abort changed definitions"
        iso_check 0.16
        event generate $w <ButtonRelease-1> -x $x -y $y; iso_pump
        ::CutViewManager::close
    }
    scvcutview high_iso clear; puts "CUT_VIEW_HIGH_ORDER_ISO_PASS"
}
if {[catch {run_high_order_iso} message]} { puts stderr "CUT_VIEW_HIGH_ORDER_ISO_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
