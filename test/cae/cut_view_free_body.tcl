# Manager free-body options. The cut is fixed when this window opens.
namespace eval ::CutViewFreeBody {
    variable w .scvcutfreebody
    variable model ""; variable cut ""; variable id ""; variable status ""
}
proc ::CutViewFreeBody::quote {value} {
    set slash [format %c 92]
    set escaped [string map [list $slash "$slash$slash" [format %c 34] "$slash[format %c 34]" [format %c 10] "${slash}n" [format %c 13] "${slash}r" [format %c 9] "${slash}t"] $value]
    return [format {"%s"} $escaped]
}
proc ::CutViewFreeBody::call {action} {
    variable status
    if {[catch {uplevel #0 [list ::CutViewFreeBody::$action]} result]} { set status $result } else { set status $result }
}
proc ::CutViewFreeBody::apply {} {
    variable model; variable cut; variable id
    foreach key {field mapping components heat beam overlay scale group elements absolute units origin frame outputFrame sections normals enabled distributed componentVectors resolution tangent datum sumPoint normalSign domainField domainSource domainValue domainMask domainComponent} { variable $key }
    if {$field eq ""} { error "Select an explicit physical field" }
    set step [dict get [scvcutview $model timeinfo] step]
    set pairs {}
    foreach {key value} [list cut $cut group $group datum $datum sumPoint $sumPoint] { lappend pairs "[quote $key]:[quote $value]" }
    lappend pairs [format {"field":{"name":%s,"nodal":%s,"component":0,"step":%d}} [quote $field] [expr {$mapping eq "point" ? "true":"false"}] $step]
    foreach key {heat beam overlay enabled distributed componentVectors absolute} { lappend pairs "[quote $key]:[expr {[set $key] ? {true}:{false}}]" }
    foreach {key value count} [list components $components 9 elements $elements [llength $elements] momentOrigin $origin 3 fieldFrame $frame 12 outputFrame $outputFrame 12 tangentY $tangent 3] {
        lappend pairs "[quote $key]:\[[::CutViewPhysics::numbers $value $count]\]"
    }
    foreach {key value} [list arrowScale $scale normalSign $normalSign resolution $resolution] { lappend pairs "[quote $key]:[::CutViewPhysics::numbers $value 1]" }
    lappend pairs "\"sections\":$sections" "\"normals\":$normals"
    lappend pairs [format {"domainField":{"name":%s,"nodal":false,"component":%s,"step":%d}} [quote $domainField] [::CutViewPhysics::numbers $domainComponent 1] $step]
    lappend pairs "\"domainSource\":[quote $domainSource]" "\"domainValue\":[::CutViewPhysics::numbers $domainValue 1]" "\"domainMask\":[expr {$domainMask ? {true}:{false}}]"
    set us {}; set i 0
    ::CutViewPhysics::numbers $units 5
    foreach key {length stress flux force moment} { lappend us "[quote $key]:\[[quote "input $key"],[lindex $units $i]\]"; incr i }
    lappend pairs "\"units\":\{[join $us ,]\}"
    scvcutview $model physics $id "\{[join $pairs ,]\}"
    if {[llength [info commands ::CutViewManager::refresh]]} { ::CutViewManager::refresh }
    if {$enabled} { return [scvcutview $model physicsreport $id] }
    return "Configuration saved; free body disabled"
}
proc ::CutViewFreeBody::export {} {
    variable output; variable model; variable id
    if {$output eq ""} { error "Enter a CSV path" }
    set csv [scvcutview $model physicscsv $id]
    set f [open $output w]; try { puts -nonewline $f $csv } finally { close $f }
    return "Saved $output"
}
proc ::CutViewFreeBody::capture {} {
    variable model; variable id; variable sourceObject; variable sourcePresentation; variable cut
    scvcutview $model physicsdisplaygroup $id $sourceObject $sourcePresentation
    scvcutfreebody $model $cut
    return "Captured source display group; recapture after source visibility changes"
}
proc ::CutViewFreeBody::captureDefault {} {
    variable model; variable id
    scvcutview $model physicsdefault $id
    return "Captured common template; cut overrides remain independent"
}
proc ::CutViewFreeBody::useDefault {} {
    variable model; variable cut; variable id
    scvcutview $model physicsapplydefault $cut $id 0
    scvcutfreebody $model $cut
    return "Validated common template for this cut; enable when ready"
}
proc scvcutfreebody {theModel {theCut ""}} {
    package require Tk
    if {$theCut eq ""} { set theCut $::CutViewManager::selected }
    if {$theCut eq ""} { error "Select a cut first" }
    set found 0
    foreach d [scvcutview $theModel list] { if {[dict get $d id] eq $theCut} { set found 1 } }
    if {!$found} { error "Unknown cut $theCut" }
    set w $::CutViewFreeBody::w; catch {destroy $w}
    set ::CutViewFreeBody::model $theModel; set ::CutViewFreeBody::cut $theCut
    set ::CutViewFreeBody::id "viewcut/$theCut"; set ::CutViewFreeBody::status ""
    if {![dict get [scvcutview $theModel physicsstate $theCut] configured] && [scvcutview $theModel physicsdefault]} {
        scvcutview $theModel physicsapplydefault $theCut $::CutViewFreeBody::id 0
    }
    foreach {key value} {
        field "" mapping point components {0 3 4 3 1 5 4 5 2} heat 0 beam 0 overlay 1 scale 0.1
        group all elements {} absolute 0 units {1 1 1 1 1} origin {0 0 0}
        frame {0 0 0 1 0 0 0 1 0 0 0 1} outputFrame {0 0 0 1 0 0 0 1 0 0 0 1}
        sections {[]} normals {[]} enabled 1 distributed 0 componentVectors 0 resolution 0 tangent {0 1 0}
        datum global sumPoint centroid normalSign 1 output ""
        domainField "" domainSource explicit domainValue 0 domainMask 0 domainComponent 0 sourceObject "" sourcePresentation ""
    } { set ::CutViewFreeBody::$key $value }
    if {![catch {scvcutview $theModel physicsconfig $::CutViewFreeBody::id} config]} {
        foreach key {field mapping components heat beam overlay scale group elements absolute units origin frame outputFrame sections normals enabled distributed componentVectors resolution tangent datum sumPoint normalSign domainField domainSource domainValue domainMask domainComponent} {
            set ::CutViewFreeBody::$key [dict get $config $key]
        }
    }
    set fields {}
    foreach f [scvcutview $theModel fields] { lappend fields [dict get $f name] }
    toplevel $w; wm title $w "Free Body: $theModel / $theCut"
    ttk::label $w.help -text "Physical FE values, independent of contour. Tensor map: row-major indices (9).\nUnits: positive factors to SI. Shell/layer/beam sections require explicit JSON.\nResolution: 0=output frame, 1=normal/tangent, 2=datum. Empty named domains reject." -justify left
    pack $w.help -fill x -padx 8 -pady 4
    foreach {row title} {binding {Physical field} domain {Integration domain} set {Element set: cell labels or group bits} display {Capture actual display group (same source mesh)} sum {Summation and orientation} axes {Component resolution} input {Input field frame (origin X Y Z)} output {Output frame (origin X Y Z)} units {SI factors: length stress flux force moment} section {Section / layer and explicit normal mappings} arrows {Vectors} export {CSV report}} {
        ttk::labelframe $w.$row -text $title -padding 4; pack $w.$row -fill x -padx 8 -pady 2
    }
    foreach {row key values width} [list binding field [lsort -unique $fields] 18 binding mapping {point cell} 7 sum sumPoint {centroid user} 10 sum normalSign {1 -1} 4 axes resolution {0 1 2} 4] {
        ttk::combobox $w.$row.$key -state readonly -values $values -textvariable ::CutViewFreeBody::$key -width $width
        pack $w.$row.$key -side left -padx 2
    }
    foreach {row key title width} {
        binding components {Component map} 24 domain group {Name (all=whole model)} 18 domain elements {Element IDs} 24
        sum origin {User point} 20 axes tangent {Tangent Y} 14 axes datum CSYS 12
        set domainField {Cell field} 18 set domainComponent Component 4 set domainValue Value 6
        display sourceObject Object 16 display sourcePresentation Presentation 16
        input frame {Field frame} 55 output outputFrame {Output frame} 55 units units Factors 30
        section sections Sections 45 section normals Normals 20 arrows scale Scale 8 export output File 50
    } {
        ttk::label $w.$row.${key}Label -text $title
        ttk::entry $w.$row.$key -textvariable ::CutViewFreeBody::$key -width $width
        pack $w.$row.${key}Label $w.$row.$key -side left -padx 2
    }
    foreach {row key title} {binding heat {Heat flow} binding beam Beam domain absolute {Absolute contributions} set domainMask {Group bit} arrows enabled Enabled arrows overlay Arrows arrows distributed {Distributed (max 1024)} arrows componentVectors Components} {
        ttk::checkbutton $w.$row.$key -text $title -variable ::CutViewFreeBody::$key; pack $w.$row.$key -side left -padx 2
    }
    ttk::button $w.export.save -text Save -command {::CutViewFreeBody::call export}; pack $w.export.save -side left
    ttk::button $w.display.capture -text Capture -command {::CutViewFreeBody::call capture}; pack $w.display.capture -side left
    ttk::frame $w.actions; pack $w.actions -fill x -padx 8 -pady 4
    ttk::button $w.actions.apply -text Apply -command {::CutViewFreeBody::call apply}
    ttk::button $w.actions.close -text Close -command [list destroy $w]
    ttk::button $w.actions.captureDefault -text "Capture common template" -command {::CutViewFreeBody::call captureDefault}
    ttk::button $w.actions.useDefault -text "Use common template" -command {::CutViewFreeBody::call useDefault}
    pack $w.actions.apply $w.actions.captureDefault $w.actions.useDefault $w.actions.close -side left -padx 4
    ttk::label $w.status -textvariable ::CutViewFreeBody::status -wraplength 850; pack $w.status -fill x -padx 8 -pady 4
}
