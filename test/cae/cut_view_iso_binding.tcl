namespace eval ::CutViewIsoBinding { variable w .scvcutisobinding; variable model ""; variable cut ""; variable status "" }
proc ::CutViewIsoBinding::apply {} {
    variable model; variable cut; variable measure; variable layout; variable shear; variable override
    variable threshold; variable before; variable regions; variable status
    set index [lsearch -exact {components mises pressure maxprincipal midprincipal minprincipal tresca} $measure]
    set li [lsearch -exact {xx-yy-zz-xy-xz-yz xx-yy-zz-xy-yz-xz row-major column-major} $layout]
    if {[catch {scvcutview $model isobinding $cut $index $li $shear $override $threshold $before {*}$regions} result]} { set status $result; return }
    set status "Binding saved; contour choices remain independent"
    ::CutViewManager::refresh 1
}
proc scvcutisobinding {model {cut ""}} {
    if {$cut eq ""} { set cut $::CutViewManager::selected }
    set config [scvcutview $model isobinding $cut]
    set w $::CutViewIsoBinding::w; catch {destroy $w}
    set ::CutViewIsoBinding::model $model; set ::CutViewIsoBinding::cut $cut; set ::CutViewIsoBinding::status ""
    foreach key {shear override threshold before regions} { set ::CutViewIsoBinding::$key [dict get $config $key] }
    set ::CutViewIsoBinding::measure [lindex {components mises pressure maxprincipal midprincipal minprincipal tresca} [dict get $config measure]]
    set ::CutViewIsoBinding::layout [lindex {xx-yy-zz-xy-xz-yz xx-yy-zz-xy-yz-xz row-major column-major} [dict get $config layout]]
    toplevel $w; wm title $w "Isosurface binding: $model / $cut"
    ttk::label $w.help -text "Bound field/component/frame: edit explicitly with Cut Field / Apply.\nThese tensor and averaging options are saved independently of the contour.\n100% override replaces the saved threshold; regions remain independent." -justify left
    pack $w.help -fill x -padx 8 -pady 5
    foreach {row title} {tensor {Derived scalar} average {Averaging snapshot} regions {Element regions (empty = one region)}} { ttk::labelframe $w.$row -text $title -padding 6; pack $w.$row -fill x -padx 8 -pady 3 }
    foreach {key values} {measure {components mises pressure maxprincipal midprincipal minprincipal tresca} layout {xx-yy-zz-xy-xz-yz xx-yy-zz-xy-yz-xz row-major column-major}} {
        ttk::combobox $w.tensor.$key -state readonly -values $values -textvariable ::CutViewIsoBinding::$key; pack $w.tensor.$key -side left -padx 3
    }
    foreach {row key title} {tensor shear {Engineering shear} average override {Override to 100%} average before {Invariant before averaging}} {
        ttk::checkbutton $w.$row.$key -text $title -variable ::CutViewIsoBinding::$key; pack $w.$row.$key -side left -padx 3
    }
    ttk::entry $w.average.threshold -width 8 -textvariable ::CutViewIsoBinding::threshold; pack $w.average.threshold -side left -padx 3
    ttk::entry $w.regions.values -textvariable ::CutViewIsoBinding::regions; pack $w.regions.values -fill x
    ttk::frame $w.actions; pack $w.actions -fill x -padx 8 -pady 5
    ttk::button $w.actions.apply -text Apply -command ::CutViewIsoBinding::apply
    ttk::button $w.actions.close -text Close -command [list destroy $w]; pack $w.actions.apply $w.actions.close -side left -padx 3
    ttk::label $w.status -textvariable ::CutViewIsoBinding::status -wraplength 650; pack $w.status -fill x -padx 8 -pady 4
}
