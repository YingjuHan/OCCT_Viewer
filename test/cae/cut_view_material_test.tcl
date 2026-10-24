# Material plane acceptance: original FE coordinates/fields, modes, masks and session.
set material_dir [file dirname [file normalize [info script]]]
source [file join $material_dir viewdraw_runtime.tcl]
source [file join $material_dir cut_view_manager.tcl]
proc material_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc bgerror {message} { puts stderr "CUT_VIEW_MATERIAL_FAIL $message\n$::errorInfo"; exit 1 }
if {[catch {
    set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
    vinit CutMaterial -width 1000 -height 760; vsetbg -color WHITE
    scvcutview material read [file join $material_dir .. native fixtures deformed_hex.vtu]
    scvcutview material add cut 1 0 0 -.57
    scvcutview material field point temperature 0
    scvcutview material visibility cut 2
    scvcutview material display
    scvcutmanager material
    .scvcutmanager.controls.definition invoke
    set ::CutViewPlacement::follow 1; set ::CutViewPlacement::reference Current
    .scvcutplacement.body.followApply invoke
    material_assert {$::CutViewPlacement::status eq ""} "Manager material enable failed: $::CutViewPlacement::status"
    .scvcutplacement.body.close invoke
    set first [scvcutview material materialstats cut]
    foreach mode {cloud wireframe cloud_edges} {
        scvcutview material displaymode $mode
        set first [scvcutview material materialstats cut]
        foreach scale {.3 -.2 0 .5} {
            scvcutview material deform displacement $scale
            set probe [scvcutview material probe cut 1 0 1]
            lassign [dict get $probe position] x y z
            material_assert {abs($x-$scale*$z-.57)<2e-6} "Material section did not follow shear: $probe"
            material_assert {abs([lindex [dict get $probe values] 0]-(.57+2*$y+3*$z))<2e-6} "Original field was reinterpreted spatially"
            material_assert {[dict get [scvcutview material renderinfo] fallback] eq "material_reference_cut"} "Material cut used world-plane clipping"
        }
        material_assert {[dict get [scvcutview material materialstats cut] referenceBuilds]==[dict get $first referenceBuilds]} "Scale/mode edit reintersected reference"
        foreach mask {0 1 2 3 4 5 6 7} { scvcutview material visibility cut $mask }
        scvcutview material fit; vaxo; scvcutview material handle cut 0
        vdump [file join $output ${mode}.png]
        material_assert {[dict get [scvcutimagestats [file join $output ${mode}.png]] visible]>500} "Blank material rendering"
    }
    scvcutview material visibility cut 2
    set snapshot [scvcutview material snapshot]
    scvcutview material sessionsave [file join $output material.json]
    scvcutview material material cut 0 Current
    scvcutview material sessionload [file join $output material.json]
    material_assert {[scvcutview material snapshot] eq $snapshot} "Material session did not restore captured reference"
    scvcutview material material cut 0 Current
    material_assert {[dict get [scvcutview material renderinfo] backend] eq "gpu_plane"} "Spatial GPU path was not restored"
    ::CutViewManager::close; scvcutview material clear
} message options]} {
    puts stderr "CUT_VIEW_MATERIAL_FAIL $message\n[dict get $options -errorinfo]"; exit 1
}
puts CUT_VIEW_MATERIAL_PASS
exit 0
