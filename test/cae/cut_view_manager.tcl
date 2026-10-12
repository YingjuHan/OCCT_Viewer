# DRAW/Tk cut manager. Source this file, then call scvcutmanager with a loaded model.
if {[llength [info commands ::CutViewManager::close]]} { ::CutViewManager::close }
namespace eval ::CutViewManager {
    variable w .scvcutmanager
    variable model ""
    variable generation 0
    variable selected ""
    variable busy 0
    variable multiple 1
    variable gizmo 1
    variable motion Translate
    variable axis Y
    variable angle 0
    variable lastAngle 0
    variable position 0
    variable positionText 0
    variable sliderSnapshot ""
    variable normal {1 0 0}
    variable shape plane
    variable createShape Plane
    variable isoFieldLabel ""
    variable isoComponent 0
    variable nx 1
    variable ny 0
    variable nz 0
    variable cx 0
    variable cy 0
    variable cz 0
    variable name ""
    variable enabled 1
    variable below 0
    variable on 1
    variable above 0
    variable cellmode constant
    variable fieldLabel "No Result"
    variable component 0
    variable recoveryThreshold 100
    variable recoveryBefore 0
    variable recoveryRegion "All regions"
    variable recoveryFields {}
    variable recoveryLabels {}
    variable lastRecovery ""
    variable tensorMeasure components
    variable tensorLayout xx-yy-zz-xy-xz-yz
    variable tensorShear tensor
    variable displayTracePolicy lowest
    variable displayTraceRegion 0
    variable displayTraceRegions {}
    variable physicalTracePolicy lowest
    variable physicalTraceRegion 0
    variable physicalTraceRegions {}
    variable lastTrace {}
    variable lastTraceKey ""
    variable fields {}
    variable fieldLabels {}
    variable status ""
    variable lastPlane {}
    variable lastShape ""
    variable lastBounds {}
    variable lastDragging 0
    variable serial 0
    variable timer ""
}
source [file join [file dirname [info script]] cut_view_tools.tcl]

