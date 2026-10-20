# Interactive (-i) DRAW acceptance: real Tk menus/dialogs and every manager control.
set controls_dir [file dirname [file normalize [info script]]]
set controls_output [file normalize [file join $controls_dir .. .. build cut-view-controls]]
if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set controls_output $::env(CUTVIEW_OUTPUT_DIR) }
file mkdir $controls_output
set ui_model cut_view_demo
set ui_covered {}
proc bgerror {message} {
    puts stderr "CUT_VIEW_CONTROLS_FAIL: Tk callback: $message\n$::errorInfo"
    exit 1
}
proc ui_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc ui_near {actual expected message} {
    if {![string is double -strict $actual] || abs($actual-$expected) > 1.0e-6} { error "$message: $actual != $expected" }
}
proc ui_pump {{ms 25}} {
    set ::ui_tick 0
    after $ms {set ::ui_tick 1}
    vwait ::ui_tick
    update idletasks
}
proc ui_capture {path} {
    global controls_dir
    update idletasks
    if {$::tcl_platform(platform) eq "windows"} {
        # Keep Tk responsive while PrintWindow captures the manager root wrapper.
        set capturePath $path
        file delete $capturePath
        set capture [open [list | powershell.exe -NoProfile -NonInteractive -WindowStyle Hidden -File \
            [file join $controls_dir cut_view_capture.ps1] -WindowHandle [winfo id .scvcutmanager] \
            -OutputPath $path 2>@1] r]
        fconfigure $capture -blocking 0
        set captureLog ""
        set captureDeadline [expr {[clock milliseconds]+30000}]
        while {![eof $capture] && [clock milliseconds] < $captureDeadline} {
            append captureLog [read $capture]
            ui_pump 25
        }
        set captured [eof $capture]
        close $capture
        ui_assert {$captured && [string first CUT_VIEW_CAPTURE_PASS $captureLog] >= 0 && [file exists $capturePath]} "Manager capture failed: $captureLog"
    } elseif {[auto_execok import] ne ""} { exec import -window [winfo id .scvcutmanager] $path }
}
proc ui_mark {widget} {
    if {$widget ni $::ui_covered} { lappend ::ui_covered $widget }
}
proc ui_ok {} { ui_assert {$::CutViewManager::status eq ""} "Manager error: $::CutViewManager::status" }
proc ui_cut {id} {
    foreach cut [scvcutview $::ui_model list] { if {[dict get $cut id] eq $id} { return $cut } }
    error "Missing cut $id"
}
proc ui_click {widget} {
    puts "CONTROL_CLICK $widget"; flush stdout
    ui_mark $widget
    ui_assert {[winfo ismapped $widget]} "Unmapped control: $widget"
    set x [expr {[winfo width $widget]/2}]; set y [expr {[winfo height $widget]/2}]
    event generate $widget <Enter> -x $x -y $y
    event generate $widget <ButtonPress-1> -x $x -y $y
    ui_pump 10
    event generate $widget <ButtonRelease-1> -x $x -y $y
    ui_pump
}
proc ui_entry {widget value {enter 0}} {
    puts "CONTROL_ENTRY $widget $value enter=$enter"; flush stdout
    ui_mark $widget
    focus -force $widget
    $widget delete 0 end
    $widget insert 0 $value
    if {$enter} { event generate $widget <Return>; ui_pump }
}
proc ui_combo {widget value} {
    puts "CONTROL_MENU $widget $value"; flush stdout
    ui_mark $widget
    set index [lsearch -exact [$widget cget -values] $value]
    ui_assert {$index >= 0} "Unavailable menu option: $widget/$value"
    event generate $widget <ButtonPress-1> -x [expr {[winfo width $widget]-8}] -y [expr {[winfo height $widget]/2}]
    ui_pump
    set listbox $widget.popdown.f.l
    ui_assert {[winfo exists $listbox] && [winfo ismapped $listbox]} "Menu did not open: $widget"
    $listbox see $index
    update idletasks
    lassign [$listbox bbox $index] x y width height
    event generate $listbox <ButtonPress-1> -x [expr {$x+5}] -y [expr {$y+$height/2}]
    event generate $listbox <ButtonRelease-1> -x [expr {$x+5}] -y [expr {$y+$height/2}]
    ui_pump
    ui_assert {[$widget get] eq $value} "Menu selection failed: $widget"
    ui_ok
}
proc ui_tree {id {column #0}} {
    puts "CONTROL_TREE $id $column"; flush stdout
    set w .scvcutmanager.cuts
    ui_mark $w
    $w see $id
    update idletasks
    lassign [$w bbox $id $column] x y width height
    event generate $w <ButtonPress-1> -x [expr {$x+$width/2}] -y [expr {$y+$height/2}]
    event generate $w <ButtonRelease-1> -x [expr {$x+$width/2}] -y [expr {$y+$height/2}]
    ui_pump
    ui_ok
}
proc ui_dialog_complete {path cancel} {
    set dialog .scvcutmanager.__tk_filedialog
    if {![winfo exists $dialog] || ![winfo ismapped $dialog]} {
        after 25 [list ui_dialog_complete $path $cancel]
        return
    }
    set ::ui_dialog_seen 1
    puts "CONTROL_DIALOG_READY $path cancel=$cancel"; flush stdout
    if {$cancel} { $dialog.contents.f2.cancel invoke } else {
        set entry $dialog.contents.f2.ent
        $entry delete 0 end; $entry insert 0 $path
        focus -force $entry
        if {[file exists $path] && $::ui_dialog_save} { after 25 [list ui_confirm_overwrite $dialog] }
        $dialog.contents.f2.ok invoke
    }
}
proc ui_confirm_overwrite {parent} {
    foreach dialog [list $parent.__tk__messagebox .scvcutmanager.__tk__messagebox .__tk__messagebox] {
        if {[winfo exists $dialog.yes] && [winfo ismapped $dialog]} {
            set ::ui_overwrite_seen 1
            $dialog.yes invoke
            return
        }
    }
    if {[winfo exists $parent] && [winfo ismapped $parent]} { after 25 [list ui_confirm_overwrite $parent] }
}
proc ui_dialog {button path {cancel 0}} {
    set ::ui_dialog_seen 0
    set ::ui_dialog_save [expr {[string match *.save $button]}]
    after 100 [list ui_dialog_complete $path $cancel]
    ui_click $button
    ui_assert {$::ui_dialog_seen} "Real file dialog was not tested: $button"
}
proc ui_controls {parent} {
    set result {}
    foreach child [winfo children $parent] {
        if {[winfo class $child] in {TButton TCheckbutton TCombobox TSpinbox TEntry Scale Treeview}} { lappend result $child }
        lappend result {*}[ui_controls $child]
    }
    return $result
}
proc ui_check_section {id} {
    set presentation [scvcutview $::ui_model presentation $id]
    set result [scvcutview $::ui_model eval $id]
    ui_assert {[dict get $presentation revision] == [dict get $result revision]} "Geometry evaluated but display was not synchronized: $presentation / [dict get $result revision], status=$::CutViewManager::status, cut=[ui_cut $id]"
    foreach value [dict get $result section values] {
        lassign $value x y z scalar
        lassign [dict get [ui_cut $id] plane] a b c d
        ui_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 "Section node off current plane"
        ui_near $scalar [expr {$x+2*$y+3*$z}] "Non-node result interpolation"
    }
}
proc ui_run {} {
    global controls_dir controls_output ui_model
    source [file join $controls_dir cut_view_demo.tcl]
    set beforeOpen [dict get [scvcutview $ui_model presentation x] revision]
    ::CutViewManager::close
    scvcutmanager $ui_model
    ui_assert {[dict get [scvcutview $ui_model presentation x] revision] == $beforeOpen} "Reopening manager invalidated the active field"
    if {$::tcl_platform(platform) eq "windows"} {
        # Exercise real Tk dialogs with the same options and widget assertions.
        # Windows shell dialogs do not expose Tk widgets to event generate.
        source [file join $::tk_library tkfbox.tcl]
        source [file join $::tk_library msgbox.tcl]
        rename ::tk_getOpenFile ::ui_native_getOpenFile
        rename ::tk_getSaveFile ::ui_native_getSaveFile
        rename ::tk_messageBox ::ui_native_messageBox
        proc ::tk_getOpenFile {args} { return [::tk::dialog::file:: open {*}$args] }
        proc ::tk_getSaveFile {args} { return [::tk::dialog::file:: save {*}$args] }
        proc ::tk_messageBox {args} { return [::tk::MessageBox {*}$args] }
        puts "CONTROL_DIALOG_BACKEND Tk (Windows shell dialogs excluded)"
    }
    ui_pump
    set expected [ui_controls .scvcutmanager]
    ui_ok
    ui_click .scvcutmanager.options
    ui_assert {[winfo exists .scvcutoptions] && $::CutViewOptions::status eq ""} "View Cut Options did not open"
    destroy .scvcutoptions
    ui_click .scvcutmanager.actions.rename
    ui_assert {[winfo exists .scvcutrename]} "Rename did not open"
    .scvcutrename.cancel invoke
    ui_click .scvcutmanager.tools
    ui_assert {[winfo exists .scvcuttools]} "Viewer tools did not open"
    destroy .scvcuttools
    # Exercise DRAW and Tk grid in the same interactive interpreter, without replacing either.
    ui_assert {[llength [info commands ::tk::grid]]} "OCCT SDK lacks the Tk grid collision fix"
    ttk::frame .grid_probe; ttk::label .grid_probe.label -text Probe
    grid .grid_probe.label
    ui_assert {[dict get [grid info .grid_probe.label] -in] eq ".grid_probe"} "Tk grid dispatch failed"
    grid remove .grid_probe.label; grid .grid_probe.label; grid forget .grid_probe.label
    destroy .grid_probe
    if {[catch {grid 0} message]} { ui_assert {$message eq "DRAW grid object is unavailable"} "Numeric DRAW grid dispatch changed: $message" }

    ui_tree x
    foreach role {display physical} {
        foreach policy {negative positive lowest} {
            ui_combo .scvcutmanager.traces.${role}Policy $policy
            ui_click .scvcutmanager.traces.${role}Apply
            ui_ok
            ui_assert {[dict get [scvcutview $ui_model trace x $role] policy] eq $policy} "Trace policy control failed"
        }
        ui_combo .scvcutmanager.traces.${role}Policy region
        ui_entry .scvcutmanager.traces.${role}Region -9223372036854775808
        ui_entry .scvcutmanager.traces.${role}Regions {-9223372036854775808}
        ui_click .scvcutmanager.traces.${role}Apply
        ui_ok
        set exact [scvcutview $ui_model trace x $role]
        ui_assert {[dict get $exact region] eq "-9223372036854775808" && [lindex [dict get $exact regions] 0] eq "-9223372036854775808"} "Trace control rounded int64 IDs"
        set snapshot [scvcutview $ui_model snapshot]
        ui_entry .scvcutmanager.traces.${role}Region 1.5 1
        ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $snapshot} "Invalid trace edit changed definitions"
        ui_entry .scvcutmanager.traces.${role}Region -9223372036854775808 1
        ui_ok
        ui_combo .scvcutmanager.traces.${role}Policy lowest
        ui_click .scvcutmanager.traces.${role}Apply
        ui_ok
    }
    ui_capture [file join $controls_output trace_manager.png]

    ui_tree y
    ui_assert {$::CutViewManager::selected eq "y" && ![dict get [ui_cut y] enabled]} "Selecting enabled an inactive cut"
    ui_tree y show
    ui_assert {[dict get [ui_cut y] enabled]} "Tree Show toggle failed"
    ui_tree y show
    foreach column {below on above} bit {1 2 4} {
        set before [dict get [ui_cut y] visibility]
        ui_tree y $column
        ui_assert {[dict get [ui_cut y] visibility] == ($before ^ $bit)} "Tree portion toggle failed"
    }
    ui_tree x
    ui_click .scvcutmanager.controls.show
    ui_assert {![dict get [ui_cut x] enabled]} "Show button did not disable"
    ui_click .scvcutmanager.controls.show
    foreach widget {below on above} bit {1 2 4} {
        set before [dict get [ui_cut x] visibility]
        ui_click .scvcutmanager.controls.$widget
        ui_assert {[dict get [ui_cut x] visibility] == ($before ^ $bit)} "Portion checkbutton failed"
    }
    scvcutview $ui_model visibility x 2
    ::CutViewManager::refresh 1
    foreach widget {name nx ny nz position} value {{Plane {A} with spaces} 2 0 0 0.3} {
        ui_entry .scvcutmanager.controls.$widget $value
    }
    ui_click .scvcutmanager.controls.apply
    ui_ok
    ui_near [dict get [ui_cut x] offset] 0.3 "Numeric plane Apply"
    ui_assert {[dict get [ui_cut x] name] eq {Plane {A} with spaces}} "Name quoting failed"
    ui_entry .scvcutmanager.controls.position 0.35 1
    ui_near [dict get [ui_cut x] offset] 0.35 "Position Return binding"
    set saved [scvcutview $ui_model snapshot]
    foreach value {{} nonsense NaN Inf -Inf} {
        ui_entry .scvcutmanager.controls.position $value 1
        ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $saved} "Invalid position changed the cut: $value"
    }
    ui_entry .scvcutmanager.controls.position -0.1 1
    ui_near [dict get [ui_cut x] offset] -0.1 "Negative position input"
    ui_entry .scvcutmanager.controls.position 0.35 1
    set saved [scvcutview $ui_model snapshot]
    ui_entry .scvcutmanager.controls.nx 0
    ui_click .scvcutmanager.controls.apply
    ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $saved} "Invalid normal was not rejected atomically"
    set error $::CutViewManager::status
    ui_pump 200
    ui_assert {$::CutViewManager::status eq $error} "Polling erased the input error"
    ui_entry .scvcutmanager.controls.nx 1
    ui_click .scvcutmanager.controls.apply
    ui_ok
    # Selection must refresh entries even if a numeric edit retains keyboard focus.
    focus -force .scvcutmanager.controls.position
    ui_tree y
    ui_assert {$::CutViewManager::nx == 0 && $::CutViewManager::ny == 1} "Selected plane kept stale focused entries"
    ui_tree x

    ui_click .scvcutmanager.controls.definition
    ui_assert {[winfo exists .scvcutplacement]} "Missing plane definition editor"
    ui_click .scvcutplacement.body.close
    set saved [scvcutview $ui_model snapshot]
    ui_combo .scvcutmanager.controls.sensitivity 100
    ui_assert {[scvcutview $ui_model snapshot] eq $saved} "Sensitivity moved the plane"
    ui_combo .scvcutmanager.controls.sensitivity 1
    ui_combo .scvcutmanager.controls.motion Translate
    ui_mark .scvcutmanager.controls.translate
    set w .scvcutmanager.controls.translate
    lassign [$w coords [$w get]] px py
    event generate $w <ButtonPress-1> -x $px -y $py
    foreach target {0.4 0.5 0.6} {
        lassign [$w coords $target] x y
        event generate $w <B1-Motion> -x $x -y $y -state 256
        ui_pump
        ui_near [dict get [ui_cut x] offset] [$w get] "Slider did not recompute before release"
        ui_check_section x
    }
    event generate $w <ButtonRelease-1> -x $x -y $y
    ui_pump
    ui_combo .scvcutmanager.controls.motion Rotate
    ui_mark .scvcutmanager.controls.rotate
    foreach axis {2 3} {
        scvcutview $ui_model move x 1 1 1 -0.6
        ::CutViewManager::refresh 1
        ui_combo .scvcutmanager.controls.axis $axis
        set before [dict get [ui_cut x] plane]
        .scvcutmanager.controls.rotate set 20
        ui_pump
        ui_assert {[dict get [ui_cut x] plane] ne $before} "Rotation slider failed on axis $axis"
        ui_check_section x
    }
    ui_combo .scvcutmanager.controls.motion Translate
    ui_click .scvcutmanager.controls.gizmo
    ui_assert {![dict get [scvcutview $ui_model handleinfo] gizmo]} "Manipulator toggle-off failed"
    ui_click .scvcutmanager.controls.gizmo
    ui_assert {[dict get [scvcutview $ui_model handleinfo] gizmo]} "Manipulator toggle-on failed"

    ui_combo .scvcutmanager.actions.shape Plane
    ui_click .scvcutmanager.actions.create
    set created $::CutViewManager::selected
    ui_assert {[dict get [ui_cut $created] shape] eq "plane"} "Create did not create a plane"
    ui_click .scvcutmanager.actions.copy
    ui_assert {$::CutViewManager::selected ne $created} "Copy did not create an independent cut"
    ui_click .scvcutmanager.actions.delete
    ui_tree $created
    ui_click .scvcutmanager.actions.delete
    ui_tree x
    ui_click .scvcutmanager.header.multiple
    ui_assert {$::CutViewManager::multiple == 0} "Multiple toggle failed"
    ui_tree y show
    ui_assert {[dict get [ui_cut y] enabled] && ![dict get [ui_cut x] enabled]} "Single-cut activation left two cuts enabled"
    ui_click .scvcutmanager.header.multiple
    ui_tree x show
    ui_assert {[dict get [ui_cut y] enabled] && [dict get [ui_cut x] enabled]} "Multiple cuts did not remain independent"
    scvcutview $ui_model enable y 0
    ui_tree x

    foreach mode {wireframe cloud_edges cloud} {
        ui_combo .scvcutmanager.results.displaymode $mode
        ui_assert {[scvcutview $ui_model displaymode] eq $mode} "Manager mode selection ignored"
    }
    ui_combo .scvcutmanager.results.field "pressure (cell)"
    ui_combo .scvcutmanager.results.cellmode projected
    ui_assert {[dict get [ui_cut x] cellmode] eq "projected"} "Projected CellData menu failed"
    ui_combo .scvcutmanager.results.cellmode constant
    ui_combo .scvcutmanager.results.field "No Result"
    ui_assert {[.scvcutmanager.results.component instate disabled]} "No Result leaves the component spinner enabled"
    ui_combo .scvcutmanager.results.field "temperature (point)"
    # Recovery controls are part of the real widget inventory and Save/Load state.
    ui_entry .scvcutmanager.recovery.threshold 0 1
    ui_near [dict get [scvcutview $ui_model recovery] threshold] 0 "Recovery threshold Return"
    foreach invalid {nonsense -1 101 NaN Inf} {
        set saved [scvcutview $ui_model snapshot]
        ui_entry .scvcutmanager.recovery.threshold $invalid 1
        ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $saved} "Invalid recovery threshold changed state"
    }
    ui_entry .scvcutmanager.recovery.threshold 99
    ui_click .scvcutmanager.recovery.apply; ui_ok
    ui_near [dict get [scvcutview $ui_model recovery] threshold] 99 "Recovery Apply"
    ui_click .scvcutmanager.recovery.before; ui_ok
    ui_assert {[dict get [scvcutview $ui_model recovery] before]} "Before-average control"
    ui_combo .scvcutmanager.recovery.region "pressure (cell)"; ui_ok
    ui_assert {[llength [dict get [scvcutview $ui_model recovery] regions]] > 0} "Region field menu"
    ui_combo .scvcutmanager.recovery.region "Current regions"; ui_ok
    ui_combo .scvcutmanager.recovery.region "All regions"; ui_ok
    ui_assert {![llength [dict get [scvcutview $ui_model recovery] regions]]} "Clear recovery regions"
    ui_combo .scvcutmanager.recovery.region "pressure (cell)"; ui_ok
    ui_mark .scvcutmanager.results.component
    event generate .scvcutmanager.results.component <<Increment>>
    ui_pump
    ui_ok
    ui_entry .scvcutmanager.results.component nonsense 1
    ui_assert {$::CutViewManager::status ne ""} "Invalid component was silently accepted"
    ui_entry .scvcutmanager.results.component 9 1
    ui_assert {$::CutViewManager::status ne ""} "Out-of-range component was silently accepted"
    ui_entry .scvcutmanager.results.component 0 1
    ui_ok
    foreach command {
        {field point temperature 9} {field point temperature bogus} {visibility x 1.5}
        {visibility x 99999999999999999999999} {enable x 3} {move x 1 0 0 bogus}
        {rotate x bogus 20} {handle x 2} {move x 1 0 0 Inf}
    } {
        set saved [scvcutview $ui_model snapshot]
        set fields [scvcutview $ui_model fields]
        ui_assert {[catch {scvcutview $ui_model {*}$command}]} "Invalid command accepted: $command"
        ui_assert {[scvcutview $ui_model snapshot] eq $saved && [scvcutview $ui_model fields] eq $fields} "Invalid command left partial state: $command"
        scvcutview $ui_model eval x
    }
    set path [file join $controls_output "definitions with spaces.json"]
    file delete -force $path
    set saved [scvcutview $ui_model snapshot]
    ui_dialog .scvcutmanager.actions.save $path
    ui_ok
    ui_assert {[file exists $path]} "Save button did not write definitions"
    scvcutview $ui_model recovery 100 0 none
    scvcutview $ui_model remove y
    ui_dialog .scvcutmanager.actions.load $path
    ui_ok
    ui_assert {[scvcutview $ui_model snapshot] eq $saved} "Load button did not restore definitions: saved=$saved actual=[scvcutview $ui_model snapshot]"
    ui_near $::CutViewManager::recoveryThreshold 99 "Load did not refresh recovery threshold"
    ui_assert {$::CutViewManager::recoveryBefore && $::CutViewManager::recoveryRegion eq "Current regions"} "Load did not refresh recovery policy widgets"
    ui_dialog .scvcutmanager.actions.save $path 1
    ui_dialog .scvcutmanager.actions.load $path 1
    ui_assert {[scvcutview $ui_model snapshot] eq $saved} "Cancelled dialog changed definitions"
    set invalid [file join $controls_output invalid.json]
    set stream [open $invalid w]; puts $stream {not JSON}; close $stream
    ui_dialog .scvcutmanager.actions.load $invalid
    ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $saved} "Invalid file load was not atomic"
    ui_click .scvcutmanager.controls.apply
    ui_ok

    # Cover the already-present optional shape controls without using them for plane acceptance.
    ui_combo .scvcutmanager.actions.shape Isosurface
    ui_click .scvcutmanager.actions.create
    ui_ok
    ui_assert {[dict get [ui_cut $::CutViewManager::selected] shape] eq "isosurface"} "Optional shape selector failed"
    ui_combo .scvcutmanager.controls.isofield "pressure (cell)"
    ui_mark .scvcutmanager.controls.isocomponent
    event generate .scvcutmanager.controls.isocomponent <<Increment>>
    ui_pump
    ui_ok
    ui_click .scvcutmanager.actions.delete

    # Curved shapes reuse the portion/result controls and add three center entries.
    foreach shape {Sphere Cylinder} {
        ui_combo .scvcutmanager.actions.shape $shape
        ui_click .scvcutmanager.actions.create
        ui_ok
        set curved $::CutViewManager::selected
        ui_assert {[dict get [ui_cut $curved] shape] eq [string tolower $shape]} "Curved shape creation failed"
        foreach widget {cx cy cz position} value {0.2 0.2 0.2 0.08} {
            ui_entry .scvcutmanager.controls.$widget $value
        }
        if {$shape eq "Cylinder"} {
            foreach widget {nx ny nz} value {0 0 2} { ui_entry .scvcutmanager.controls.$widget $value }
        }
        ui_click .scvcutmanager.controls.apply
        ui_ok
        foreach value [dict get [ui_cut $curved] center] { ui_near $value 0.2 "Curved center input" }
        ui_near [dict get [ui_cut $curved] radius] 0.08 "Curved radius input"
        set saved [scvcutview $ui_model snapshot]
        ui_entry .scvcutmanager.controls.position 0 1
        ui_assert {$::CutViewManager::status ne "" && [scvcutview $ui_model snapshot] eq $saved} "Zero radius changed the cut"
        ui_entry .scvcutmanager.controls.position 0.08 1
        ui_ok
        set w .scvcutmanager.controls.translate
        lassign [$w coords [$w get]] px py
        event generate $w <ButtonPress-1> -x $px -y $py
        foreach target {0.1 0.15} {
            lassign [$w coords $target] x y
            event generate $w <B1-Motion> -x $x -y $y -state 256
            ui_pump
            ui_near [dict get [ui_cut $curved] radius] [$w get] "Radius slider did not update before release"
            set shown [scvcutview $ui_model presentation $curved]
            set computed [scvcutview $ui_model eval $curved]
            ui_assert {[dict get $shown revision] == [dict get $computed revision]} "Curved slider left a stale display"
        }
        event generate $w <ButtonRelease-1> -x $x -y $y
        ui_pump
        ui_click .scvcutmanager.controls.gizmo
        ui_assert {![dict get [scvcutview $ui_model handleinfo] gizmo]} "Curved gizmo off"
        ui_click .scvcutmanager.controls.gizmo
        ui_assert {[dict get [scvcutview $ui_model handleinfo] gizmo]} "Curved gizmo on"
        if {$shape eq "Cylinder"} {
            ui_combo .scvcutmanager.controls.motion Rotate
            ui_combo .scvcutmanager.controls.axis X
            .scvcutmanager.controls.rotate set 20
            ui_pump
            ui_assert {abs([lindex [dict get [ui_cut $curved] axis] 1]) > 0.1} "Cylinder axis rotation failed"
        }
        ui_click .scvcutmanager.actions.copy
        ui_assert {[dict get [ui_cut $::CutViewManager::selected] shape] eq [string tolower $shape]} "Curved copy lost shape"
        ui_click .scvcutmanager.actions.delete
        ui_tree $curved
        set saved [scvcutview $ui_model snapshot]
        ui_dialog .scvcutmanager.actions.save $path
        ui_dialog .scvcutmanager.actions.load $path
        ui_ok
        ui_assert {[scvcutview $ui_model snapshot] eq $saved} "Curved definitions did not round-trip"
        ui_tree $curved
        ui_click .scvcutmanager.actions.delete
    }

    ui_click .scvcutmanager.freebody
    ui_assert {[winfo exists .scvcutfreebody]} "Manager free-body options did not open"
    .scvcutfreebody.actions.close invoke
    # Deleting the last cut disables every cut-scoped control; Create remains usable.
    foreach cut [scvcutview $ui_model list] { scvcutview $ui_model remove [dict get $cut id] }
    ::CutViewManager::refresh
    foreach widget {apply show nx ny nz cx cy cz position name gizmo below on above motion axis} {
        ui_assert {[.scvcutmanager.controls.$widget instate disabled]} "Empty manager has enabled control $widget"
    }
    ui_combo .scvcutmanager.actions.shape Plane
    ui_click .scvcutmanager.actions.create
    ui_ok
    set id $::CutViewManager::selected
    set plane [dict get [ui_cut $id] plane]
    set origin [expr {-[lindex $plane 3]}]
    lassign [scvcutview $ui_model project $origin 0.1 0.1] px py
    scvcutview $ui_model mouse move $px $py
    ui_assert {[dict get [scvcutview $ui_model handleinfo] detected] eq "plane"} "Cannot pick plane for Close-during-drag test"
    scvcutview $ui_model mouse down $px $py
    scvcutview $ui_model mouse move [expr {$px-10}] $py
    ui_assert {[dict get [scvcutview $ui_model handleinfo] dragging]} "Close-during-drag test did not start dragging"
    set nativeHeld [scvcutview $ui_model nativeinfo]
    ui_assert {[dict get $nativeHeld dragging] && [dict get $nativeHeld begin] > 0 && [dict get $nativeHeld update] > 0} "Native mouse drag events missing"
    ui_capture [file join $controls_output manager_controls.png]
    ui_click .scvcutmanager.close
    ui_assert {![winfo exists .scvcutmanager] && $::CutViewManager::timer eq ""} "Close left a window/timer"
    ui_assert {[dict get [scvcutview $ui_model handleinfo] id] eq ""} "Close left a positioning aid"
    ui_assert {![dict get [scvcutview $ui_model nativeinfo] dragging] && [dict get [scvcutview $ui_model nativeinfo] abort] > 0} "Native mouse abort notification missing"
    foreach actual [dict get [ui_cut $id] plane] expectedValue $plane { ui_near $actual $expectedValue "Close did not cancel active plane drag" }
    scvcutview $ui_model clear
    # New models receive defaults once; replacements rebuild fields, bounds and controls.
    set ui_model lifecycle
    set fixtures [file join $controls_dir .. native fixtures]
    scvcutview $ui_model read [file join $fixtures tetra.vtu]
    scvcutview $ui_model display
    scvcutmanager $ui_model
    ui_pump
    ui_assert {[llength [scvcutview $ui_model list]] == 3} "New-model default XYZ planes missing"
    foreach cut [scvcutview $ui_model list] { ui_assert {![dict get $cut enabled]} "New model started clipped" }
    set info [scvcutview $ui_model info]
    set saved [scvcutview $ui_model snapshot]
    ui_assert {[catch {scvcutview $ui_model replace [file join $fixtures missing.vtu]}]} "Invalid replacement accepted"
    ui_assert {[scvcutview $ui_model info] eq $info && [scvcutview $ui_model snapshot] eq $saved} "Failed replacement destroyed model"
    ui_tree x show
    scvcutview $ui_model fit
    lassign [scvcutview $ui_model project 0.5 0.1 0.1] px py
    scvcutview $ui_model mouse move $px $py
    scvcutview $ui_model mouse down $px $py
    scvcutview $ui_model mouse move [expr {$px-10}] $py
    ui_assert {[dict get [scvcutview $ui_model handleinfo] dragging]} "Replace-during-drag test did not start dragging"
    scvcutview $ui_model replace [file join $fixtures stress_heat.vtu]
    ui_pump 200
    ui_ok
    ui_assert {$::CutViewManager::generation > [dict get $info generation]} "Manager did not follow model replacement"
    ui_assert {[llength [scvcutview $ui_model list]] == 3} "Replacement default planes missing"
    ui_combo .scvcutmanager.results.field "heat_flux (point)"
    event generate .scvcutmanager.results.component <<Increment>>
    ui_pump
    ui_ok
    ui_assert {$::CutViewManager::component == 1} "Vector component spinner did not change the component"
    ui_combo .scvcutmanager.results.field "stress (point)"
    ui_entry .scvcutmanager.results.component 5 1
    ui_ok
    scvcutview $ui_model replace [file join $fixtures tetra.vtm.series]
    ui_pump 200
    ui_combo .scvcutmanager.results.field "temperature (point) - step 1"
    set found 0
    foreach field [scvcutview $ui_model fields] {
        if {[dict get $field active]} { ui_assert {[dict get $field step] == 1} "Result timestep menu failed"; set found 1 }
    }
    ui_assert {$found} "Timestep menu did not bind a result"
    scvcutview $ui_model replace [file join $fixtures tensor_hex.vtu]
    ui_pump 200
    ui_combo .scvcutmanager.results.field "stress (cell)"
    ui_combo .scvcutmanager.tensor.layout xx-yy-zz-xy-yz-xz
    ui_combo .scvcutmanager.tensor.layout row-major
    ui_combo .scvcutmanager.tensor.measure mises
    ui_combo .scvcutmanager.tensor.shear engineering
    ui_assert {[dict get [scvcutview $ui_model tensor] measure] eq "mises" && [dict get [scvcutview $ui_model tensor] shear] eq "engineering"} "Tensor controls did not apply conventions"
    ui_combo .scvcutmanager.tensor.layout column-major
    ui_combo .scvcutmanager.tensor.layout row-major
    ui_combo .scvcutmanager.tensor.shear tensor
    ui_combo .scvcutmanager.tensor.measure maxprincipal
    ui_combo .scvcutmanager.tensor.measure tresca
    ui_combo .scvcutmanager.tensor.measure components
    ui_combo .scvcutmanager.tensor.measure mises
    ui_capture [file join $controls_output tensor_manager.png]
    set missing {}
    foreach widget $expected { if {$widget ni $::ui_covered} { lappend missing $widget } }
    ui_assert {[llength $missing] == 0} "Untested controls: $missing"
    puts "CUT_VIEW_CONTROL_COVERAGE: [llength $expected]/[llength $expected] $expected"
    scvcutview $ui_model clear
    ui_pump 200
    ui_assert {![winfo exists .scvcutmanager] && $::CutViewManager::timer eq ""} "Closing a model left stale manager controls"
    puts "CUT_VIEW_CONTROLS_PASS: $controls_output"
}
if {[catch {ui_run} message options]} {
    puts stderr "CUT_VIEW_CONTROLS_FAIL: $message\n[dict get $options -errorinfo]"
    exit 1
}
exit 0
