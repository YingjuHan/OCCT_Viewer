set scope_dir [file dirname [file normalize [info script]]]
source [file join $scope_dir viewdraw_runtime.tcl]
pload MODELING
source [file join $scope_dir cut_view_manager.tcl]
proc scope_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc scope_dump {name} { vrepaint; vdump [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png] }
proc scope_changed {a b} { dict get [scvcutimagecompare [file join $::env(CUTVIEW_OUTPUT_DIR) $a.png] [file join $::env(CUTVIEW_OUTPUT_DIR) $b.png]] changed }
if {[catch {
    file mkdir $::env(CUTVIEW_OUTPUT_DIR)
    vinit Driver1/Viewer1/ScopeA -width 800 -height 600
    vbackground -color white
    scvcutview scope_a read [file join $scope_dir .. native fixtures deformed_hex.vtu]
    scvcutview scope_a add p 1 0 0 -.5
    scvcutview scope_a add q 0 1 0 -.5
    scvcutview scope_a enable q 0
    scvcutview scope_a visibility p 3
    scvcutview scope_a field point temperature 0
    scvcutview scope_a display; vaxo; scvcutview scope_a fit
    scope_assert {[dict get [scvcutview scope_a renderinfo] backend] eq "gpu_plane"} "Needs resident GPU body"
    scvcutmanager scope_a
    set ::CutViewManager::selected q; ::CutViewManager::refresh 1
    set ::CutViewManager::sensitivity 100
    scope_dump a_before
    scvcutview scope_b share scope_a
    vinit Driver1/Viewer1/ScopeB -width 800 -height 600
    vbackground -color white
    scope_dump b_empty
    scope_assert {[dict get [scvcutimagestats [file join $::env(CUTVIEW_OUTPUT_DIR) b_empty.png]] visible]<10} "A objects leaked into new viewport B"
    scvcutview scope_b add b 0 0 1 -.25
    scvcutview scope_b field point temperature 0
    scvcutview scope_b display; vaxo; scvcutview scope_b fit
    scvcutmanager scope_b
    set ::CutViewManager::sensitivity 10
    scope_dump b_before
    scvcutview scope_a move p 1 0 0 -.7
    scope_dump b_after
    scope_assert {[scope_changed b_before b_after]==0} "A cut edits changed viewport B"
    scvcutmanager scope_a
    scope_assert {$::CutViewManager::selected eq "q" && $::CutViewManager::sensitivity==100} "Manager lost A selection/precision"
    scvcutmanager scope_b
    scope_assert {$::CutViewManager::selected eq "b" && $::CutViewManager::sensitivity==10} "Manager state crossed models"
    vactivate Driver1/Viewer1/ScopeA
    scope_dump a_changed
    vactivate Driver1/Viewer1/ScopeB
    set request {{"cut":"b","field":{"name":"displacement","nodal":true,"component":0,"step":-1},"components":[0,1,2,0,1,2,0,1,2]}}
    scvcutview scope_b physics viewcut/b $request
    scvcutview scope_b annotation b 1 0 1 note {B only annotation}
    box scope_cad 1 1 1
    bsave scope_cad [file join $::env(CUTVIEW_OUTPUT_DIR) scope.brep]
    scvcutview scope_b cadsection scope_cad b 0
    scvcutfreebody scope_b b
    .scvcutfreebody.actions.captureDefault invoke
    scope_assert {[scvcutview scope_b physicsdefault]} "Template capture UI failed: $::CutViewFreeBody::status"
    scvcutview scope_b add c 1 0 0 -.3
    scvcutview scope_b enable c 0
    scvcutfreebody scope_b c
    scope_assert {[dict get [scvcutview scope_b physicsstate c] configured]
        && ![dict get [scvcutview scope_b physicsstate c] enabled]} "New cut did not validate/apply common template"
    scvcutview scope_b sessionsave [file join $::env(CUTVIEW_OUTPUT_DIR) scope.json] [file join $::env(CUTVIEW_OUTPUT_DIR) scope.brep]
    scvcutview scope_b sessionload [file join $::env(CUTVIEW_OUTPUT_DIR) scope.json]
    scope_assert {[scvcutview scope_b physicsdefault] && ![scvcutview scope_a physicsdefault]} "Template session or viewport isolation failed"
    scope_dump b_loaded
    vactivate Driver1/Viewer1/ScopeA
    scope_dump a_after
    scope_assert {[scope_changed a_before a_after]>0} "A edit did not affect its own viewport"
    scope_assert {[scope_changed a_changed a_after]==0} "B session/overlays changed viewport A"
    ::CutViewManager::close; scvcutview scope_b clear; scvcutview scope_a clear
} message options]} { puts stderr "CUT_VIEW_SCOPE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_SCOPE_PASS
exit 0