proc ::CutViewManager::invoke {command args} {
    variable status
    if {[catch {uplevel #0 [list $command {*}$args]} message]} { set status $message } else { set status "" }
}

proc ::CutViewManager::refresh {{attach 0}} {
    variable w; variable model; variable selected; variable busy; variable lastPlane; variable lastDragging
    variable nx; variable ny; variable nz; variable normal; variable name; variable position; variable positionText
    variable cx; variable cy; variable cz
    variable enabled; variable below; variable on; variable above; variable cellmode
    variable angle; variable lastAngle; variable gizmo
    variable shape; variable lastShape; variable motion; variable isoFieldLabel; variable isoComponent; variable fields; variable fieldLabels
    variable generation; variable lastBounds
    if {$busy || ![winfo exists $w]} { return }
    if {[catch {scvcutview $model info} info]} { ::CutViewManager::close; return }
    if {[dict get $info generation] != $generation} { scvcutmanager $model; return }
    set definitions [scvcutview $model list]
    set ids {}
    set busy 1
    try {
        foreach cut $definitions {
            set id [dict get $cut id]
            lappend ids $id
            set mask [dict get $cut visibility]
            set values [list [expr {[dict get $cut enabled] ? "\u2713" : ""}]]
            foreach bit {1 2 4} { lappend values [expr {($mask & $bit) ? "\u2713" : ""}] }
            if {[$w.cuts exists $id]} { $w.cuts item $id -text [dict get $cut name] -values $values } else {
                $w.cuts insert {} end -id $id -text [dict get $cut name] -values $values
            }
        }
        foreach id [$w.cuts children {}] { if {$id ni $ids} { $w.cuts delete $id } }
        if {$selected ni $ids} { set selected [lindex $ids 0]; set attach 1 }
        foreach widget [list $w.controls.apply $w.controls.translate $w.controls.rotate $w.controls.show $w.actions.copy $w.actions.delete \
            $w.controls.name $w.controls.nx $w.controls.ny $w.controls.nz $w.controls.cx $w.controls.cy $w.controls.cz $w.controls.position $w.controls.gizmo \
            $w.controls.below $w.controls.on $w.controls.above $w.controls.isocomponent] {
            $widget configure -state [expr {$selected eq "" ? "disabled" : "normal"}]
        }
        foreach widget [list $w.controls.motion $w.controls.axis $w.controls.isofield $w.results.cellmode] {
            $widget configure -state [expr {$selected eq "" ? "disabled" : "readonly"}]
        }
        ::CutViewManager::recoveryRefresh
        foreach role {display physical} {
            $w.traces.${role}Policy configure -state [expr {$selected eq "" ? "disabled" : "readonly"}]
            foreach suffix {Region Regions Apply} { $w.traces.$role$suffix configure -state [expr {$selected eq "" ? "disabled" : "normal"}] }
        }
        if {$selected eq ""} { set lastPlane {}; set lastShape ""; return }
        if {[$w.cuts selection] ne [list $selected]} { $w.cuts selection set $selected }
        foreach cut $definitions { if {[dict get $cut id] eq $selected} { break } }
        set shape [dict get $cut shape]
        if {$shape eq "plane"} { set plane [dict get $cut plane] } elseif {$shape in {sphere cylinder}} {
            set plane [list $shape [dict get $cut center] [dict get $cut axis] [dict get $cut radius]]
        } else {
            set plane [list $shape [dict get $cut field] [dict get $cut mapping] [dict get $cut component] [dict get $cut step] [dict get $cut value]]
        }
        set info [scvcutview $model handleinfo]
        set dragging [dict get $info dragging]
        set changed [expr {$plane ne $lastPlane}]
        set bounds [scvcutview $model bounds]
        if {$changed || $attach || $bounds ne $lastBounds} {
            if {$shape eq "plane"} { set normal [lrange $plane 0 2] }
            if {$shape eq "cylinder"} { set normal [dict get $cut axis] }
            set focused [focus]
            if {$attach || $dragging || $lastDragging || $focused ni [list $w.controls.nx $w.controls.ny $w.controls.nz $w.controls.cx $w.controls.cy $w.controls.cz $w.controls.name $w.controls.position]} {
                if {$shape eq "plane"} {
                    lassign $normal nx ny nz
                    foreach var {nx ny nz} { set $var [format %.10g [set $var]] }
                    set position [expr {-[lindex $plane 3]}]
                } elseif {$shape in {sphere cylinder}} {
                    lassign [dict get $cut center] cx cy cz
                    lassign [dict get $cut axis] nx ny nz
                    foreach var {cx cy cz nx ny nz} { set $var [format %.10g [set $var]] }
                    set position [dict get $cut radius]
                } else {
                    set position [dict get $cut value]
                    set isoComponent [dict get $cut component]
                    set index 0
                    foreach field $fields {
                        incr index
                        if {[dict get $field name] eq [dict get $cut field] && [dict get $field mapping] eq [dict get $cut mapping]
                            && [dict get $field step] == [dict get $cut step]} {
                            set isoFieldLabel [lindex $fieldLabels $index]
                            $w.controls.isocomponent configure -to [expr {[dict get $field components]-1}]
                            break
                        }
                    }
                }
                set positionText [format %.12g $position]
                set name [dict get $cut name]
            }
            lassign [scvcutview $model range $selected] lower upper
            set lower [expr {min($lower,$position)}]; set upper [expr {max($upper,$position)}]
            if {$upper <= $lower} { set upper [expr {$lower+1}] }
            $w.controls.translate configure -from $lower -to $upper
            if {$shape ne $lastShape} {
                pack forget $w.controls.nx $w.controls.ny $w.controls.nz $w.controls.isofield $w.controls.isocomponent
                pack forget $w.controls.rowcenter
                if {$shape eq "plane"} {
                    pack $w.controls.row1 -after $w.controls.row0 -fill x -pady 3
                    $w.controls.label1 configure -text Normal
                    $w.controls.label2 configure -text Position
                    foreach widget {nx ny nz} { pack $w.controls.$widget -in $w.controls.row1 -side left -fill x -expand 1 -padx 2 }
                    $w.controls.motion configure -state readonly -values {Translate Rotate}
                    if {$motion ni {Translate Rotate}} { set motion Translate }
                } elseif {$shape in {sphere cylinder}} {
                    $w.controls.label1 configure -text Axis
                    $w.controls.label2 configure -text Radius
                    pack $w.controls.rowcenter -after $w.controls.row0 -fill x -pady 3
                    if {$shape eq "cylinder"} {
                        pack $w.controls.row1 -after $w.controls.rowcenter -fill x -pady 3
                        foreach widget {nx ny nz} { pack $w.controls.$widget -in $w.controls.row1 -side left -fill x -expand 1 -padx 2 }
                        $w.controls.motion configure -state readonly -values {Radius Rotate}
                    } else {
                        pack forget $w.controls.row1
                        $w.controls.motion configure -state readonly -values {Radius}
                    }
                    set motion Radius
                } else {
                    pack $w.controls.row1 -after $w.controls.row0 -fill x -pady 3
                    $w.controls.label1 configure -text "Cut Field"
                    $w.controls.label2 configure -text Threshold
                    pack $w.controls.isocomponent -in $w.controls.row1 -side right -padx 2
                    pack $w.controls.isofield -in $w.controls.row1 -side left -fill x -expand 1 -padx 2
                    set motion Value
                    $w.controls.motion configure -state disabled -values {Value}
                }
                ::CutViewManager::motion
            }
        }
        if {$shape eq "isosurface"} {
            foreach widget {axis motion gizmo} { $w.controls.$widget configure -state disabled }
        }
        if {$shape eq "sphere"} { $w.controls.axis configure -state disabled }
        set enabled [dict get $cut enabled]
        set mask [dict get $cut visibility]
        set below [expr {($mask & 1) != 0}]; set on [expr {($mask & 2) != 0}]; set above [expr {($mask & 4) != 0}]
        set cellmode [dict get $cut cellmode]
        ::CutViewManager::traceRefresh
        if {$attach} { set angle 0; set lastAngle 0 }
        if {!$dragging && ($attach || $changed || $lastDragging)} {
            scvcutview $model handle $selected [expr {$shape ne "isosurface"}] $gizmo
        }
        set lastPlane $plane; set lastShape $shape; set lastBounds $bounds; set lastDragging $dragging
    } finally { set busy 0 }
}

proc ::CutViewManager::poll {} {
    variable w; variable timer
    if {![winfo exists $w]} { return }
    set timer ""
    if {[catch {::CutViewManager::refresh} message]} { set ::CutViewManager::status $message }
    if {[winfo exists $w] && $timer eq ""} { set timer [after 150 ::CutViewManager::poll] }
}

proc ::CutViewManager::select {} {
    variable w; variable selected; variable busy
    if {$busy} { return }
    set id [lindex [$w.cuts selection] 0]
    if {$id eq "" || $id eq $selected} { return }
    set selected $id
    refresh 1
}

proc ::CutViewManager::toggleCell {x y} {
    variable w; variable model; variable selected; variable multiple
    set id [$w.cuts identify row $x $y]; set column [$w.cuts identify column $x $y]
    if {$id eq "" || $column eq "#0"} { return }
    foreach cut [scvcutview $model list] { if {[dict get $cut id] eq $id} { break } }
    if {$column eq "#1"} {
        set active [expr {![dict get $cut enabled]}]
        if {!$multiple && $active} {
            foreach other [scvcutview $model list] { scvcutview $model enable [dict get $other id] 0 }
        }
        scvcutview $model enable $id $active
    } else {
        set bit [expr {1 << ([string range $column 1 end]-2)}]
        scvcutview $model visibility $id [expr {[dict get $cut visibility] ^ $bit}]
    }
    refresh
}

proc ::CutViewManager::setMultiple {} {
    variable model; variable selected; variable multiple
    if {!$multiple} {
        set keep ""
        foreach cut [scvcutview $model list] {
            if {[dict get $cut enabled] && ($keep eq "" || [dict get $cut id] eq $selected)} { set keep [dict get $cut id] }
        }
        foreach cut [scvcutview $model list] { scvcutview $model enable [dict get $cut id] [expr {[dict get $cut id] eq $keep}] }
    }
    refresh
}

proc ::CutViewManager::visibility {} {
    variable model; variable selected; variable below; variable on; variable above; variable enabled; variable multiple
    if {$selected eq ""} { return }
    if {!$multiple && $enabled} {
        foreach cut [scvcutview $model list] { if {[dict get $cut id] ne $selected} { scvcutview $model enable [dict get $cut id] 0 } }
    }
    scvcutview $model enable $selected $enabled
    scvcutview $model visibility $selected [expr {$below | ($on << 1) | ($above << 2)}]
    refresh
}

proc ::CutViewManager::translate {value} {
    variable busy; variable model; variable selected; variable normal; variable shape
    variable positionText
    if {$busy || $selected eq "" || [dict get [scvcutview $model handleinfo] dragging]} { return }
    if {$shape eq "isosurface"} { scvcutview $model isovalue $selected $value } elseif {$shape in {sphere cylinder}} {
        scvcutview $model radius $selected $value
    } else {
        scvcutview $model move $selected {*}$normal [expr {-$value}]
    }
    set positionText [format %.12g $value]
    refresh
}

proc ::CutViewManager::sliderBegin {} {
    variable sliderSnapshot; variable model; variable w
    set sliderSnapshot [scvcutview $model snapshot]
    focus -force $w.controls.translate
}
proc ::CutViewManager::sliderEnd {} {
    variable sliderSnapshot; set sliderSnapshot ""
}
proc ::CutViewManager::sliderAbort {} {
    variable sliderSnapshot; variable model
    if {$sliderSnapshot ne ""} {
        scvcutview $model restore $sliderSnapshot
        set sliderSnapshot ""
        refresh 1
    }
}

proc ::CutViewManager::rotate {value} {
    variable busy; variable model; variable selected; variable axis; variable lastAngle
    if {$busy || $selected eq "" || [dict get [scvcutview $model handleinfo] dragging]} { return }
    set delta [expr {$value-$lastAngle}]
    scvcutview $model rotate $selected [lsearch -exact {X Y Z} $axis] $delta
    set lastAngle $value
    refresh
}

proc ::CutViewManager::motion {} {
    variable w; variable motion; variable angle; variable lastAngle
    set angle 0; set lastAngle 0
    pack forget $w.controls.translate $w.controls.rotate
    if {$motion ne "Rotate"} {
        pack $w.controls.translate -in $w.controls.row4 -fill x
    } else { pack $w.controls.rotate -in $w.controls.row4 -fill x }
}

proc ::CutViewManager::apply {} {
    variable model; variable selected; variable nx; variable ny; variable nz; variable name; variable positionText
    variable shape; variable cx; variable cy; variable cz
    if {$selected eq ""} { return }
    if {[string trim $name] eq ""} { error "Cut name must not be empty" }
    set desiredName $name
    set position [::CutViewManager::number $positionText Position]
    if {$shape eq "isosurface"} { ::CutViewManager::isoBinding $position } elseif {$shape in {sphere cylinder}} {
        set x [::CutViewManager::number $cx "Center X"]
        set y [::CutViewManager::number $cy "Center Y"]
        set z [::CutViewManager::number $cz "Center Z"]
        if {$shape eq "sphere"} {
            scvcutview $model spheremove $selected $x $y $z $position
        } else {
            set a [::CutViewManager::number $nx "Axis X"]
            set b [::CutViewManager::number $ny "Axis Y"]
            set c [::CutViewManager::number $nz "Axis Z"]
            scvcutview $model cylindermove $selected $x $y $z $a $b $c $position
        }
    } else {
        set x [::CutViewManager::number $nx "Normal X"]
        set y [::CutViewManager::number $ny "Normal Y"]
        set z [::CutViewManager::number $nz "Normal Z"]
        set length [expr {sqrt($x*$x+$y*$y+$z*$z)}]
        if {$length < 1.0e-12} { error "Plane normal must be nonzero" }
        scvcutview $model move $selected $x $y $z [expr {-$position*$length}]
    }
    scvcutview $model rename $selected $desiredName
    refresh 1
}

proc ::CutViewManager::number {value label} {
    if {[catch {expr {double($value)}} parsed] || [catch {expr {abs($parsed) < Inf}} finite] || !$finite} {
        error "$label must be a finite number"
    }
    return $parsed
}

proc ::CutViewManager::isoField {} {
    variable fields; variable fieldLabels; variable isoFieldLabel; variable isoComponent; variable w
    set index [lsearch -exact $fieldLabels $isoFieldLabel]
    if {$index <= 0} { error "Select an isosurface field" }
    set field [lindex $fields [expr {$index-1}]]
    set max [expr {[dict get $field components]-1}]
    $w.controls.isocomponent configure -to $max
    if {![string is integer -strict $isoComponent] || $isoComponent < 0 || $isoComponent > $max} { set isoComponent 0 }
    return $field
}

proc ::CutViewManager::isoBinding {{value ""}} {
    variable busy; variable model; variable selected; variable shape; variable isoComponent; variable position
    if {$busy || $selected eq "" || $shape ne "isosurface"} { return }
    if {$value eq ""} { set value $position }
    set field [::CutViewManager::isoField]
    scvcutview $model isomove $selected [dict get $field mapping] [dict get $field name] $isoComponent $value [dict get $field step]
    refresh 1
}

proc ::CutViewManager::create {{copy 0}} {
    variable model; variable selected; variable serial; variable multiple
    variable createShape; variable isoComponent; variable isoFieldLabel; variable fieldLabel; variable component; variable shape
    set id cut_[incr serial]
    while {[lsearch -exact [lmap cut [scvcutview $model list] {dict get $cut id}] $id] >= 0} { set id cut_[incr serial] }
    if {$copy && $selected ne ""} { scvcutview $model copy $selected $id } elseif {$createShape eq "Isosurface"} {
        if {$shape eq "plane" && $fieldLabel ne "No Result"} { set isoFieldLabel $fieldLabel; set isoComponent $component }
        set field [::CutViewManager::isoField]
        scvcutview $model isoadd $id [dict get $field mapping] [dict get $field name] $isoComponent 0 [dict get $field step]
        lassign [scvcutview $model range $id] lower upper
        scvcutview $model isovalue $id [expr {($lower+$upper)/2.0}]
        scvcutview $model visibility $id 3
    } elseif {$createShape in {Sphere Cylinder}} {
        lassign [scvcutview $model bounds] xmin ymin zmin xmax ymax zmax
        set center [list [expr {($xmin+$xmax)/2.0}] [expr {($ymin+$ymax)/2.0}] [expr {($zmin+$zmax)/2.0}]]
        set radius [expr {sqrt(($xmax-$xmin)**2+($ymax-$ymin)**2+($zmax-$zmin)**2)/4.0}]
        if {$createShape eq "Sphere"} { scvcutview $model sphereadd $id {*}$center $radius } else {
            scvcutview $model cylinderadd $id {*}$center 0 0 1 $radius
        }
        scvcutview $model visibility $id 3
    } else {
        lassign [scvcutview $model bounds] xmin ymin zmin xmax ymax zmax
        scvcutview $model add $id 1 0 0 [expr {-($xmin+$xmax)/2.0}]
        scvcutview $model visibility $id 3
    }
    scvcutview $model rename $id "Cut-$serial"
    if {!$multiple} { foreach cut [scvcutview $model list] { scvcutview $model enable [dict get $cut id] [expr {[dict get $cut id] eq $id}] } }
    set selected $id
    refresh 1
}

proc ::CutViewManager::remove {} {
    variable model; variable selected
    if {$selected eq ""} { return }
    scvcutview $model remove $selected
    set selected ""
    refresh 1
}

proc ::CutViewManager::field {{selectionChanged 0}} {
    variable model; variable fields; variable fieldLabel; variable fieldLabels; variable component; variable w
    set index [lsearch -exact $fieldLabels $fieldLabel]
    if {$index <= 0} {
        scvcutview $model field none
        set component 0
        $w.results.component configure -state disabled -to 0
        return
    }
    set field [lindex $fields [expr {$index-1}]]
    set max [expr {[dict get $field components]-1}]
    $w.results.component configure -state normal -to $max
    if {![string is integer -strict $component] || $component < 0} { error "Result component must be a nonnegative integer" }
    if {$component > $max} {
        if {$selectionChanged} { set component 0 } else { error "Result component is outside the selected field" }
    }
    scvcutview $model field [dict get $field mapping] [dict get $field name] $component [dict get $field step]
}

proc ::CutViewManager::recoveryRefresh {} {
    variable model; variable lastRecovery; variable recoveryThreshold; variable recoveryBefore; variable recoveryRegion
    variable tensorMeasure; variable tensorLayout; variable tensorShear
    set tensor [scvcutview $model tensor]
    set tensorMeasure [dict get $tensor measure]; set tensorLayout [dict get $tensor layout]; set tensorShear [dict get $tensor shear]
    set current [scvcutview $model recovery]
    if {$current eq $lastRecovery} { return }
    set recoveryThreshold [dict get $current threshold]
    set recoveryBefore [dict get $current before]
    set recoveryRegion [expr {[llength [dict get $current regions]] ? "Current regions" : "All regions"}]
    set lastRecovery $current
}

proc ::CutViewManager::tensor {} {
    variable model; variable tensorMeasure; variable tensorLayout; variable tensorShear
    scvcutview $model tensor $tensorMeasure $tensorLayout $tensorShear
    refresh
}

proc ::CutViewManager::traceRefresh {} {
    variable w; variable model; variable selected; variable generation; variable lastTrace; variable lastTraceKey
    set key [list $model $generation $selected]
    if {$key ne $lastTraceKey} { set lastTrace {}; set lastTraceKey $key }
    foreach role {display physical} {
        set current [scvcutview $model trace $selected $role]
        foreach key {policy region regions} suffix {Policy Region Regions} {
            variable ${role}Trace$suffix
            set value [dict get $current $key]
            if {![dict exists $lastTrace $selected $role $key] || [dict get $lastTrace $selected $role $key] ne $value} {
                set ${role}Trace$suffix $value
            }
        }
        dict set lastTrace $selected $role $current
        $w.traces.${role}Policy configure -state readonly
    }
}
proc ::CutViewManager::trace {role} {
    variable model; variable selected; variable lastTrace
    variable ${role}TracePolicy; variable ${role}TraceRegion; variable ${role}TraceRegions
    set policy [set ${role}TracePolicy]
    set command [list scvcutview $model trace $selected $role $policy]
    if {$policy eq "region"} { lappend command [set ${role}TraceRegion] [set ${role}TraceRegions] }
    if {[catch {{*}$command} message]} {
        set lastTrace {}; refresh; error $message
    }
    refresh
}

proc ::CutViewManager::recovery {} {
    variable model; variable recoveryThreshold; variable recoveryBefore; variable recoveryRegion
    variable recoveryLabels; variable recoveryFields
    variable lastRecovery
    set command [list scvcutview $model recovery $recoveryThreshold $recoveryBefore]
    if {$recoveryRegion eq "All regions"} { lappend command none } elseif {$recoveryRegion eq "Current regions"} {
        lappend command keep
    } else {
        set index [lsearch -exact $recoveryLabels $recoveryRegion]
        if {$index < 2} { error "Select a region field" }
        set field [lindex $recoveryFields [expr {$index-2}]]
        lappend command [dict get $field name]
        if {[dict get $field step] >= 0} { lappend command [dict get $field step] }
    }
    {*}$command
    set lastRecovery [scvcutview $model recovery]
    set recoveryThreshold [dict get $lastRecovery threshold]
    set recoveryBefore [dict get $lastRecovery before]
    refresh
}

proc ::CutViewManager::cellmode {} {
    variable model; variable selected; variable cellmode
    if {$selected ne ""} { scvcutview $model cellmode $selected $cellmode }
    refresh
}

proc ::CutViewManager::save {} {
    variable model; variable w
    set path [tk_getSaveFile -parent $w -defaultextension .json -filetypes {{{Cut Definitions} {.json}}}]
    if {$path eq ""} { return }
    set stream [open $path w]
    try { puts $stream [scvcutview $model snapshot] } finally { ::close $stream }
}

proc ::CutViewManager::load {} {
    variable model; variable w; variable selected
    variable lastRecovery
    set path [tk_getOpenFile -parent $w -filetypes {{{Cut Definitions} {.json}}}]
    if {$path eq ""} { return }
    set stream [open $path r]
    try { set document [read $stream] } finally { ::close $stream }
    scvcutview $model restore $document
    set lastRecovery ""
    set selected ""
    refresh 1
}

proc ::CutViewManager::close {} {
    variable w; variable model; variable selected; variable timer
    variable sliderSnapshot; set sliderSnapshot ""
    variable lastRecovery; set lastRecovery ""
    if {$timer ne ""} { after cancel $timer; set timer "" }
    if {$model ne "" && $selected ne ""} { catch {scvcutview $model handle $selected 0} }
    if {[winfo exists $w]} { destroy $w }
    if {[winfo exists .scvcuttools]} { destroy .scvcuttools }
    set model ""; set selected ""
}

proc scvcutmanager {theModel} {
    package require Tk
    ::CutViewManager::close
    namespace eval ::CutViewManager { set lastPlane {}; set lastShape ""; set lastDragging 0; set selected ""; set status "" }
    set ::CutViewManager::model $theModel
    set ::CutViewManager::generation [dict get [scvcutview $theModel info] generation]
    set w $::CutViewManager::w
    set definitions [scvcutview $theModel list]
    if {[llength $definitions] == 0} { scvcutview $theModel defaults }
    toplevel $w
    wm title $w "Cut View Manager"
    wm minsize $w 560 560
    wm geometry $w +[expr {max(0,min(840,[winfo screenwidth $w]-800))}]+10
    ttk::style configure CutView.Treeview -rowheight [expr {[font metrics TkDefaultFont -linespace]+10}]
    ttk::frame $w.header -padding 8
    ttk::label $w.header.model -text "Model: $theModel"
    ttk::checkbutton $w.header.multiple -text "Allow Multiple Cuts" -variable ::CutViewManager::multiple -command {::CutViewManager::invoke ::CutViewManager::setMultiple}
    pack $w.header.model -side left
    pack $w.header.multiple -side right
    pack $w.header -fill x
    ttk::treeview $w.cuts -columns {show below on above} -height 2 -selectmode browse -style CutView.Treeview
    $w.cuts heading #0 -text Name; $w.cuts column #0 -width 220 -minwidth 100
    foreach column {show below on above} label {Show Below On Above} {
        set width [expr {[font measure TkHeadingFont $label]+24}]
        $w.cuts heading $column -text $label; $w.cuts column $column -width $width -minwidth $width -stretch 0 -anchor center
    }
    pack $w.cuts -fill both -expand 1 -padx 8
    bind $w.cuts <<TreeviewSelect>> {::CutViewManager::invoke ::CutViewManager::select}
    bind $w.cuts <ButtonRelease-1> {::CutViewManager::invoke ::CutViewManager::toggleCell %x %y}
    ttk::frame $w.actions -padding 8
    foreach widget {create copy delete save load} label {Create Copy Delete Save Load} command {
        {::CutViewManager::create} {::CutViewManager::create 1} {::CutViewManager::remove} {::CutViewManager::save} {::CutViewManager::load}
    } {
        ttk::button $w.actions.$widget -text $label -command [list ::CutViewManager::invoke {*}$command]
        pack $w.actions.$widget -side left -padx 2
    }
    pack $w.actions -fill x
    ttk::combobox $w.actions.shape -state readonly -values {Plane Cylinder Sphere Isosurface} -textvariable ::CutViewManager::createShape -width 10
    pack $w.actions.shape -side left -padx 4 -before $w.actions.create
    ttk::frame $w.controls -padding 8
    for {set row 0} {$row < 6} {incr row} {
        ttk::frame $w.controls.row$row
        pack $w.controls.row$row -fill x -pady 3
    }
    foreach label {Name Normal Position Motion} row {0 1 2 3} {
        ttk::label $w.controls.label$row -text $label -width 8
        pack $w.controls.label$row -in $w.controls.row$row -side left -padx 4
    }
    ttk::entry $w.controls.name -textvariable ::CutViewManager::name
    pack $w.controls.name -in $w.controls.row0 -fill x -expand 1
    foreach widget {nx ny nz} column {1 2 3} {
        ttk::entry $w.controls.$widget -width 10 -textvariable ::CutViewManager::$widget
        pack $w.controls.$widget -in $w.controls.row1 -side left -fill x -expand 1 -padx 2
    }
    ttk::frame $w.controls.rowcenter
    ttk::label $w.controls.centerlabel -text Center -width 8
    pack $w.controls.centerlabel -in $w.controls.rowcenter -side left -padx 4
    foreach widget {cx cy cz} {
        ttk::entry $w.controls.$widget -width 10 -textvariable ::CutViewManager::$widget
        pack $w.controls.$widget -in $w.controls.rowcenter -side left -fill x -expand 1 -padx 2
        bind $w.controls.$widget <Return> {::CutViewManager::invoke ::CutViewManager::apply}
    }
    ttk::combobox $w.controls.isofield -state readonly -textvariable ::CutViewManager::isoFieldLabel
    ttk::spinbox $w.controls.isocomponent -from 0 -to 0 -width 5 -textvariable ::CutViewManager::isoComponent -command {::CutViewManager::invoke ::CutViewManager::isoBinding}
    bind $w.controls.isofield <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::isoBinding}
    bind $w.controls.isocomponent <Return> {::CutViewManager::invoke ::CutViewManager::isoBinding}
    ttk::entry $w.controls.position -textvariable ::CutViewManager::positionText -width 12
    pack $w.controls.position -in $w.controls.row2 -side left -fill x -expand 1
    bind $w.controls.position <Return> {::CutViewManager::invoke ::CutViewManager::apply}
    ttk::button $w.controls.apply -text Apply -command {::CutViewManager::invoke ::CutViewManager::apply}
    pack $w.controls.apply -in $w.controls.row2 -side left -padx 4
    ttk::checkbutton $w.controls.show -text Show -variable ::CutViewManager::enabled -command {::CutViewManager::invoke ::CutViewManager::visibility}
    pack $w.controls.show -in $w.controls.row2 -side left -padx 4
    ttk::combobox $w.controls.motion -state readonly -values {Translate Rotate} -textvariable ::CutViewManager::motion -width 12
    ttk::combobox $w.controls.axis -state readonly -values {X Y Z} -textvariable ::CutViewManager::axis -width 4
    pack $w.controls.motion -in $w.controls.row3 -side left -fill x -expand 1
    pack $w.controls.axis -in $w.controls.row3 -side left -padx 4
    ttk::checkbutton $w.controls.gizmo -text Manipulator -variable ::CutViewManager::gizmo -command {::CutViewManager::invoke ::CutViewManager::refresh 1}
    pack $w.controls.gizmo -in $w.controls.row3 -side left -padx 4
    bind $w.controls.motion <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::motion}
    bind $w.controls.axis <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::motion}
    ::tk::scale $w.controls.translate -orient horizontal -showvalue 0 -resolution -1 -digits 17 -variable ::CutViewManager::position -command {::CutViewManager::invoke ::CutViewManager::translate}
    bind $w.controls.translate <ButtonPress-1> +{::CutViewManager::sliderBegin}
    bind $w.controls.translate <ButtonRelease-1> +{::CutViewManager::sliderEnd}
    bind $w.controls.translate <Escape> {::CutViewManager::invoke ::CutViewManager::sliderAbort}
    ::tk::scale $w.controls.rotate -orient horizontal -from -180 -to 180 -resolution 0.5 -showvalue 1 -variable ::CutViewManager::angle -command {::CutViewManager::invoke ::CutViewManager::rotate}
    ::CutViewManager::motion
    foreach widget {below on above} label {Below On Above} column {1 2 3} {
        ttk::checkbutton $w.controls.$widget -text $label -variable ::CutViewManager::$widget -command {::CutViewManager::invoke ::CutViewManager::visibility}
        pack $w.controls.$widget -in $w.controls.row5 -side left -padx 12 -pady 4
    }
    pack $w.controls -fill x
    ttk::separator $w.separator
    pack $w.separator -fill x -padx 8
    ttk::frame $w.results -padding 8
    ttk::frame $w.results.row0; ttk::frame $w.results.row1
    pack $w.results.row0 $w.results.row1 -fill x -pady 3
    ttk::label $w.results.label -text Result -width 8
    set ::CutViewManager::fields [scvcutview $theModel fields]
    set ::CutViewManager::fieldLabels [list "No Result"]
    set ::CutViewManager::fieldLabel "No Result"
    foreach field $::CutViewManager::fields {
        set label "[dict get $field name] ([dict get $field mapping])"
        if {[dict get $field step] >= 0} { append label " - step [dict get $field step]" }
        lappend ::CutViewManager::fieldLabels $label
        if {[dict get $field active]} {
            set ::CutViewManager::fieldLabel $label
            set ::CutViewManager::component [dict get $field component]
        }
    }
    $w.controls.isofield configure -values [lrange $::CutViewManager::fieldLabels 1 end]
    set ::CutViewManager::isoFieldLabel [lindex $::CutViewManager::fieldLabels 1]
    ttk::combobox $w.results.field -state readonly -values $::CutViewManager::fieldLabels -textvariable ::CutViewManager::fieldLabel
    ttk::spinbox $w.results.component -from 0 -to 0 -width 5 -textvariable ::CutViewManager::component -command {::CutViewManager::invoke ::CutViewManager::field}
    ttk::label $w.results.celllabel -text CellData -width 8
    ttk::combobox $w.results.cellmode -state readonly -values {constant projected} -textvariable ::CutViewManager::cellmode -width 12
    pack $w.results.label -in $w.results.row0 -side left -padx 4
    pack $w.results.component -in $w.results.row0 -side right -padx 4
    pack $w.results.field -in $w.results.row0 -side left -fill x -expand 1
    pack $w.results.celllabel -in $w.results.row1 -side left -padx 4
    pack $w.results.cellmode -in $w.results.row1 -side left
    bind $w.results.field <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::field 1}
    bind $w.results.component <Return> {::CutViewManager::invoke ::CutViewManager::field}
    bind $w.results.cellmode <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::cellmode}
    pack $w.results -fill x
    ttk::labelframe $w.recovery -text "Projected CellData Recovery" -padding 8
    ttk::frame $w.recovery.row0; ttk::frame $w.recovery.row1
    pack $w.recovery.row0 $w.recovery.row1 -fill x -pady 3
    ttk::label $w.recovery.thresholdLabel -text "Threshold (%)"
    ttk::entry $w.recovery.threshold -textvariable ::CutViewManager::recoveryThreshold -width 7
    ttk::checkbutton $w.recovery.before -text "Invariant before averaging" -variable ::CutViewManager::recoveryBefore -command {::CutViewManager::invoke ::CutViewManager::recovery}
    ttk::button $w.recovery.apply -text Apply -command {::CutViewManager::invoke ::CutViewManager::recovery}
    pack $w.recovery.thresholdLabel $w.recovery.threshold $w.recovery.before -in $w.recovery.row0 -side left -padx 3
    pack $w.recovery.apply -in $w.recovery.row0 -side right
    set ::CutViewManager::recoveryFields {}
    set ::CutViewManager::recoveryLabels [list "All regions" "Current regions"]
    foreach field $::CutViewManager::fields label [lrange $::CutViewManager::fieldLabels 1 end] {
        if {[dict get $field mapping] eq "cell" && [dict get $field components] == 1} {
            lappend ::CutViewManager::recoveryFields $field
            lappend ::CutViewManager::recoveryLabels $label
        }
    }
    ttk::label $w.recovery.regionLabel -text Region
    ttk::combobox $w.recovery.region -state readonly -values $::CutViewManager::recoveryLabels -textvariable ::CutViewManager::recoveryRegion
    pack $w.recovery.regionLabel -in $w.recovery.row1 -side left -padx 3
    pack $w.recovery.region -in $w.recovery.row1 -side left -fill x -expand 1
    bind $w.recovery.threshold <Return> {::CutViewManager::invoke ::CutViewManager::recovery}
    bind $w.recovery.region <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::recovery}
    pack $w.recovery -fill x -padx 8
    ttk::labelframe $w.tensor -text "Tensor Display (tensor fields only)" -padding 8
    ttk::frame $w.tensor.row0; ttk::frame $w.tensor.row1
    pack $w.tensor.row0 $w.tensor.row1 -fill x -pady 3
    ttk::label $w.tensor.measureLabel -text Measure
    ttk::combobox $w.tensor.measure -state readonly -width 14 -values {components mises pressure maxprincipal midprincipal minprincipal tresca} -textvariable ::CutViewManager::tensorMeasure
    ttk::label $w.tensor.shearLabel -text Shear
    ttk::combobox $w.tensor.shear -state readonly -width 12 -values {tensor engineering} -textvariable ::CutViewManager::tensorShear
    ttk::label $w.tensor.layoutLabel -text Layout
    ttk::combobox $w.tensor.layout -state readonly -values {xx-yy-zz-xy-xz-yz xx-yy-zz-xy-yz-xz row-major column-major} -textvariable ::CutViewManager::tensorLayout
    pack $w.tensor.measureLabel $w.tensor.measure $w.tensor.shearLabel $w.tensor.shear -in $w.tensor.row0 -side left -padx 3
    pack $w.tensor.measure -fill x -expand 1
    pack $w.tensor.layoutLabel -in $w.tensor.row1 -side left -padx 3
    pack $w.tensor.layout -in $w.tensor.row1 -side left -fill x -expand 1 -padx 3
    foreach control {measure layout shear} { bind $w.tensor.$control <<ComboboxSelected>> {::CutViewManager::invoke ::CutViewManager::tensor} }
    pack $w.tensor -fill x -padx 8
    ttk::labelframe $w.traces -text "Interface Source Selection" -padding 6
    foreach role {display physical} title {Display Physical} {
        ttk::frame $w.traces.${role}Row
        ttk::frame $w.traces.${role}Assignments
        ttk::label $w.traces.${role}Label -text $title -width 8
        ttk::combobox $w.traces.${role}Policy -state readonly -width 10 -values {lowest negative positive region} -textvariable ::CutViewManager::${role}TracePolicy
        ttk::label $w.traces.${role}RegionLabel -text "Region ID"
        ttk::entry $w.traces.${role}Region -width 20 -textvariable ::CutViewManager::${role}TraceRegion
        ttk::button $w.traces.${role}Apply -text Apply -command [list ::CutViewManager::invoke ::CutViewManager::trace $role]
        pack $w.traces.${role}Label $w.traces.${role}Policy $w.traces.${role}RegionLabel $w.traces.${role}Region -in $w.traces.${role}Row -side left -padx 3
        pack $w.traces.${role}Apply -in $w.traces.${role}Row -side right
        ttk::label $w.traces.${role}RegionsLabel -text "Element regions"
        ttk::entry $w.traces.${role}Regions -textvariable ::CutViewManager::${role}TraceRegions
        pack $w.traces.${role}RegionsLabel -in $w.traces.${role}Assignments -side left -padx 3
        pack $w.traces.${role}Regions -in $w.traces.${role}Assignments -side left -fill x -expand 1 -padx 3
        pack $w.traces.${role}Row $w.traces.${role}Assignments -fill x -pady 2
        foreach suffix {Region Regions} { bind $w.traces.$role$suffix <Return> [list ::CutViewManager::invoke ::CutViewManager::trace $role] }
    }
    pack $w.traces -fill x -padx 8
    ttk::button $w.tools -text "Viewer Tools" -command [list scvcuttools $theModel]
    pack $w.tools -fill x -padx 8 -pady 3
    ttk::label $w.status -textvariable ::CutViewManager::status -wraplength 530 -foreground #a02020
    pack $w.status -fill x -padx 8
    ttk::button $w.close -text Close -command ::CutViewManager::close
    pack $w.close -side right -padx 8 -pady 8
    wm protocol $w WM_DELETE_WINDOW ::CutViewManager::close
    ::CutViewManager::refresh 1
    ::CutViewManager::field 1
    ::CutViewManager::poll
    update idletasks
    wm minsize $w [winfo reqwidth $w] [expr {[winfo reqheight $w]-3*([font metrics TkDefaultFont -linespace]+10)}]
}
