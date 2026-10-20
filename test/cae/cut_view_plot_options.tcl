# Portion plot options. Opening/reloading only queries the live model.
namespace eval ::CutViewOptions {
    variable w .scvcutoptions
    variable model ""
    variable generation 0
    variable cut ""
    variable scope "Model defaults"
    variable status ""
    variable values
    variable modes {wireframe cloud cloud_edges shrink free_edges sharp_edges}
}

proc ::CutViewOptions::owner {} {
    variable model; variable generation; variable cut; variable scope
    if {[dict get [scvcutview $model info] generation] != $generation} { error "Model changed; reopen View Cut Options" }
    if {$scope eq "Model defaults"} { return defaults }
    foreach definition [scvcutview $model list] { if {[dict get $definition id] eq $cut} { return $cut } }
    error "The selected cut was removed; reopen View Cut Options"
}

proc ::CutViewOptions::execute {command args} {
    variable status
    if {[catch {uplevel #0 [list $command {*}$args]} result]} { set status $result } else { set status "" }
}

proc ::CutViewOptions::load {{portions {0 1 2}}} {
    variable w; variable model; variable cut; variable values; variable modes
    set target [owner]
    foreach portion $portions {
        set style [scvcutview $model cutstyle $target $portion]
        foreach key {transparency width} { set values($portion,$key) [format %.7g [dict get $style $key]] }
        set mode [dict get $style mode]
        set values($portion,mode) [lindex $modes $mode]
        # Preserve uncommon existing modes when editing other style properties.
        if {$values($portion,mode) eq ""} { set values($portion,mode) "mode:$mode" }
        lassign [dict get $style color] values($portion,r) values($portion,g) values($portion,b)
        foreach channel {r g b} { set values($portion,$channel) [format %.7g $values($portion,$channel)] }
        set values($portion,override) [dict get $style override]
        $w.tabs.p$portion.override configure -state [expr {$target eq "defaults" ? "disabled" : "normal"}]
    }
    $w.target configure -text [expr {$target eq "defaults" ? "Model defaults (all cuts without an override)" : "Cut: $cut (pinned when this dialog opened)"}]
}

proc ::CutViewOptions::apply {} {
    variable w; variable model; variable values; variable modes
    set target [owner]
    set portion [$w.tabs index current]
    if {$target ne "defaults" && !$values($portion,override)} {
        scvcutview $model clearstyle $target $portion
    } else {
        set transparency [::CutViewManager::number $values($portion,transparency) Transparency]
        set width [::CutViewManager::number $values($portion,width) "Line width"]
        if {$transparency < 0 || $transparency > 1} { error {Transparency must be in [0,1]} }
        if {$width <= 0} { error "Line width must be positive" }
        set rgb {}
        foreach channel {r g b} {
            set value [::CutViewManager::number $values($portion,$channel) "Face RGB"]
            if {$value < 0 || $value > 1} { error {Face RGB components must be in [0,1]} }
            lappend rgb $value
        }
        set mode [lsearch -exact $modes $values($portion,mode)]
        if {$mode < 0 && ![regexp {^mode:([0-9]+)$} $values($portion,mode) -> mode]} { error "Invalid display mode" }
        scvcutview $model cutstyle $target $portion $transparency $width $mode {*}$rgb
    }
    ::CutViewManager::refresh
    load [list $portion]
}

proc scvcutoptions {model {scope "Model defaults"}} {
    set w $::CutViewOptions::w
    if {[winfo exists $w]} { destroy $w }
    set ::CutViewOptions::model $model
    set ::CutViewOptions::generation [dict get [scvcutview $model info] generation]
    set ::CutViewOptions::cut $::CutViewManager::selected
    set ::CutViewOptions::scope $scope
    set ::CutViewOptions::status ""
    toplevel $w
    wm title $w "View Cut Options"
    ttk::combobox $w.scope -state readonly -values {"Model defaults" "Selected cut override"} -textvariable ::CutViewOptions::scope -width 28
    pack $w.scope -fill x -padx 8 -pady 6
    bind $w.scope <<ComboboxSelected>> {::CutViewOptions::execute ::CutViewOptions::load}
    ttk::label $w.target -wraplength 450
    pack $w.target -fill x -padx 8
    ttk::notebook $w.tabs
    pack $w.tabs -fill both -expand 1 -padx 8 -pady 6
    foreach portion {0 1 2} label {"Below Cut" "On Cut" "Above Cut"} {
        set tab $w.tabs.p$portion
        ttk::frame $tab -padding 8
        $w.tabs add $tab -text $label
        ttk::checkbutton $tab.override -text "Override model defaults (project extension)" -variable ::CutViewOptions::values($portion,override)
        pack $tab.override -anchor w -pady 4
        foreach key {mode transparency width} title {Display "Transparency (0 opaque, 1 invisible)" "Edge width (pixels)"} {
            ttk::frame $tab.$key
            ttk::label $tab.$key.label -text $title -width 35
            pack $tab.$key.label -side left
            if {$key eq "mode"} {
                ttk::combobox $tab.$key.value -state readonly -values {cloud wireframe cloud_edges} -textvariable ::CutViewOptions::values($portion,$key) -width 15
            } else {
                ttk::entry $tab.$key.value -textvariable ::CutViewOptions::values($portion,$key) -width 15
            }
            pack $tab.$key.value -side right -fill x -expand 1
            pack $tab.$key -fill x -pady 4
        }
        ttk::labelframe $tab.color -text "Face RGB (0..1, used without a scalar field)" -padding 4
        foreach channel {r g b} {
            ttk::label $tab.color.l$channel -text [string toupper $channel]
            ttk::entry $tab.color.$channel -textvariable ::CutViewOptions::values($portion,$channel) -width 10
            pack $tab.color.l$channel $tab.color.$channel -side left -padx 3
        }
        pack $tab.color -fill x -pady 5
    }
    ttk::label $w.note -text "Apply changes only the current tab. Mixed modes or transparency may use the CPU backend. Save the full session to retain styles." -wraplength 450
    pack $w.note -fill x -padx 8 -pady 4
    ttk::label $w.status -textvariable ::CutViewOptions::status -foreground #a02020 -wraplength 450
    pack $w.status -fill x -padx 8
    ttk::frame $w.actions -padding 8
    foreach name {apply reload close} text {"Apply tab" Reload Close} command {
        {::CutViewOptions::execute ::CutViewOptions::apply}
        {::CutViewOptions::execute ::CutViewOptions::load}
        {destroy .scvcutoptions}
    } {
        ttk::button $w.actions.$name -text $text -command $command
        pack $w.actions.$name -side left -padx 4
    }
    pack $w.actions -fill x
    ::CutViewOptions::execute ::CutViewOptions::load
}

# Independent rename must not apply pending numeric geometry edits.
namespace eval ::CutViewRename { variable model ""; variable cut ""; variable generation 0; variable name ""; variable status "" }
proc ::CutViewManager::renameDialog {} {
    variable selected; variable model
    if {$selected eq ""} { return }
    catch {destroy .scvcutrename}
    set ::CutViewRename::model $model
    set ::CutViewRename::cut $selected
    set ::CutViewRename::generation [dict get [scvcutview $model info] generation]
    foreach definition [scvcutview $model list] {
        if {[dict get $definition id] eq $selected} { set ::CutViewRename::name [dict get $definition name]; break }
    }
    set ::CutViewRename::status ""
    toplevel .scvcutrename
    wm title .scvcutrename "Rename View Cut"
    ttk::label .scvcutrename.target -text "Cut ID: $selected"
    ttk::entry .scvcutrename.name -textvariable ::CutViewRename::name -width 36
    ttk::button .scvcutrename.apply -text Rename -command ::CutViewRename::apply
    ttk::button .scvcutrename.cancel -text Cancel -command {destroy .scvcutrename}
    ttk::label .scvcutrename.status -textvariable ::CutViewRename::status -foreground #a02020 -wraplength 350
    pack .scvcutrename.target .scvcutrename.name .scvcutrename.apply .scvcutrename.cancel .scvcutrename.status -fill x -padx 8 -pady 4
    bind .scvcutrename.name <Return> ::CutViewRename::apply
    focus .scvcutrename.name
    .scvcutrename.name selection range 0 end
}
proc ::CutViewRename::apply {} {
    variable model; variable cut; variable generation; variable name; variable status
    if {[catch {
        if {[string trim $name] eq ""} { error "Cut name must not be empty" }
        if {[dict get [scvcutview $model info] generation] != $generation} { error "Model changed; reopen Rename" }
        scvcutview $model rename $cut $name
    } message]} { set status $message; return }
    destroy .scvcutrename
    ::CutViewManager::refresh 1
}
