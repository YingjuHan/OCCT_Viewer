# Real viewer-event acceptance, not programmatic SetPlane-only animation.
set interaction_dir [file dirname [file normalize [info script]]]
proc bgerror {message} {
    puts stderr "CUT_VIEW_INTERACTION_FAIL: Tk callback: $message\n$::errorInfo"
    exit 1
}
proc interaction_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc interaction_near {actual expected message} {
    if {abs($actual-$expected) > 1.0e-6} { error "$message: $actual != $expected" }
}
proc interaction_cut {id} {
    foreach cut [scvcutview cut_view_demo list] { if {[dict get $cut id] eq $id} { return $cut } }
    error "Missing cut $id"
}
proc interaction_check_section {} {
    set shown [scvcutview cut_view_demo presentation x]
    set plane [dict get [interaction_cut x] plane]
    set result [scvcutview cut_view_demo eval x]
    interaction_assert {[dict get $shown revision] == [dict get $result revision]} "Lazy evaluation hid a stale displayed section"
    set values [dict get $result section values]
    interaction_assert {[llength $values] > 0} "Empty dragged section"
    foreach value $values {
        lassign $value x y z temperature
        lassign $plane a b c d
        interaction_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 "Dragged node is not on the current plane"
        interaction_near $temperature [expr {$x+2*$y+3*$z}] "Dragged field is not interpolated"
    }
    return $result
}
proc interaction_image {filename} {
    global interaction_output
    set path [file join $interaction_output $filename]
    vdump $path
    set stats [scvcutimagestats $path]
    interaction_assert {[dict get $stats colored] > 500 && [dict get $stats colors] > 20} "Interactive contour is blank"
    return [dict get $stats checksum]
}
proc interaction_find_manipulator {mode} {
    set info [scvcutview cut_view_demo handleinfo]
    lassign [dict get $info origin] ox oy oz
    lassign [scvcutview cut_view_demo project $ox $oy $oz] cx cy
    foreach radius {20 30 40 50 60 70 80} {
        for {set angle 0} {$angle < 360} {incr angle 15} {
            set x [expr {round($cx+$radius*cos($angle*acos(-1)/180))}]
            set y [expr {round($cy+$radius*sin($angle*acos(-1)/180))}]
            scvcutview cut_view_demo mouse move $x $y
            set info [scvcutview cut_view_demo handleinfo]
            if {[dict get $info detected] eq "manipulator" && [dict get $info mode] == $mode} { return [list $x $y $cx $cy] }
        }
    }
    error "Manipulator mode $mode cannot be picked"
}
proc run_cut_view_interaction {} {
    global interaction_dir interaction_output
    set interaction_output [file normalize [file join $interaction_dir .. .. build cut-view-interaction]]
    if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set interaction_output $::env(CUTVIEW_OUTPUT_DIR) }
    file mkdir $interaction_output
    source [file join $interaction_dir cut_view_demo.tcl]
    update idletasks
    interaction_assert {[winfo exists .scvcutmanager]} "Manager did not open"
    interaction_assert {$::CutViewManager::status eq ""} "Manager initialization failed"
    interaction_assert {[llength [scvcutview cut_view_demo list]] == 3} "Default XYZ cuts are missing"
    interaction_image initial.png
    scvcutview cut_view_demo enable x 0
    interaction_image no_active_cuts.png
    scvcutview cut_view_demo enable x 1

    # Selecting an inactive row must not change either row's activation.
    set ::CutViewManager::selected y
    ::CutViewManager::refresh 1
    interaction_assert {![dict get [interaction_cut y] enabled] && [dict get [interaction_cut x] enabled]} "Selection changed activation"
    set ::CutViewManager::selected x
    ::CutViewManager::refresh 1
    .scvcutmanager.controls.translate set 0.4
    update idletasks
    interaction_near [lindex [dict get [interaction_cut x] plane] 3] -0.4 "Manager translation"
    interaction_check_section

    # Drag the OCCT rotation ring, then the normal-axis arrow, using real event routing.
    scvcutview cut_view_demo move x 1 0 0 -0.25
    ::CutViewManager::refresh 1
    lassign [interaction_find_manipulator 2] px py cx cy
    set saved [dict get [interaction_cut x] plane]
    scvcutview cut_view_demo mouse down $px $py
    set angle [expr {15*acos(-1)/180}]
    set dx [expr {$px-$cx}]; set dy [expr {$py-$cy}]
    set tx [expr {round($cx+$dx*cos($angle)-$dy*sin($angle))}]
    set ty [expr {round($cy+$dx*sin($angle)+$dy*cos($angle))}]
    scvcutview cut_view_demo mouse move $tx $ty
    interaction_assert {[dict get [interaction_cut x] plane] ne $saved} "Rotation ring did not recompute the cut"
    interaction_check_section
    scvcutview cut_view_demo mouse up $tx $ty
    interaction_check_section
    interaction_image mouse_rotation.png

    ::CutViewManager::refresh 1
    lassign [interaction_find_manipulator 2] px py cx cy
    set saved [dict get [interaction_cut x] plane]
    scvcutview cut_view_demo mouse down $px $py
    scvcutview cut_view_demo mouse move [expr {$px+10}] [expr {$py-10}]
    interaction_assert {[dict get [interaction_cut x] plane] ne $saved} "Manipulator abort test did not rotate"
    scvcutview cut_view_demo mouse abort $px $py
    foreach actual [dict get [interaction_cut x] plane] expected $saved { interaction_near $actual $expected "Manipulator abort rollback" }
    interaction_check_section

    scvcutview cut_view_demo move x 1 0 0 -0.25
    ::CutViewManager::refresh 1
    lassign [interaction_find_manipulator 1] px py cx cy
    set saved [dict get [interaction_cut x] plane]
    scvcutview cut_view_demo mouse down $px $py
    scvcutview cut_view_demo mouse move [expr {$px+15}] $py
    interaction_assert {[dict get [interaction_cut x] plane] ne $saved} "Translation arrow did not recompute the cut"
    interaction_check_section
    scvcutview cut_view_demo mouse up [expr {$px+15}] $py
    ::CutViewManager::refresh
    interaction_check_section
    interaction_image mouse_arrow.png
    set ::CutViewManager::axis Y; set ::CutViewManager::lastAngle 0
    ::CutViewManager::rotate 20
    set plane [dict get [interaction_cut x] plane]
    interaction_near [lindex $plane 0] [expr {cos(20*acos(-1)/180)}] "Manager rotation normal"
    interaction_check_section
    interaction_image rotated.png
    scvcutview cut_view_demo move x 1 0 0 -0.25
    ::CutViewManager::refresh 1

    # Press/hold/move/release travel through the same AIS_ViewController as real mouse input.
    lassign [scvcutview cut_view_demo project 0.25 0.2 0.2] startX startY
    scvcutview cut_view_demo mouse move $startX $startY
    interaction_assert {[dict get [scvcutview cut_view_demo handleinfo] detected] eq "plane"} "Plane cannot be picked"
    set before [dict get [scvcutview cut_view_demo handleinfo] updates]
    scvcutview cut_view_demo mouse down $startX $startY
    set previous -0.25
    set checksum [interaction_image mouse_before.png]
    for {set frame 1} {$frame <= 6} {incr frame} {
        set sync [dict get [scvcutview cut_view_demo presentation x] synchronizations]
        scvcutview cut_view_demo mouse move [expr {$startX-5*$frame}] $startY
        interaction_assert {[dict get [scvcutview cut_view_demo handleinfo] dragging]} "Drag ended before mouse release"
        interaction_assert {[dict get [scvcutview cut_view_demo presentation x] synchronizations] > $sync} "Display did not update during held drag"
        set d [lindex [dict get [interaction_cut x] plane] 3]
        interaction_assert {abs($d-$previous) > 1.0e-5} "Mouse movement did not update the plane"
        set previous $d
        interaction_check_section
        set next [interaction_image mouse_$frame.png]
        interaction_assert {$next ne $checksum} "Rendered cloud pixels did not change during held drag"
        set checksum $next
    }
    scvcutview cut_view_demo mouse up [expr {$startX-30}] $startY
    set info [scvcutview cut_view_demo handleinfo]
    interaction_assert {![dict get $info dragging] && [dict get $info updates] >= $before+6} "Mouse drag lifecycle failed"
    ::CutViewManager::refresh
    interaction_near $::CutViewManager::position [expr {-$previous}] "Manager did not follow mouse position"

    # Abort must restore both geometry and interpolated values, not only the positioning aid.
    set saved [dict get [interaction_cut x] plane]
    lassign [scvcutview cut_view_demo project [expr {-$previous}] 0.2 0.2] startX startY
    scvcutview cut_view_demo mouse move $startX $startY
    scvcutview cut_view_demo mouse down $startX $startY
    scvcutview cut_view_demo mouse move [expr {$startX+20}] $startY
    interaction_assert {[dict get [interaction_cut x] plane] ne $saved} "Abort test did not start a drag"
    scvcutview cut_view_demo mouse abort $startX $startY
    foreach actual [dict get [interaction_cut x] plane] expected $saved { interaction_near $actual $expected "Abort rollback" }
    interaction_check_section

    # Every portion combination must recompute while the mouse button is held.
    foreach mask {0 1 2 3 4 5 6 7} {
        scvcutview cut_view_demo move x 1 0 0 -0.25
        scvcutview cut_view_demo visibility x $mask
        scvcutview cut_view_demo handle x 1 0
        lassign [scvcutview cut_view_demo project 0.25 0.2 0.2] px py
        scvcutview cut_view_demo mouse move $px $py
        scvcutview cut_view_demo mouse down $px $py
        set sync [dict get [scvcutview cut_view_demo presentation x] synchronizations]
        scvcutview cut_view_demo mouse move [expr {$px-10}] $py
        set shown [scvcutview cut_view_demo presentation x]
        interaction_assert {[dict get [scvcutview cut_view_demo handleinfo] dragging] && [dict get $shown synchronizations] > $sync} "Portion mask $mask did not update during drag"
        set result [scvcutview cut_view_demo eval x]
        if {$mask != 0} { interaction_assert {[dict get $shown revision] == [dict get $result revision]} "Stale mask $mask presentation" }
        set index 0
        foreach key {negative section positive} {
            interaction_assert {([dict get $result $key cells] > 0) == (($mask & (1 << $index)) != 0)} "Dragged portion combination $mask/$key"
            incr index
        }
        scvcutview cut_view_demo mouse up [expr {$px-10}] $py
    }
    scvcutview cut_view_demo visibility x 2

    foreach mask {0 1 2 3 4 5 6 7} {
        set ::CutViewManager::below [expr {($mask & 1) != 0}]
        set ::CutViewManager::on [expr {($mask & 2) != 0}]
        set ::CutViewManager::above [expr {($mask & 4) != 0}]
        ::CutViewManager::visibility
        interaction_assert {[dict get [interaction_cut x] visibility] == $mask} "Manager portion combination"
    }
    scvcutview cut_view_demo visibility x 2
    set ::CutViewManager::name {Plane {A} with spaces}
    ::CutViewManager::apply
    interaction_assert {[dict get [interaction_cut x] name] eq {Plane {A} with spaces}} "Unsafe Tcl metadata quoting"
    .scvcutmanager.actions.copy invoke
    set copy $::CutViewManager::selected
    interaction_assert {$copy ne "x" && [llength [scvcutview cut_view_demo list]] == 4} "Manager copy"
    .scvcutmanager.actions.delete invoke
    interaction_assert {[llength [scvcutview cut_view_demo list]] == 3} "Manager delete"
    set ::CutViewManager::selected x
    ::CutViewManager::refresh 1
    set ::CutViewManager::multiple 0
    ::CutViewManager::setMultiple
    interaction_assert {[llength [lsearch -all -inline -not -exact [lmap cut [scvcutview cut_view_demo list] {dict get $cut enabled}] 0]] == 1} "Single-cut mode"
    set ::CutViewManager::multiple 1

    set ::CutViewManager::fieldLabel [lindex $::CutViewManager::fieldLabels 2]
    ::CutViewManager::field
    set ::CutViewManager::cellmode projected
    ::CutViewManager::cellmode
    interaction_assert {[dict get [interaction_cut x] cellmode] eq "projected"} "Manager CellData mode"
    set ::CutViewManager::fieldLabel [lindex $::CutViewManager::fieldLabels 1]
    ::CutViewManager::field
    interaction_check_section
    interaction_image manager_final.png
    rename tk_getSaveFile interaction_real_save_dialog
    proc tk_getSaveFile args { return [file join $::interaction_output definitions.json] }
    try { ::CutViewManager::save } finally { rename tk_getSaveFile {}; rename interaction_real_save_dialog tk_getSaveFile }
    scvcutview cut_view_demo remove y
    rename tk_getOpenFile interaction_real_open_dialog
    proc tk_getOpenFile args { return [file join $::interaction_output definitions.json] }
    try { ::CutViewManager::load } finally { rename tk_getOpenFile {}; rename interaction_real_open_dialog tk_getOpenFile }
    interaction_assert {[llength [scvcutview cut_view_demo list]] == 3} "Saved definitions did not restore"
    interaction_check_section
    update idletasks
    if {[auto_execok import] ne ""} {
        exec import -window [winfo id .scvcutmanager] [file join $interaction_output manager_panel.png]
    }
    ::CutViewManager::close
    source [file join $interaction_dir cut_view_demo.tcl]
    interaction_check_section
    ::CutViewManager::close
    scvcutview cut_view_demo clear

    foreach model {surface_drag cell_drag cell_constant_drag} file {surface.vtp two_tetra.vtu two_tetra.vtu} mapping {point cell cell} field {temperature pressure pressure} {
        scvcutview $model read [file normalize [file join $interaction_dir .. native fixtures $file]]
        scvcutview $model add x 1 0 0 -0.25
        scvcutview $model field $mapping $field 0
        if {$model eq "cell_drag"} { scvcutview $model cellmode x projected }
        scvcutview $model display
        scvcutview $model handle x 1
        scvcutview $model fit
        set z [expr {$mapping eq "point" ? 0 : 0.6}]
        set y [expr {$mapping eq "point" ? 0.2 : 0.05}]
        lassign [scvcutview $model project 0.25 $y $z] px py
        scvcutview $model mouse move $px $py
        interaction_assert {[dict get [scvcutview $model handleinfo] detected] eq "plane"} "Cannot pick $model positioning aid"
        scvcutview $model mouse down $px $py
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview $model presentation x] synchronizations]
            scvcutview $model mouse move [expr {$px-5*$frame}] $py
            set shown [scvcutview $model presentation x]
            interaction_assert {[dict get [scvcutview $model handleinfo] dragging] && [dict get $shown synchronizations] > $sync} "No live display update for $model"
            set plane [dict get [lindex [scvcutview $model list] 0] plane]
            set offset [expr {-[lindex $plane 3]}]
            interaction_assert {abs($offset-0.25) > 1.0e-5} "Mouse did not cut $model"
            set result [scvcutview $model eval x]
            interaction_assert {[dict get $shown revision] == [dict get $result revision]} "Lazy evaluation hid stale $model presentation"
            if {$mapping eq "point"} {
                interaction_assert {[dict get $result lines] == 1} "Missing surface intersection line"
                set values [lsort -real [dict get $result lineValues]]
                interaction_near [lindex $values 0] $offset "Dragged surface interpolation start"
                interaction_near [lindex $values 1] [expr {2-$offset}] "Dragged surface interpolation end"
            } elseif {$model eq "cell_constant_drag"} {
                set values [lsort -unique [dict get $result section values]]
                interaction_assert {$values eq {2 6}} "Dragged constant CellData lost element discontinuity"
            } else {
                foreach value [dict get $result section values] {
                    lassign $value x y z pressure
                    interaction_near $x $offset "Dragged elemental section geometry"
                    interaction_near $pressure [expr {4-2*$z}] "Dragged projected CellData interpolation"
                }
            }
        }
        scvcutview $model mouse up [expr {$px-15}] $py
        set image [file join $interaction_output ${model}.png]
        vdump $image
        set stats [scvcutimagestats $image]
        interaction_assert {[dict get $stats colored] > 100 && [dict get $stats colors] > 0} "Missing $model contour"
        scvcutview $model clear
    }
    # Large displacement must also move the guide, ranges and actual held-drag intersections.
    scvcutview deformed_drag read [file normalize [file join $interaction_dir .. native fixtures deformed_tetra.vtu]]
    scvcutview deformed_drag add x 1 0 0 -0.25
    scvcutview deformed_drag field point temperature 0
    scvcutview deformed_drag display
    scvcutview deformed_drag handle x 1
    set initialShift [dict get [scvcutview deformed_drag deform displacement 1000] effectiveScale]
    set shift [dict get [scvcutview deformed_drag deform displacement [expr {2000.0/$initialShift}]] effectiveScale]
    interaction_assert {$shift > 1} "Deformed drag fixture does not exercise out-of-original-bounds motion"
    lassign [scvcutview deformed_drag bounds] xmin ymin zmin xmax ymax zmax
    interaction_near $xmin $shift "Guide bounds did not follow deformation"
    interaction_near $xmax [expr {$shift+1}] "Deformed upper guide bound"
    lassign [scvcutview deformed_drag range x] lower upper
    interaction_near $lower $shift "Deformed slider range lower"
    scvcutview deformed_drag move x 1 0 0 [expr {-$shift-0.25}]
    scvcutview deformed_drag fit
    lassign [scvcutview deformed_drag project [expr {$shift+0.25}] 0.2 0.2] px py
    scvcutview deformed_drag mouse move $px $py
    interaction_assert {[dict get [scvcutview deformed_drag handleinfo] detected] eq "plane"} "Deformed plane guide cannot be picked"
    scvcutview deformed_drag mouse down $px $py
    for {set frame 1} {$frame <= 3} {incr frame} {
        scvcutview deformed_drag mouse move [expr {$px-5*$frame}] $py
        set shown [scvcutview deformed_drag presentation x]
        set result [scvcutview deformed_drag eval x]
        interaction_assert {[dict get [scvcutview deformed_drag handleinfo] dragging]
            && [dict get $shown revision] == [dict get $result revision]} "Deformed section did not update before release"
        set offset [expr {-[lindex [dict get [lindex [scvcutview deformed_drag list] 0] plane] 3]}]
        interaction_assert {abs($offset-$shift-0.25) > 1.0e-5} "Deformed plane was clamped to original model bounds"
        foreach value [dict get $result section values] {
            lassign $value x y z temperature
            interaction_near $x $offset "Deformed dragged section position"
            interaction_near $temperature [expr {$x-$shift+2*$y+3*$z}] "Deformed dragged cloud interpolation"
        }
    }
    scvcutview deformed_drag mouse up [expr {$px-15}] $py
    scvcutview deformed_drag deform none 0
    interaction_near [lindex [scvcutview deformed_drag bounds] 0] 0 "Reset leaves stale deformed bounds"
    scvcutview deformed_drag clear
    puts "CUT_VIEW_INTERACTION_PASS: $interaction_output"
}
if {[catch {run_cut_view_interaction} message options]} {
    puts stderr "CUT_VIEW_INTERACTION_FAIL: $message"
    puts stderr [dict get $options -errorinfo]
    exit 1
}
exit 0
