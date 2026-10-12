set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload MODELING VISUALIZATION
pload -VIEWDRAW
set output [file normalize [file join $script_dir .. .. build cut-view-tools]]
if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set output $::env(CUTVIEW_OUTPUT_DIR) }
file mkdir $output
set covered {}
proc tool_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc tool_pump {} { set ::tool_tick 0; after 25 {set ::tool_tick 1}; vwait ::tool_tick; update idletasks }
proc tool_mark {widget} { if {$widget ni $::covered} { lappend ::covered $widget } }
proc tool_click {widget} {
    tool_mark $widget; tool_assert {[winfo ismapped $widget]} "Unmapped tool $widget"
    event generate $widget <Enter>
    event generate $widget <ButtonPress-1> -x 5 -y 5
    event generate $widget <ButtonRelease-1> -x 5 -y 5
    tool_pump
}
proc tool_entry {widget value} { tool_mark $widget; $widget delete 0 end; $widget insert 0 $value; tool_pump }
proc tool_combo {widget value} { tool_mark $widget; $widget set $value; event generate $widget <<ComboboxSelected>>; tool_pump }
proc tool_inventory {parent} {
    set controls {}
    foreach child [winfo children $parent] {
        if {[winfo class $child] in {TButton TCheckbutton TCombobox TSpinbox TEntry}} { lappend controls $child }
        lappend controls {*}[tool_inventory $child]
    }
    return $controls
}
proc bgerror {message} { puts stderr "CUT_VIEW_TOOLS_FAIL $message\n$::errorInfo"; exit 1 }
proc tools_run {} {
    global script_dir output covered
    vinit NativeTools -width 800 -height 600
    vsetbg -color WHITE
    scvcutview tools read [file join $script_dir .. native fixtures stress_heat.vtu]
    scvcutview tools add p 1 0 0 -0.25
    scvcutview tools field point heat_flux 0
    scvcutview tools display
    vaxo; scvcutview tools fit
    source [file join $script_dir cut_view_manager.tcl]
    scvcutmanager tools
    scvcutview tools field point heat_flux 0
    tool_pump
    tool_click .scvcutmanager.tools
    set expected [tool_inventory .scvcuttools]
    tool_entry .scvcuttools.datum.values {0.25 0 0 0 1 0 -1 0 0 0 0 1}
    tool_click .scvcuttools.datum.apply
    tool_assert {$::CutViewTools::status eq ""} "Valid datum rejected"
    tool_entry .scvcuttools.local.values {0 -1 0 0}
    tool_click .scvcuttools.local.apply
    set plane [dict get [lindex [scvcutview tools list] 0] plane]
    tool_assert {abs([lindex $plane 3]+0.25) < 1e-8 && [lindex $plane 0] == 1} "Local/global edit failed"
    tool_entry .scvcuttools.datum.values {0.25 0 0 0 1 0 -1 0 0 0 0 -1}
    tool_click .scvcuttools.datum.apply
    tool_assert {$::CutViewTools::status ne ""} "Left-handed datum accepted"
    tool_entry .scvcuttools.datum.values {0.25 0 0 0 1 0 -1 0 0 0 0 1}
    tool_click .scvcuttools.datum.apply
    tool_click .scvcuttools.datum.axes
    tool_combo .scvcuttools.style.portion 1
    tool_combo .scvcuttools.style.owner "Common defaults"
    tool_entry .scvcuttools.style.opacity 0.1
    tool_click .scvcuttools.style.apply
    tool_combo .scvcuttools.style.owner "Selected cut"
    tool_entry .scvcuttools.style.opacity 0.3
    tool_entry .scvcuttools.style.width 4
    tool_click .scvcuttools.style.edges
    tool_click .scvcuttools.style.apply
    set style [scvcutview tools cutstyle p 1]
    tool_assert {abs([dict get $style transparency]-0.3) < 1e-6 && [dict get $style width] == 4 && [dict get $style mode] == 2} "Cut style controls"
    tool_click .scvcuttools.style.reset
    tool_assert {abs([dict get [scvcutview tools cutstyle p 1] transparency]-0.1) < 1e-6} "Override reset did not restore common style"
    tool_click .scvcuttools.style.apply
    tool_click .scvcuttools.contour.auto
    tool_entry .scvcuttools.contour.minimum 0
    tool_entry .scvcuttools.contour.maximum 6
    tool_entry .scvcuttools.contour.intervals 8
    tool_click .scvcuttools.contour.legend
    tool_click .scvcuttools.contour.apply
    set contour [scvcutview tools contour]
    tool_assert {![dict get $contour automatic] && [dict get $contour maximum] == 6 && [dict get $contour legend] && [dict get $contour intervals] == 8} "Contour controls"
    vrepaint; vdump [file join $output contour.png]
    tool_click .scvcuttools.contour.legend
    vrepaint; vdump [file join $output no_legend.png]
    tool_assert {[dict get [scvcutimagecompare [file join $output contour.png] [file join $output no_legend.png]] changed] > 300} "Legend did not affect redraw"
    tool_click .scvcuttools.contour.legend
    tool_entry .scvcuttools.vector.scale 0.15
    tool_entry .scvcuttools.vector.sampling 1
    tool_combo .scvcuttools.vector.field "heat_flux (point) step -1"
    tool_click .scvcuttools.vector.orientation
    tool_click .scvcuttools.vector.apply
    tool_assert {$::CutViewTools::status eq ""} "Vector controls: $::CutViewTools::status"
    vrepaint; vdump [file join $output orientation.png]
    tool_assert {[dict get [scvcutimagecompare [file join $output contour.png] [file join $output orientation.png]] changed] > 20} "Vectors did not affect redraw"
    tool_entry .scvcuttools.probe.index 0
    tool_combo .scvcuttools.probe.pick 1
    tool_click .scvcuttools.probe.apply
    set probe $::CutViewTools::status
    lassign [dict get $probe position] x y z
    tool_assert {abs([lindex [dict get $probe values] 1]-(1+4*$y)) < 1e-6 && [dict get $probe element] == 0} "Probe FE/element reference"
    set sum 0; foreach weight [dict get $probe nodes] { set sum [expr {$sum+[lindex $weight 1]}] }
    tool_assert {abs($sum-1) < 1e-8} "Probe provenance weights"
    scvcutview tools handle p 0
    lassign [scvcutview tools project $x $y $z] px py
    scvcutview tools mouse move $px $py
    set picked [scvcutview tools picked]
    lassign [dict get $picked position] a b c
    tool_assert {abs([lindex [dict get $picked values] 1]-(1+4*$b)) < 1e-6 && [dict get $picked element] == 0} "Real node picking/probe"
    tool_click .scvcuttools.probe.node
    tool_click .scvcuttools.probe.apply
    tool_assert {[dict get $::CutViewTools::status element] == 0} "Element probe"
    set elementProbe $::CutViewTools::status
    tool_combo .scvcuttools.probe.pick 2
    lassign [dict get $elementProbe position] x y z
    lassign [scvcutview tools project $x $y $z] px py
    scvcutview tools mouse move $px $py
    set picked [scvcutview tools picked]
    tool_assert {[dict get $picked element] == 0} "Real element picking/probe"
    tool_click .scvcuttools.probe.node
    tool_entry .scvcuttools.annotation.text "Original heat flux"
    tool_click .scvcuttools.annotation.apply
    tool_assert {$::CutViewTools::status eq ""} "Annotation control"
    uplevel #0 {box cad 1 1 1}
    tool_entry .scvcuttools.cad.shape cad
    tool_click .scvcuttools.cad.sync
    tool_click .scvcuttools.cad.apply
    tool_assert {$::CutViewTools::status eq ""} "CAD section control: $::CutViewTools::status"
    tool_click .scvcuttools.cad.clip
    tool_combo .scvcuttools.probe.pick 0
    vrepaint; vdump [file join $output tools.png]
    tool_assert {[dict get [scvcutimagestats [file join $output tools.png]] colored] > 100} "Tools screenshot lacks field colors"
    tool_click .scvcuttools.cad.clear
    tool_click .scvcuttools.annotation.clear
    tool_click .scvcuttools.time
    CutViewTime::close
    tool_click .scvcuttools.physics
    destroy .scvcutphysics
    tool_click .scvcuttools.cache
    set cacheExpected [tool_inventory .scvcutcache]
    tool_assert {[llength $cacheExpected]==7} "Cache inventory"
    tool_entry .scvcutcache.source 1
    tool_entry .scvcutcache.result 1
    tool_click .scvcutcache.apply
    tool_assert {[dict get $::CutViewCache::status sourceLimit]==1048576} "Cache policy controls"
    tool_click .scvcutcache.info
    set request [dict get $::CutViewCache::status request]
    tool_click .scvcutcache.request
    tool_assert {[dict get $::CutViewCache::status request]>$request} "Request generation control"
    tool_click .scvcutcache.cancel
    tool_entry .scvcutcache.source 0; tool_click .scvcutcache.apply
    tool_assert {$::CutViewCache::status ne "" && [dict get [scvcutview tools cacheinfo] sourceLimit]==1048576} "Invalid cache limit changed live policy"
    tool_click .scvcutcache.close
    foreach widget $cacheExpected { tool_assert {$widget in $covered} "Untested cache control $widget" }
    puts "CUT_VIEW_CACHE_COVERAGE 7/7"
    tool_click .scvcuttools.close
    foreach widget $expected { tool_assert {$widget in $covered} "Untested tool $widget" }
    puts "CUT_VIEW_TOOLS_COVERAGE [llength $expected]/[llength $expected]"
    scvcutview tools clear
    for {set i 0} {$i < 8} {incr i} { tool_pump }
    tool_assert {![winfo exists .scvcutmanager] && ![winfo exists .scvcuttools]} "Model close left stale tools"
    puts "CUT_VIEW_TOOLS_PASS $output"
}
if {[catch {tools_run} message options]} { puts stderr "CUT_VIEW_TOOLS_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
exit 0
