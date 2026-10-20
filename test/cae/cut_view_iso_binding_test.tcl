set ib_dir [file dirname [file normalize [info script]]]
source [file join $ib_dir viewdraw_runtime.tcl]
source [file join $ib_dir cut_view_manager.tcl]
proc ib_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc bgerror {message} { puts stderr "CUT_VIEW_ISO_BINDING_FAIL $message\n$::errorInfo"; exit 1 }
if {[catch {
    set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
    vinit IsoBinding -width 950 -height 700
    scvcutview iso read [file join $ib_dir .. native fixtures stress_heat.vtu]
    scvcutview iso isoadd s point stress 0 3
    scvcutview iso field point heat_flux 1
    scvcutview iso display
    scvcutmanager iso
    scvcutisobinding iso s
    set ::CutViewIsoBinding::measure mises
    .scvcutisobinding.actions.apply invoke
    set binding [scvcutview iso isobinding s]
    ib_assert {[dict get $binding measure]==1} "Derived binding failed: $::CutViewIsoBinding::status"
    scvcutview iso tensor pressure xx-yy-zz-xy-xz-yz tensor
    scvcutview iso recovery 12 1 none
    scvcutview iso field point heat_flux 0
    ib_assert {[scvcutview iso isobinding s] eq $binding} "Contour settings changed iso snapshot"
    set ::CutViewIsoBinding::threshold -1
    .scvcutisobinding.actions.apply invoke
    ib_assert {[scvcutview iso isobinding s] eq $binding} "Invalid averaging published"
    scvcutview iso sessionsave [file join $output binding.json]
    scvcutview iso isobinding s 0 0 0 1 100 0
    scvcutview iso sessionload [file join $output binding.json]
    ib_assert {[scvcutview iso isobinding s] eq $binding} "Session lost iso binding"
    ::CutViewManager::close; scvcutview iso clear
} message options]} { puts stderr "CUT_VIEW_ISO_BINDING_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_ISO_BINDING_PASS
exit 0
