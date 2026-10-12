namespace eval CutViewTime {
    variable model ""
    variable step 0
    variable period 0.1
    variable timestamps 1
    variable loop 1
    variable deformation "none"
    variable scale 0
    variable datum material
    variable configuration updated
    variable anchors {0 0 1 2}
    variable cut ""
    variable slicing range
    variable parameters {0 0.5 3}
    variable status ""
    variable timer ""
    variable last 0
}
proc CutViewTime::execute {args} {
    variable model; variable status
    if {[catch {uplevel #0 [list scvcutview $model {*}$args]} value]} { set status $value; return 0 }
    set status ""; return 1
}
proc CutViewTime::refresh {} {
    variable model; variable step; variable cut; variable deformation
    set info [scvcutview $model timeinfo]
    set step [dict get $info step]
    set fields {none}
    foreach field [scvcutview $model fields] {
        if {[dict get $field kind] eq "vector" && [dict get $field mapping] eq "point"} { lappend fields [dict get $field name] }
    }
    .scvcuttime.deform.field configure -values $fields
    if {$deformation ni $fields} { set deformation none }
    set cuts {}; foreach definition [scvcutview $model list] { lappend cuts [dict get $definition id] }
    .scvcuttime.material.cut configure -values $cuts
    if {$cut ni $cuts} { set cut [lindex $cuts 0] }
    .scvcuttime.step.value configure -to [expr {max(0,[dict get $info count]-1)}]
}
proc CutViewTime::applyStep {} {
    variable step
    if {[execute step $step]} { refresh; if {[winfo exists .scvcutmanager]} { CutViewManager::refresh } }
}
proc CutViewTime::deform {} { variable deformation; variable scale; execute timeddeform $deformation $scale }
proc CutViewTime::material {} {
    variable datum; variable configuration; variable anchors; variable cut
    if {[execute materialframe $datum $configuration {*}$anchors]} { execute follow $cut $datum }
}
proc CutViewTime::slices {} { variable cut; variable slicing; variable parameters; execute slicing $cut $slicing {*}$parameters }
proc CutViewTime::pause {} {
    variable timer
    if {$timer ne ""} { after cancel $timer; set timer "" }
    execute pause
}
proc CutViewTime::play {restart} {
    variable period; variable loop; variable timestamps; variable last
    pause
    if {[execute play $period $loop $restart $timestamps]} { set last [clock milliseconds]; tick }
}
proc CutViewTime::tick {} {
    variable model; variable timer; variable last
    if {![winfo exists .scvcuttime]} { return }
    set now [clock milliseconds]; set elapsed [expr {($now-$last)/1000.0}]; set last $now
    if {![execute tick $elapsed]} { return }
    refresh
    if {[dict get [scvcutview $model timeinfo] playing]} { set timer [after 16 CutViewTime::tick] }
}
proc CutViewTime::close {} { pause; catch {destroy .scvcuttime} }
proc scvcuttime {name} {
    if {[winfo exists .scvcuttime]} { CutViewTime::close }
    set ::CutViewTime::model $name
    toplevel .scvcuttime; wm title .scvcuttime "Time, material frames and slicing"
    wm protocol .scvcuttime WM_DELETE_WINDOW CutViewTime::close
    foreach section {step animation deform material slicing} { ttk::labelframe .scvcuttime.$section -text [string totitle $section]; pack .scvcuttime.$section -fill x -padx 6 -pady 3 }
    ttk::spinbox .scvcuttime.step.value -from 0 -to 0 -textvariable ::CutViewTime::step -width 7
    ttk::button .scvcuttime.step.apply -text "Switch step" -command CutViewTime::applyStep
    pack .scvcuttime.step.value .scvcuttime.step.apply -side left
    ttk::entry .scvcuttime.animation.period -textvariable ::CutViewTime::period -width 8
    ttk::checkbutton .scvcuttime.animation.timestamps -text "Source timestamps (speed)" -variable ::CutViewTime::timestamps
    ttk::checkbutton .scvcuttime.animation.loop -text Loop -variable ::CutViewTime::loop
    ttk::button .scvcuttime.animation.play -text Play -command {CutViewTime::play 0}
    ttk::button .scvcuttime.animation.pause -text Pause -command CutViewTime::pause
    ttk::button .scvcuttime.animation.restart -text Restart -command {CutViewTime::play 1}
    pack {*}[winfo children .scvcuttime.animation] -side left
    ttk::combobox .scvcuttime.deform.field -state readonly -textvariable ::CutViewTime::deformation -width 20
    ttk::entry .scvcuttime.deform.scale -textvariable ::CutViewTime::scale -width 8
    ttk::button .scvcuttime.deform.apply -text Apply -command CutViewTime::deform
    pack {*}[winfo children .scvcuttime.deform] -side left
    ttk::entry .scvcuttime.material.datum -textvariable ::CutViewTime::datum -width 10
    ttk::combobox .scvcuttime.material.configuration -state readonly -values {reference updated} -textvariable ::CutViewTime::configuration -width 10
    ttk::entry .scvcuttime.material.anchors -textvariable ::CutViewTime::anchors -width 35
    ttk::combobox .scvcuttime.material.cut -state readonly -textvariable ::CutViewTime::cut -width 10
    ttk::button .scvcuttime.material.apply -text "Bind frame (step n0 n1 n2)" -command CutViewTime::material
    pack {*}[winfo children .scvcuttime.material] -side left
    ttk::combobox .scvcuttime.slicing.mode -state readonly -values {regular range rotational path} -textvariable ::CutViewTime::slicing -width 12
    ttk::entry .scvcuttime.slicing.parameters -textvariable ::CutViewTime::parameters -width 48
    ttk::button .scvcuttime.slicing.apply -text "Generate slices" -command CutViewTime::slices
    pack {*}[winfo children .scvcuttime.slicing] -side left
    ttk::label .scvcuttime.status -textvariable ::CutViewTime::status -wraplength 600; pack .scvcuttime.status -fill x
    ttk::button .scvcuttime.close -text Close -command CutViewTime::close; pack .scvcuttime.close -pady 6
    CutViewTime::refresh
}
