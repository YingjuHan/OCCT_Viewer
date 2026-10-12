set tensor_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $tensor_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
proc tensor_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc tensor_near {actual expected message} { tensor_assert {abs($actual-$expected) < 2e-5} "$message: $actual != $expected" }
proc tensor_check {expected} {
    set section [dict get [scvcutview tensor_model eval cut] section]
    set mapping [dict get $section valueMapping]
    tensor_assert {[llength [dict get $section values]] > 0} "Tensor section values absent"
    foreach sample [dict get $section values] {
        set actual [expr {$mapping eq "element" ? $sample : [lindex $sample end]}]
        tensor_near $actual $expected "Tensor independent display reference"
    }
    set force [scvcutview tensor_model freebody cut stress 0 4 8 1 2 5 0 0 0]
    tensor_near [lindex [dict get $force force] 0] 7 "Tensor measure changed physical normal stress"
    tensor_near [lindex [dict get $force force] 1] 2 "Tensor measure changed physical shear"
}
proc tensor_run {} {
    global tensor_dir
    scvcutview tensor_model read [file join $tensor_dir .. native fixtures tensor_hex.vtu]
    scvcutview tensor_model add cut 1 0 0 -0.25
    scvcutview tensor_model visibility cut 2
    # Select a nonzero raw index to catch accidental use of it on scalar measures.
    scvcutview tensor_model field cell stress 5
    scvcutview tensor_model cellmode cut projected
    set left [expr {sqrt(61.0)}]; set right [expr {sqrt(93.0)}]
    scvcutview tensor_model tensor mises row-major tensor
    tensor_check [expr {0.75*$left+0.25*8}]
    scvcutview tensor_model recovery 100 1 none
    tensor_check [expr {0.75*$left+0.25*($left+$right)/2}]
    scvcutview tensor_model recovery 100 0 material
    tensor_check $left
    scvcutview tensor_model tensor mises row-major engineering
    tensor_check [expr {sqrt(52.0)}]
    scvcutview tensor_model tensor pressure row-major tensor
    tensor_check [expr {-7.0/3}]
    scvcutview tensor_model tensor maxprincipal row-major tensor
    tensor_check [expr {(7+sqrt(65.0))/2}]
    scvcutview tensor_model tensor midprincipal row-major tensor
    tensor_check 0
    scvcutview tensor_model tensor minprincipal row-major tensor
    tensor_check [expr {(7-sqrt(65.0))/2}]
    scvcutview tensor_model tensor tresca column-major tensor
    tensor_check [expr {sqrt(65.0)}]
    set snapshot [scvcutview tensor_model snapshot]
    scvcutview tensor_model tensor components row-major engineering
    scvcutview tensor_model restore $snapshot
    tensor_assert {[scvcutview tensor_model snapshot] eq $snapshot} "Tensor snapshot round trip changed settings"
    foreach options {{mises xx-yy-zz-xy-xz-yz tensor} {mises row-major invalid} {invalid row-major tensor}} {
        tensor_assert {[catch {scvcutview tensor_model tensor {*}$options}]} "Invalid tensor conventions accepted"
        tensor_assert {[scvcutview tensor_model snapshot] eq $snapshot} "Invalid conventions changed live settings"
    }
    foreach mask {0 1 2 3 4 5 6 7} { scvcutview tensor_model visibility cut $mask; scvcutview tensor_model eval cut }
    scvcutview tensor_model visibility cut 2
    if {[info exists ::env(CUTVIEW_TENSOR_VISUAL)]} {
        set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
        scvcutview tensor_model move cut 0 1 0 -0.5
        scvcutview tensor_model recovery 100 0 none
        vinit Tensor -width 800 -height 600
        vsetbg -color WHITE
        scvcutview tensor_model display
        vaxo
        scvcutview tensor_model fit
        source [file join $tensor_dir cut_view_manager.tcl]
        scvcutmanager tensor_model
        set ::CutViewManager::gizmo 0
        scvcutview tensor_model handle cut 0
        update
        vdump [file join $output tensor_cloud.png]
        set pixels [scvcutimagestats [file join $output tensor_cloud.png]]
        tensor_assert {[dict get $pixels colored] > 300 && [dict get $pixels colors] > 20} "Derived tensor cloud has no field colors"
        # Recompute while changing the invariant and ordering, then retain a
        # second cloud so the visual audit checks both bindings.
        scvcutview tensor_model recovery 100 1 none
        scvcutview tensor_model tensor mises row-major tensor
        vdump [file join $output tensor_before.png]
        set changed [scvcutimagestats [file join $output tensor_before.png]]
        tensor_assert {[dict get $changed checksum] ne [dict get $pixels checksum]} "Tensor measure/order did not update cloud pixels"
        ::CutViewManager::refresh
        update
        ::CutViewManager::close
    }
    scvcutview tensor_model clear
    puts "CUT_VIEW_TENSOR_PASS"
}
if {[catch {tensor_run} message options]} {
    puts stderr "CUT_VIEW_TENSOR_FAIL: $message\n[dict get $options -errorinfo]"
    exit 1
}
exit 0
