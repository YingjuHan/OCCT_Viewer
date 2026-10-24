set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload MODELING VISUALIZATION
pload -VIEWDRAW
set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
set covered {}
proc phys_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc phys_pump {} { set ::phys_tick 0; after 25 {set ::phys_tick 1}; vwait ::phys_tick; update idletasks }
proc phys_mark {w} { if {$w ni $::covered} { lappend ::covered $w } }
proc phys_click {w} { phys_mark $w; phys_assert {[winfo ismapped $w]} "Unmapped $w"; event generate $w <Enter>; event generate $w <ButtonPress-1> -x 5 -y 5; event generate $w <ButtonRelease-1> -x 5 -y 5; phys_pump }
proc phys_entry {w value} { phys_mark $w; $w delete 0 end; $w insert 0 $value; phys_pump }
proc phys_combo {w value} { phys_mark $w; $w set $value; event generate $w <<ComboboxSelected>>; phys_pump }
proc phys_inventory {w} { set result {}; foreach c [winfo children $w] { if {[winfo class $c] in {TButton TCheckbutton TCombobox TEntry}} { lappend result $c }; lappend result {*}[phys_inventory $c] }; return $result }
proc bgerror {message} { puts stderr "CUT_VIEW_PHYSICS_FAIL $message\n$::errorInfo"; exit 1 }
proc phys_run {} {
    global script_dir output covered
    vinit NativePhysics -width 900 -height 650; vsetbg -color WHITE
    scvcutview physics read [file join $script_dir .. native fixtures stress_heat.vtu]
    scvcutview physics add p 1 0 0 -0.25
    scvcutview physics field point heat_flux 1 -1
    scvcutview physics display; vaxo; scvcutview physics fit
    vrepaint; vdump [file join $output before.png]
    source [file join $script_dir cut_view_physics.tcl]; scvcutphysics physics; phys_pump
    set expected [phys_inventory .scvcutphysics]
    phys_entry .scvcutphysics.binding.id r
    phys_combo .scvcutphysics.binding.cut p
    phys_combo .scvcutphysics.binding.field stress
    phys_combo .scvcutphysics.binding.mapping point
    phys_entry .scvcutphysics.binding.components {0 3 4 3 1 5 4 5 2}
    phys_entry .scvcutphysics.binding.scale 0.15
    phys_entry .scvcutphysics.options.group part
    phys_entry .scvcutphysics.options.elements 0
    phys_entry .scvcutphysics.units.factors {0.001 1000000 1000000 1 1}
    phys_entry .scvcutphysics.frame.origin {0 0 0}
    phys_entry .scvcutphysics.frame.axes {0 0 0 1 0 0 0 1 0 0 0 1}
    phys_entry .scvcutphysics.section.mappings {[]}
    phys_click .scvcutphysics.binding.apply
    phys_assert {[dict get $::CutViewPhysics::status overlays]>0} "Missing actual physical overlays"
    set reference [scvcutview physics physicsreport r]
    phys_click .scvcutphysics.options.report
    phys_assert {$::CutViewPhysics::status eq $reference} "Report control"
    vrepaint; vdump [file join $output arrows.png]
    phys_assert {[dict get [scvcutimagecompare [file join $output before.png] [file join $output arrows.png]] changed]>30} "Resultant arrows did not draw"
    phys_click .scvcutphysics.binding.beam; phys_click .scvcutphysics.binding.apply
    phys_assert {$::CutViewPhysics::status ne "" && [scvcutview physics physicsreport r] eq $reference} "Invalid beam changed live report"
    phys_click .scvcutphysics.binding.beam
    phys_click .scvcutphysics.options.absolute; phys_click .scvcutphysics.binding.apply
    phys_assert {[lindex [dict get $::CutViewPhysics::status moment] 2]>=0} "Absolute summation"
    phys_click .scvcutphysics.options.absolute
    phys_click .scvcutphysics.binding.overlay; phys_click .scvcutphysics.binding.apply
    phys_assert {[dict get $::CutViewPhysics::status overlays]==0} "Arrow toggle"
    phys_click .scvcutphysics.binding.overlay
    phys_click .scvcutphysics.binding.heat; phys_combo .scvcutphysics.binding.field heat_flux
    phys_entry .scvcutphysics.binding.components {0 1 2 0 0 0 0 0 0}; phys_click .scvcutphysics.binding.apply
    phys_assert {abs([dict get $::CutViewPhysics::status heatFlow]-.5625)<1e-6} "Heat SI reference"
    phys_entry .scvcutphysics.export.path [file join $output report.csv]; phys_click .scvcutphysics.export.save
    set f [open [file join $output report.csv]]; set csv [read $f]; close $f
    phys_assert {[string first "heat_W" $csv]>=0 && [string first "total" $csv]>=0} "CSV export"
    phys_entry .scvcutphysics.automatic.id distance
    phys_combo .scvcutphysics.automatic.kind crack
    phys_combo .scvcutphysics.automatic.field heat_flux
    phys_entry .scvcutphysics.automatic.component 1
    phys_entry .scvcutphysics.automatic.threshold 2
    phys_entry .scvcutphysics.automatic.sign -1
    phys_entry .scvcutphysics.automatic.range {0 5}
    phys_entry .scvcutphysics.automatic.mask 1
    phys_click .scvcutphysics.automatic.apply
    phys_assert {$::CutViewPhysics::status ne ""} "Nonzero crack level was accepted"
    phys_entry .scvcutphysics.automatic.threshold 0
    phys_click .scvcutphysics.automatic.apply
    phys_assert {$::CutViewPhysics::status eq ""} "Explicit crack mapping"
    set distance {}; foreach d [scvcutview physics list] { if {[dict get $d id] eq "distance"} { set distance $d } }
    phys_assert {[dict get $distance visibility]==1 && [dict get $distance sourceOrigin] eq "xfem"} "XFEM Below must retain the complete body for either sign"
    phys_entry .scvcutphysics.automatic.id density
    phys_combo .scvcutphysics.automatic.kind density; phys_entry .scvcutphysics.automatic.sign 1
    phys_entry .scvcutphysics.automatic.threshold 2
    phys_entry .scvcutphysics.automatic.mask 2; phys_click .scvcutphysics.automatic.apply
    phys_assert {$::CutViewPhysics::status eq ""} "Explicit density mapping"
    set definitions [scvcutview physics list]
    phys_entry .scvcutphysics.automatic.id bad; phys_entry .scvcutphysics.automatic.range {0 1}
    phys_click .scvcutphysics.automatic.apply
    phys_assert {$::CutViewPhysics::status ne "" && [scvcutview physics list] eq $definitions} "Invalid density publication"
    phys_click .scvcutphysics.options.remove
    phys_assert {[catch {scvcutview physics physicsreport r}]} "Remove retained report"
    phys_click .scvcutphysics.close
    foreach w $expected { phys_assert {$w in $covered} "Untested physical widget $w" }
    puts "CUT_VIEW_PHYSICS_COVERAGE [llength $expected]/[llength $expected]"
    scvcutview physics clear; puts "CUT_VIEW_PHYSICS_PASS $output"
}
if {[catch {phys_run} message options]} { puts stderr "CUT_VIEW_PHYSICS_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
exit 0
