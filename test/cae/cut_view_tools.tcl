# Native Viewer tools share scvcutview validation with the main manager and mouse aids.
namespace eval ::CutViewTools {
    variable w .scvcuttools
    variable model ""
    variable datum {0 0 0 1 0 0 0 1 0 0 0 1}
    variable local {1 0 0 -0.25}
    variable axes 0
    variable portion 1
    variable opacity 0
    variable width 2
    variable edges 0
    variable styleOwner "Selected cut"
    variable auto 1
    variable minimum 0
    variable maximum 1
    variable legend 0
    variable intervals 12
    variable vector ""
    variable orientation 0
    variable scale 1
    variable sampling 1
    variable index 0
    variable node 1
    variable text "Section result"
    variable cad ""
    variable sync 0
    variable clip 0
    variable pick 0
    variable status ""
    variable vectorBindings {}
}
proc ::CutViewTools::cut {} {
    return $::CutViewManager::selected
}
proc ::CutViewTools::call {operation} {
    variable model; variable status
    if {[catch {::CutViewTools::$operation} message]} { set status $message } else { set status $message }
}
proc ::CutViewTools::setdatum {} {
    variable model; variable datum
    scvcutview $model datum tools {*}$datum
}
proc ::CutViewTools::edit {} {
    variable model; variable local
    scvcutview $model localmove [cut] tools {*}$local
    ::CutViewManager::refresh
}
proc ::CutViewTools::reference {} {
    variable model; variable axes
    scvcutview $model axes tools $axes 0.5
}
proc ::CutViewTools::style {} {
    variable model; variable portion; variable opacity; variable width; variable edges
    set owner [expr {$::CutViewTools::styleOwner eq "Common defaults" ? "defaults" : [cut]}]
    scvcutview $model cutstyle $owner $portion $opacity $width [expr {$edges ? 2 : 1}]
}
proc ::CutViewTools::clearstyle {} { scvcutview $::CutViewTools::model clearstyle [cut] $::CutViewTools::portion }
proc ::CutViewTools::contour {} {
    variable model; variable auto; variable minimum; variable maximum; variable legend; variable intervals
    scvcutview $model contour $auto $minimum $maximum $legend $intervals
}
proc ::CutViewTools::vectors {} {
    variable model; variable vector; variable orientation; variable scale; variable sampling
    if {$vector eq "" || $vector eq "No vectors"} { scvcutview $model vectors none; return }
    if {![dict exists $::CutViewTools::vectorBindings $vector]} { error "Vector field not found" }
    scvcutview $model vectors {*}[dict get $::CutViewTools::vectorBindings $vector] $scale $sampling $orientation 1
}
proc ::CutViewTools::probe {} {
    variable model; variable portion; variable index; variable node
    return [scvcutview $model probe [cut] $portion $index $node]
}
proc ::CutViewTools::annotate {} {
    variable model; variable portion; variable index; variable node; variable text
    scvcutview $model annotation [cut] $portion $index $node tools $text
}
proc ::CutViewTools::annotationsclear {} { scvcutview $::CutViewTools::model annotationsclear }
proc ::CutViewTools::cadsection {} {
    variable model; variable cad; variable sync
    uplevel #0 [list scvcutview $model cadsection $cad [cut] $sync]
}
proc ::CutViewTools::cadclear {} { scvcutview $::CutViewTools::model cadclear }
proc ::CutViewTools::clipping {} {
    variable model; variable clip
    foreach definition [scvcutview $model list] { if {[dict get $definition id] eq [cut]} {
        if {[dict get $definition shape] ne "plane"} { error "Global clipping requires a plane" }
        scvcutview $model globalclip {*}[dict get $definition plane] $clip
        return
    } }
}
proc ::CutViewTools::picking {} { scvcutview $::CutViewTools::model pickmode $::CutViewTools::pick }
proc ::CutViewTools::refreshVectors {} {
    variable model; variable vector; variable vectorBindings; variable w
    set previous {}
    if {[info exists vectorBindings] && [dict exists $vectorBindings $vector]} { set previous [lrange [dict get $vectorBindings $vector] 0 1] }
    set vectorBindings {}; set labels [list "No vectors"]; set vector "No vectors"
    foreach field [scvcutview $model fields] {
        if {[dict get $field kind] ne "vector" || [dict get $field components] != 4} { continue }
        set binding [list [dict get $field mapping] [dict get $field name] [dict get $field step]]
        set label "[dict get $field name] ([dict get $field mapping]) step [dict get $field step]"
        dict set vectorBindings $label $binding; lappend labels $label
        if {[lrange $binding 0 1] eq $previous} { set vector $label }
    }
    $w.vector.field configure -values $labels
}

