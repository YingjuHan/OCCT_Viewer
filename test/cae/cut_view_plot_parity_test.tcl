set pp_dir [file dirname [file normalize [info script]]]
source [file join $pp_dir viewdraw_runtime.tcl]
source [file join $pp_dir cut_view_manager.tcl]
proc pp_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc pp_dump {name} { vrepaint; vdump [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png] }
proc pp_changed {a b} { dict get [scvcutimagecompare [file join $::env(CUTVIEW_OUTPUT_DIR) $a.png] [file join $::env(CUTVIEW_OUTPUT_DIR) $b.png]] changed }
if {[catch {
    file mkdir $::env(CUTVIEW_OUTPUT_DIR)
    vinit PlotParity -width 950 -height 700
    scvcutview parity read [file join $pp_dir .. native fixtures deformed_hex.vtu]
    scvcutview parity add p 1 0 0 -.5
    scvcutview parity visibility p 3
    scvcutview parity field point temperature 0
    scvcutview parity display; vaxo; scvcutview parity fit
    pp_dump cloud
    scvcutmanager parity; scvcutoptions parity
    .scvcutoptions.tabs select 1
    set ::CutViewOptions::values(1,fieldColor) 0
    foreach key {r g b er eg eb} value {1 0 0 0 0 1} { set ::CutViewOptions::values(1,$key) $value }
    .scvcutoptions.actions.apply invoke
    pp_assert {$::CutViewOptions::status eq ""} "Solid face UI: $::CutViewOptions::status"
    pp_dump solid
    pp_assert {[pp_changed cloud solid]>100} "GPU face color source was ignored"
    pp_assert {[dict get [scvcutview parity renderinfo] backend] eq "gpu_plane"} "Solid color disabled GPU"
    set ::CutViewOptions::values(1,mode) cloud_edges
    .scvcutoptions.actions.apply invoke
    scvcutview parity displaymode cloud_edges
    pp_dump solid_edges
    set ::CutViewOptions::values(1,edgeType) 1
    .scvcutoptions.actions.apply invoke
    pp_assert {$::CutViewOptions::status eq ""} "Pattern UI: $::CutViewOptions::status"
    pp_dump dashed
    pp_assert {[dict get [scvcutview parity renderinfo] fallback] eq "patterned_edges" && [pp_changed solid_edges dashed]>5} "Patterned CPU edges were ignored"
    scvcutview parity cutstyle plot 0 0 2 2 0 1 0 0 0 1 0 0 0
    foreach p {0 1 2} { scvcutview parity cutstyle defaults $p 0 2 2 1 0 0 0 0 0 0 1 1 }
    set inherited [scvcutview parity cutstyle p 1]
    pp_assert {[dict get $inherited inheritPlot] && ![dict get $inherited fieldColor] && [lindex [dict get $inherited color] 1]==1} "Plot inheritance did not resolve"
    scvcutview parity cutstyle plot 0 0 2 2 0 0 1 1 0 0 0 0 0
    pp_assert {[lindex [dict get [scvcutview parity cutstyle p 1] color] 2]==1} "Plot edit did not propagate"
    set style [scvcutview parity cutstyle p 1]
    scvcutview parity sessionsave [file join $::env(CUTVIEW_OUTPUT_DIR) parity.json]
    scvcutview parity cutstyle plot 0 0 1 1 1 1 1
    scvcutview parity sessionload [file join $::env(CUTVIEW_OUTPUT_DIR) parity.json]
    pp_assert {[scvcutview parity cutstyle p 1] eq $style} "Session lost plot style/source"
    scvcutmanager parity
    .scvcutmanager.header.behavior set abaqus
    event generate .scvcutmanager.header.behavior <<ComboboxSelected>>
    pp_assert {$::CutViewManager::status eq "" && [scvcutview parity multimode] eq "abaqus"} "Compatibility UI failed"
    pp_assert {[.scvcutmanager.controls.below cget -state] eq "disabled"} "Abaqus portion controls remained editable"
    scvcutview parity add q 0 1 0 -.5
    scvcutview parity visibility q 6
    pp_assert {[dict get [lindex [scvcutview parity list] 0] visibility]==3} "Compatibility mode rewrote saved portions"
    set request {{"cut":"p","field":{"name":"displacement","nodal":true,"component":0,"step":-1},"components":[0,1,2,0,1,2,0,1,2]}}
    scvcutview parity physics result $request
    pp_assert {[dict get [scvcutview parity physicsreport result] overlays]==0} "Compatibility mode displayed free body arrows"
    scvcutview parity multimode project
    scvcutview parity physicsenabled p 1
    pp_assert {[dict get [scvcutview parity physicsreport result] overlays]>0} "Project extension did not restore free body arrows"
    pp_assert {[catch {scvcutview parity multimode single}]} "Single mode accepted two active cuts"
    ::CutViewManager::close; scvcutview parity clear
} message options]} { puts stderr "CUT_VIEW_PLOT_PARITY_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_PLOT_PARITY_PASS
exit 0
