# Complete plane definition editor used by Cut View Manager.
namespace eval ::CutViewPlacement {
    variable w .scvcutplacement
    variable model ""
    variable id ""
    variable generation 0
    variable method "Key-in"
    variable datum global
    variable normalAxis 1
    variable status ""
    variable values
}
proc ::CutViewPlacement::open {} {
    variable w; variable model; variable id; variable generation; variable values; variable method; variable status
    set model $::CutViewManager::model; set id $::CutViewManager::selected
    if {$id eq ""} { return }
    foreach cut [scvcutview $model list] { if {[dict get $cut id] eq $id} { break } }
    if {[dict get $cut shape] ne "plane"} { error "Definition requires a plane cut" }
    set generation [dict get [scvcutview $model info] generation]
    set method "Key-in"; set status ""
    if {[winfo exists $w]} { destroy $w }
    toplevel $w; wm title $w "Plane Definition: [dict get $cut name]"
    ttk::frame $w.body -padding 12; pack $w.body -fill both -expand 1
    ttk::combobox $w.body.method -state readonly -values {Key-in {From CSYS}} -textvariable ::CutViewPlacement::method
    grid $w.body.method -row 0 -column 0 -columnspan 4 -sticky ew -pady 4
    set row 1
    foreach key {origin normal axis2} label {Origin {Normal (axis 1)} {In-plane axis 2}} {
        ttk::label $w.body.${key}Label -text $label; grid $w.body.${key}Label -row $row -column 0 -sticky w
        foreach index {0 1 2} {
            set values($key,$index) [lindex [dict get $cut $key] $index]
            ttk::entry $w.body.${key}$index -width 15 -textvariable ::CutViewPlacement::values($key,$index)
            grid $w.body.${key}$index -row $row -column [expr {$index+1}] -padx 3 -pady 4
        }
        incr row
    }
    ttk::label $w.body.datumLabel -text CSYS
    ttk::combobox $w.body.datum -state readonly -values [scvcutview $model datums] -textvariable ::CutViewPlacement::datum
    ttk::label $w.body.axisLabel -text "Normal axis"
    ttk::combobox $w.body.axis -state readonly -values {1 2 3} -width 5 -textvariable ::CutViewPlacement::normalAxis
    grid $w.body.datumLabel -row 4 -column 0 -sticky w
    grid $w.body.datum -row 4 -column 1 -sticky ew
    grid $w.body.axisLabel -row 4 -column 2
    grid $w.body.axis -row 4 -column 3
    ttk::label $w.body.help -text "Apply defines a new base frame and resets motion.\nCSYS axes use cyclic order: normal / next axis / remaining axis.\nCSYS is sampled at Apply; use Viewer Tools to define datums." -wraplength 490
    grid $w.body.help -row 5 -column 0 -columnspan 4 -sticky w -pady 8
    ttk::label $w.body.status -textvariable ::CutViewPlacement::status -foreground firebrick -wraplength 490
    grid $w.body.status -row 6 -column 0 -columnspan 4 -sticky w
    ttk::button $w.body.apply -text Apply -command {if {[catch {::CutViewPlacement::apply} message]} {set ::CutViewPlacement::status $message}}
    ttk::button $w.body.close -text Close -command [list destroy $w]
    grid $w.body.apply -row 7 -column 2 -pady 8; grid $w.body.close -row 7 -column 3
    bind $w.body.method <<ComboboxSelected>> {::CutViewPlacement::methodChanged}
    methodChanged
    bind $w <Escape> [list destroy $w]
}
proc ::CutViewPlacement::methodChanged {} {
    variable w; variable method
    foreach key {origin normal axis2} { foreach i {0 1 2} {
        $w.body.${key}$i configure -state [expr {$method eq "Key-in" ? "normal" : "disabled"}]
    } }
    foreach key {datum axis} { $w.body.$key configure -state [expr {$method eq "From CSYS" ? "readonly" : "disabled"}] }
}
proc ::CutViewPlacement::apply {} {
    variable model; variable id; variable generation; variable method; variable datum; variable normalAxis; variable values; variable status
    if {[dict get [scvcutview $model info] generation] != $generation} { error "Model was replaced; reopen Definition" }
    if {$method eq "From CSYS"} { scvcutview $model fromcsys $id $datum $normalAxis } else {
        set args {}
        foreach key {origin normal axis2} { foreach i {0 1 2} { lappend args [::CutViewManager::number $values($key,$i) $key] } }
        scvcutview $model placement $id {*}$args
    }
    set status ""; ::CutViewManager::refresh 1
}
proc ::CutViewManager::sliderRange {} {
    variable w; variable model; variable selected; variable position; variable sensitivity; variable busy
    if {$selected eq "" || ![winfo exists $w.controls.translate]} { return }
    if {$sensitivity ni {1 10 100}} { error "Sensitivity must be 1, 10 or 100" }
    lassign [scvcutview $model motionrange $selected] lower upper
    set width [expr {max(1e-12,$upper-$lower)}]
    if {$sensitivity == 1} {
        set lower [expr {min($lower,$position)}]; set upper [expr {max($upper,$position,$lower+1e-12)}]
    } else {
        set lower [expr {$position-$width/(2.0*$sensitivity)}]
        set upper [expr {$position+$width/(2.0*$sensitivity)}]
    }
    set previous $busy; set busy 1
    try { $w.controls.translate configure -from $lower -to $upper } finally { set busy $previous }
}