proc scvcuttools {model} {
    set w $::CutViewTools::w
    if {[winfo exists $w]} { destroy $w }
    set ::CutViewTools::model $model
    scvcutview $model datum tools {*}$::CutViewTools::datum
    toplevel $w; wm title $w "Native Cut Viewer Tools"
    foreach {row title} {datum Datum local {Local shape values} style {Cut portion style} contour {Contour range} vector {Vector / orientation} probe {Original FE probe} annotation Annotation cad {CAD section / clipping}} {
        ttk::labelframe $w.$row -text $title -padding 5
        pack $w.$row -fill x -padx 6 -pady 2
    }
    ttk::entry $w.datum.values -textvariable ::CutViewTools::datum -width 45
    ttk::button $w.datum.apply -text Apply -command {::CutViewTools::call setdatum}
    ttk::checkbutton $w.datum.axes -text Axes -variable ::CutViewTools::axes -command {::CutViewTools::call reference}
    pack $w.datum.values $w.datum.apply $w.datum.axes -side left -padx 3
    ttk::entry $w.local.values -textvariable ::CutViewTools::local -width 45
    ttk::button $w.local.apply -text Apply -command {::CutViewTools::call edit}
    pack $w.local.values $w.local.apply -side left -padx 3
    ttk::combobox $w.style.portion -state readonly -values {0 1 2} -width 3 -textvariable ::CutViewTools::portion
    ttk::combobox $w.style.owner -state readonly -values {"Selected cut" "Common defaults"} -width 15 -textvariable ::CutViewTools::styleOwner
    ttk::entry $w.style.opacity -width 7 -textvariable ::CutViewTools::opacity
    ttk::entry $w.style.width -width 5 -textvariable ::CutViewTools::width
    ttk::checkbutton $w.style.edges -text Edges -variable ::CutViewTools::edges -command {::CutViewTools::call style}
    ttk::button $w.style.apply -text Apply -command {::CutViewTools::call style}
    ttk::button $w.style.reset -text Reset -command {::CutViewTools::call clearstyle}
    pack $w.style.owner $w.style.portion $w.style.opacity $w.style.width $w.style.edges $w.style.apply $w.style.reset -side left -padx 3
    ttk::checkbutton $w.contour.auto -text Auto -variable ::CutViewTools::auto -command {::CutViewTools::call contour}
    ttk::entry $w.contour.minimum -width 8 -textvariable ::CutViewTools::minimum
    ttk::entry $w.contour.maximum -width 8 -textvariable ::CutViewTools::maximum
    ttk::spinbox $w.contour.intervals -from 2 -to 256 -width 5 -textvariable ::CutViewTools::intervals
    ttk::checkbutton $w.contour.legend -text Legend -variable ::CutViewTools::legend -command {::CutViewTools::call contour}
    ttk::button $w.contour.apply -text Apply -command {::CutViewTools::call contour}
    pack $w.contour.auto $w.contour.minimum $w.contour.maximum $w.contour.intervals $w.contour.legend $w.contour.apply -side left -padx 3
    set ::CutViewTools::vector "No vectors"
    ttk::combobox $w.vector.field -state readonly -width 20 -textvariable ::CutViewTools::vector
    ::CutViewTools::refreshVectors
    bind $w.vector.field <<ComboboxSelected>> {::CutViewTools::call vectors}
    ttk::entry $w.vector.scale -width 6 -textvariable ::CutViewTools::scale
    ttk::entry $w.vector.sampling -width 6 -textvariable ::CutViewTools::sampling
    ttk::checkbutton $w.vector.orientation -text Orientations -variable ::CutViewTools::orientation -command {::CutViewTools::call vectors}
    ttk::button $w.vector.apply -text Apply -command {::CutViewTools::call vectors}
    pack $w.vector.field $w.vector.scale $w.vector.sampling $w.vector.orientation $w.vector.apply -side left -padx 3
    ttk::entry $w.probe.index -width 8 -textvariable ::CutViewTools::index
    ttk::checkbutton $w.probe.node -text Node -variable ::CutViewTools::node
    ttk::combobox $w.probe.pick -state readonly -values {0 1 2} -width 3 -textvariable ::CutViewTools::pick
    bind $w.probe.pick <<ComboboxSelected>> {::CutViewTools::call picking}
    ttk::button $w.probe.apply -text Probe -command {::CutViewTools::call probe}
    pack $w.probe.index $w.probe.node $w.probe.pick $w.probe.apply -side left -padx 3
    ttk::entry $w.annotation.text -width 35 -textvariable ::CutViewTools::text
    ttk::button $w.annotation.apply -text Annotate -command {::CutViewTools::call annotate}
    ttk::button $w.annotation.clear -text Clear -command {::CutViewTools::call annotationsclear}
    pack $w.annotation.text $w.annotation.apply $w.annotation.clear -side left -padx 3
    ttk::entry $w.cad.shape -width 20 -textvariable ::CutViewTools::cad
    ttk::checkbutton $w.cad.sync -text Sync -variable ::CutViewTools::sync
    ttk::checkbutton $w.cad.clip -text Clip -variable ::CutViewTools::clip -command {::CutViewTools::call clipping}
    ttk::button $w.cad.apply -text Section -command {::CutViewTools::call cadsection}
    ttk::button $w.cad.clear -text Clear -command {::CutViewTools::call cadclear}
    pack $w.cad.shape $w.cad.sync $w.cad.clip $w.cad.apply $w.cad.clear -side left -padx 3
    ttk::label $w.status -textvariable ::CutViewTools::status -wraplength 540
    ttk::button $w.time -text "Time and slicing" -command [list scvcuttime $model]
    ttk::button $w.physics -text "Physical resultants and automatic cuts" -command [list scvcutphysics $model]
    ttk::button $w.cache -text "Cache policy" -command [list scvcutcache $model]
    ttk::button $w.close -text Close -command [list destroy $w]
    pack $w.status $w.time $w.physics $w.cache $w.close -fill x -padx 6 -pady 4
}
