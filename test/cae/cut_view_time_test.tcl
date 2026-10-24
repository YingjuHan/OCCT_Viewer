set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload MODELING VISUALIZATION
pload -VIEWDRAW
set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
set fixture $::env(VTK_SERIES_FIXTURES)
set manifest [file join $fixture native-moving.vtm.series]
set file [open $manifest w]
puts $file {{"files":[{"name":"first.vtm","time":0},{"name":"moved.vtm","time":1}]}}
close $file
set covered {}
proc time_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc time_pump {} { set ::time_tick 0; after 25 {set ::time_tick 1}; vwait ::time_tick; update idletasks }
proc time_mark {widget} { if {$widget ni $::covered} { lappend ::covered $widget } }
proc time_click {widget} { time_mark $widget; time_assert {[winfo ismapped $widget]} "Unmapped time widget $widget"; event generate $widget <Enter>; event generate $widget <ButtonPress-1> -x 5 -y 5; event generate $widget <ButtonRelease-1> -x 5 -y 5; time_pump }
proc time_entry {widget value} { time_mark $widget; $widget delete 0 end; $widget insert 0 $value; time_pump }
proc time_combo {widget value} { time_mark $widget; $widget set $value; event generate $widget <<ComboboxSelected>>; time_pump }
proc time_inventory {parent} {
    set result {}
    foreach child [winfo children $parent] { if {[winfo class $child] in {TButton TCheckbutton TCombobox TSpinbox TEntry}} { lappend result $child }; lappend result {*}[time_inventory $child] }
    return $result
}
proc bgerror {message} { puts stderr "CUT_VIEW_TIME_FAIL $message\n$::errorInfo"; exit 1 }
proc time_run {} {
    global script_dir output manifest covered
    vinit NativeTime -width 800 -height 600; vsetbg -color WHITE
    scvcutview time read $manifest
    scvcutview time add p 1 0 0 -0.25
    scvcutview time field cell pressure 0 0
    scvcutview time contour 0 0 20 1 10
    scvcutview time display; vaxo; scvcutview time fit
    source [file join $script_dir cut_view_time.tcl]; scvcuttime time; time_pump
    set expected [time_inventory .scvcuttime]
    time_entry .scvcuttime.material.datum moving
    time_combo .scvcuttime.material.configuration updated
    time_entry .scvcuttime.material.anchors {0 0 1 2 1 0 1 2}
    time_combo .scvcuttime.material.cut p
    time_click .scvcuttime.material.apply
    time_assert {$::CutViewTime::status eq ""} "Material UI: $::CutViewTime::status"
    vrepaint; vdump [file join $output step0.png]
    time_entry .scvcuttime.step.value 1; time_click .scvcuttime.step.apply
    time_assert {[dict get [scvcutview time timeinfo] step] == 1} "Step control failed"
    set plane [dict get [lindex [scvcutview time list] 0] plane]
    time_assert {abs([lindex $plane 3]+10.25) < 1e-6} "Material cut did not follow geometry"
    set probe [scvcutview time probe p 1 0 1]
    time_assert {[lindex [dict get $probe values] 0] == 17 && [dict get $probe element] == 0} "Step contour/provenance mismatch"
    scvcutview time fit; vrepaint; vdump [file join $output step1.png]
    time_assert {[dict get [scvcutimagecompare [file join $output step0.png] [file join $output step1.png]] changed] > 100} "Step redraw unchanged"
    time_entry .scvcuttime.step.value -1; time_click .scvcuttime.step.apply
    time_assert {$::CutViewTime::status ne "" && [dict get [scvcutview time timeinfo] step] == 1} "Invalid step changed live state"
    time_entry .scvcuttime.step.value 0; time_click .scvcuttime.step.apply
    time_combo .scvcuttime.deform.field displacement
    time_entry .scvcuttime.deform.scale 0.1; time_click .scvcuttime.deform.apply
    time_assert {$::CutViewTime::status eq ""} "Coordinated deformation control failed"
    time_combo .scvcuttime.deform.field none; time_click .scvcuttime.deform.apply
    time_entry .scvcuttime.animation.period 0.05
    time_click .scvcuttime.animation.timestamps
    time_click .scvcuttime.animation.loop
    time_click .scvcuttime.animation.restart
    for {set i 0} {$i < 4} {incr i} { time_pump }
    time_assert {[dict get [scvcutview time timeinfo] step] == 1 && ![dict get [scvcutview time timeinfo] playing]} "Animation restart/end failed"
    time_click .scvcuttime.animation.loop; time_click .scvcuttime.animation.play
    time_click .scvcuttime.animation.pause
    set paused [dict get [scvcutview time timeinfo] step]; time_pump
    time_assert {![dict get [scvcutview time timeinfo] playing] && [dict get [scvcutview time timeinfo] step] == $paused} "Pause advanced frame"
    time_entry .scvcuttime.step.value 0; time_click .scvcuttime.step.apply
    time_combo .scvcuttime.material.configuration reference
    time_entry .scvcuttime.material.datum fixed
    time_entry .scvcuttime.material.anchors {-1 0 1 2}; time_click .scvcuttime.material.apply
    foreach {mode parameters} {
        regular {-0.1 0.1 3}
        range {-0.1 0.1 3}
        rotational {0 0 1 0 0 0 0 0.2 3}
        path {0 0 1 0.1 0 0 0.2 0 0 0.3 0 0}
    } {
        time_combo .scvcuttime.slicing.mode $mode
        time_entry .scvcuttime.slicing.parameters $parameters; time_click .scvcuttime.slicing.apply
        time_assert {$::CutViewTime::status eq ""} "$mode slicing failed: $::CutViewTime::status"
    }
    time_entry .scvcuttime.collection.id p_path
    time_click .scvcuttime.collection.show
    time_assert {$::CutViewTime::status eq ""} "Show collection failed: $::CutViewTime::status"
    time_entry .scvcuttime.scan.request missing_request
    time_click .scvcuttime.scan.path
    time_click .scvcuttime.scan.export
    time_assert {$::CutViewTime::status ne ""} "Missing scan mapping was accepted"
    time_click .scvcuttime.collection.remove
    time_assert {$::CutViewTime::status eq ""} "Remove collection failed"
    time_click .scvcuttime.close
    foreach widget $expected { time_assert {$widget in $covered} "Untested time widget $widget" }
    puts "CUT_VIEW_TIME_COVERAGE [llength $expected]/[llength $expected]"
    scvcutview time clear
    puts "CUT_VIEW_TIME_PASS $output"
}
if {[catch {time_run} message options]} { puts stderr "CUT_VIEW_TIME_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
exit 0
