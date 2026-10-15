namespace eval ::CutViewPhysics {
    variable w .scvcutphysics; variable model ""; variable id resultant; variable cut p
    variable field stress; variable mapping point; variable components {0 3 4 3 1 5 4 5 2}
    variable heat 0; variable beam 0; variable overlay 1; variable scale 0.1
    variable group all; variable elements {}; variable absolute 0
    variable units {1 1 1 1 1}; variable origin {0 0 0}
    variable frame {0 0 0 1 0 0 0 1 0 0 0 1}; variable sections {[]}
    variable autoid automatic; variable kind crack; variable autofield heat_flux
    variable component 1; variable threshold 2; variable sign 1; variable range {0 5}
    variable mask 2; variable output ""; variable status ""
}
proc ::CutViewPhysics::numbers {values count} {
    if {[llength $values] != $count} { error "Expected $count numeric values" }
    foreach v $values { if {![string is double -strict $v] || $v != $v || abs($v) == Inf} { error "Expected finite numbers" } }
    return [join $values ,]
}
proc ::CutViewPhysics::call {action} {
    variable status
    if {[catch {uplevel #0 [list ::CutViewPhysics::$action]} result]} { set status $result } else { set status $result }
}
proc ::CutViewPhysics::apply {} {
    variable model; variable id; variable cut; variable field; variable mapping; variable components
    variable heat; variable beam; variable overlay; variable scale; variable group; variable elements
    variable absolute; variable units; variable origin; variable frame; variable sections
    if {$cut eq "" || $field eq "" || [regexp {["\\]} "$cut$field$group"]} { error "Invalid cut/field/group" }
    set step [dict get [scvcutview $model timeinfo] step]
    lassign [split [numbers $units 5] ,] l s q f m
    set cs [numbers $components 9]; set fs [numbers $frame 12]; set os [numbers $origin 3]
    set es [numbers $elements [llength $elements]]
    set json [format {{"cut":"%s","field":{"name":"%s","nodal":%s,"component":0,"step":%d},"components":[%s],"heat":%s,"beam":%s,"overlay":%s,"arrowScale":%s,"group":"%s","elements":[%s],"absolute":%s,"fieldFrame":[%s],"outputFrame":[%s],"momentOrigin":[%s],"sections":%s,"units":{"length":["input length",%s],"stress":["input stress",%s],"flux":["input flux",%s],"force":["input force",%s],"moment":["input moment",%s]}}} $cut $field [expr {$mapping eq "point" ? "true" : "false"}] $step $cs [expr {$heat ? "true":"false"}] [expr {$beam ? "true":"false"}] [expr {$overlay ? "true":"false"}] [numbers $scale 1] $group $es [expr {$absolute?"true":"false"}] $fs $fs $os $sections $l $s $q $f $m]
    scvcutview $model physics $id $json
    return [scvcutview $model physicsreport $id]
}
proc ::CutViewPhysics::report {} { return [scvcutview $::CutViewPhysics::model physicsreport $::CutViewPhysics::id] }
proc ::CutViewPhysics::remove {} { scvcutview $::CutViewPhysics::model physicsremove $::CutViewPhysics::id }
proc ::CutViewPhysics::export {} {
    variable output; variable model; variable id
    if {$output eq ""} { error "Choose a report file" }
    set csv [scvcutview $model physicscsv $id]
    set f [open $output w]; try { puts -nonewline $f $csv } finally { close $f }
}
proc ::CutViewPhysics::automatic {} {
    variable model; variable autoid; variable kind; variable autofield; variable component
    variable threshold; variable sign; variable range; variable mask
    lassign $range minimum maximum
    scvcutview $model automatic $autoid $kind $autofield $component $threshold $sign $minimum $maximum $mask
}
proc scvcutphysics {model} {
    set w $::CutViewPhysics::w; if {[winfo exists $w]} { destroy $w }; set ::CutViewPhysics::model $model
    toplevel $w; wm title $w "Physical resultants and automatic cuts"
    set cuts {}; set fields {}
    foreach d [scvcutview $model list] { lappend cuts [dict get $d id] }
    foreach f [scvcutview $model fields] { lappend fields [dict get $f name] }
    foreach {row title} {binding {Cut and physical field} options {Group and summation} units {SI factors: length stress flux force moment} frame {Origin / field and output frame} section {Explicit section mappings} automatic {Distance / density mapping} export {CSV report}} {
        ttk::labelframe $w.$row -text $title -padding 4; pack $w.$row -fill x -padx 5 -pady 2
    }
    foreach {row name variable values width} {
        binding cut cut cuts 8 binding field field fields 16 binding mapping mapping {point cell} 6
        automatic kind kind {crack density} 8 automatic field autofield fields 16
    } {
        if {$values eq "cuts"} { set values $cuts }; if {$values eq "fields"} { set values [lsort -unique $fields] }
        ttk::combobox $w.$row.$name -state readonly -values $values -textvariable ::CutViewPhysics::$variable -width $width
        pack $w.$row.$name -side left -padx 2
    }
    foreach {row name variable width} {
        binding id id 12 binding components components 24 binding scale scale 5
        options group group 12 options elements elements 20 units factors units 35
        frame origin origin 15 frame axes frame 40 section mappings sections 60
        automatic id autoid 12 automatic component component 4 automatic threshold threshold 6
        automatic sign sign 4 automatic range range 10 automatic mask mask 4 export path output 50
    } { ttk::entry $w.$row.$name -textvariable ::CutViewPhysics::$variable -width $width; pack $w.$row.$name -side left -padx 2 }
    foreach {row name variable title} {binding heat heat Heat binding beam beam Beam binding overlay overlay Arrows options absolute absolute Absolute} {
        ttk::checkbutton $w.$row.$name -text $title -variable ::CutViewPhysics::$variable; pack $w.$row.$name -side left -padx 2
    }
    foreach {row name action title} {binding apply apply Compute options report report Report options remove remove Remove automatic apply automatic Create export save export Save} {
        ttk::button $w.$row.$name -text $title -command [list ::CutViewPhysics::call $action]; pack $w.$row.$name -side left -padx 2
    }
    ttk::label $w.status -textvariable ::CutViewPhysics::status -wraplength 850; pack $w.status -fill x
    ttk::button $w.close -text Close -command [list destroy $w]; pack $w.close -fill x
}
