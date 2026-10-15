# Analytic curved intersections and real pointer acceptance, using only VIEWDRAW.
set quadric_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $quadric_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set quadric_fixtures [file normalize [file join $quadric_dir .. native fixtures]]
proc bgerror {message} {
    puts stderr "CUT_VIEW_QUADRIC_FAIL: $message\n$::errorInfo"
    exit 1
}
proc q_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc q_near {actual expected message {tolerance 1.0e-6}} {
    if {abs($actual-$expected) > $tolerance} { error "$message: $actual != $expected" }
}
proc q_cut {} { return [lindex [scvcutview curved list] 0] }
proc q_check {{synchronized 0}} {
    set shown [scvcutview curved presentation cut]
    set result [scvcutview curved eval cut]
    if {$synchronized} {
        q_assert {[dict get $shown revision] == [dict get $result revision]} "Lazy evaluation hid a stale curved display"
    }
    set cut [q_cut]
    lassign [dict get $cut center] cx cy cz
    lassign [dict get $cut axis] ax ay az
    set radius [dict get $cut radius]
    set values [dict get $result section values]
    q_assert {[llength $values] > 10} "Curved section is empty"
    foreach value $values {
        lassign $value x y z scalar
        q_near $scalar [expr {$x+2*$y+3*$z}] "Curved nodal interpolation" 1.0e-5
        set dx [expr {$x-$cx}]; set dy [expr {$y-$cy}]; set dz [expr {$z-$cz}]
        set squared [expr {$dx*$dx+$dy*$dy+$dz*$dz}]
        if {[dict get $cut shape] eq "cylinder"} {
            set squared [expr {$squared-($ax*$dx+$ay*$dy+$az*$dz)**2}]
        }
        q_near $squared [expr {$radius*$radius}] "Node is off analytic cutting surface" 1.0e-5
    }
    return $result
}
proc q_image {name} {
    set path [file join $::quadric_output $name.png]
    vdump $path
    set stats [scvcutimagestats $path]
    q_assert {[dict get $stats colored] > 150 && [dict get $stats colors] > 20} "Curved contour pixels are blank"
    return [dict get $stats checksum]
}
proc q_find_manipulator {mode} {
    lassign [dict get [scvcutview curved handleinfo] origin] ox oy oz
    lassign [scvcutview curved project $ox $oy $oz] cx cy
    foreach radius {20 30 40 50 60 70 80} {
        for {set angle 0} {$angle < 360} {incr angle 15} {
            set x [expr {round($cx+$radius*cos($angle*acos(-1)/180))}]
            set y [expr {round($cy+$radius*sin($angle*acos(-1)/180))}]
            scvcutview curved mouse move $x $y
            set info [scvcutview curved handleinfo]
            if {[dict get $info detected] eq "manipulator" && [dict get $info mode] == $mode} {
                return [list $x $y $cx $cy]
            }
        }
    }
    error "Curved manipulator mode $mode cannot be picked"
}
proc q_drag {shape mode abort} {
    lassign [q_find_manipulator $mode] px py cx cy
    set saved [q_cut]
    scvcutview curved mouse down $px $py
    set checksum [q_image ${shape}_${mode}_before]
    for {set frame 1} {$frame <= 3} {incr frame} {
        set before [dict get [scvcutview curved presentation cut] synchronizations]
        if {$mode == 1} {
            set tx [expr {$px+5*$frame}]; set ty [expr {$py-3*$frame}]
        } else {
            set angle [expr {5*$frame*acos(-1)/180}]
            set tx [expr {round($cx+($px-$cx)*cos($angle)-($py-$cy)*sin($angle))}]
            set ty [expr {round($cy+($px-$cx)*sin($angle)+($py-$cy)*cos($angle))}]
        }
        scvcutview curved mouse move $tx $ty
        q_assert {[dict get [scvcutview curved handleinfo] dragging]} "Curved drag ended before release"
        q_assert {[dict get [scvcutview curved presentation cut] synchronizations] > $before} "Curved held-drag did not synchronize"
        q_check 1
        set next [q_image ${shape}_${mode}_held_$frame]
        q_assert {$next ne $checksum} "Curved contour pixels did not update during drag"
        set checksum $next
    }
    if {$abort} {
        scvcutview curved mouse abort $px $py
        foreach key {center axis} {
            foreach actual [dict get [q_cut] $key] expected [dict get $saved $key] { q_near $actual $expected "Curved drag cancellation" }
        }
    } else { scvcutview curved mouse up $tx $ty }
    q_assert {![dict get [scvcutview curved handleinfo] dragging]} "Curved drag lifecycle did not finish"
    q_check 1
    ::CutViewManager::refresh 1
}
proc run_quadric_tests {} {
    global quadric_dir quadric_fixtures quadric_output
    set pi [expr {acos(-1)}]
    scvcutview curved read [file join $quadric_fixtures tetra.vtu]
    scvcutview curved sphereadd cut 0.2 0.2 0.2 0.08
    scvcutview curved visibility cut 7
    scvcutview curved field point temperature 0
    set result [q_check]
    q_near [dict get $result section area] [expr {4*$pi*0.08**2}] "Enclosed sphere area"
    q_near [dict get $result negative volume] [expr {4*$pi*0.08**3/3}] "Enclosed sphere volume"
    q_near [expr {[dict get $result negative volume]+[dict get $result positive volume]}] [expr {1.0/6}] "Curved volume conservation"
    q_assert {[dict get $result positive meshKind] eq "boundary"} "Curved boundary mesh mislabeled as volume cells"
    foreach command {{radius cut 0} {radius cut bogus} {spheremove cut 0 0 0 -1} {cylinderadd bad 0 0 0 0 0 0 1}} {
        set saved [scvcutview curved snapshot]
        q_assert {[catch {scvcutview curved {*}$command}]} "Invalid curved edit accepted"
        q_assert {[scvcutview curved snapshot] eq $saved} "Invalid curved edit changed definitions"
    }
    scvcutview curved field cell pressure 0
    foreach value [dict get [scvcutview curved eval cut] section values] { q_near $value 7 "Curved constant CellData" }
    scvcutview curved cellmode cut projected
    foreach value [dict get [scvcutview curved eval cut] section values] { q_near [lindex $value 3] 7 "Curved projected CellData" }
    scvcutview curved field point temperature 0
    set snapshot [scvcutview curved snapshot]
    scvcutview curved restore $snapshot
    q_assert {[scvcutview curved snapshot] eq $snapshot} "Curved JSON v3 round trip"

    scvcutview curved_surface read [file join $quadric_fixtures surface.vtp]
    scvcutview curved_surface sphereadd loop 0.2 0.2 0 0.05
    scvcutview curved_surface field point temperature 0
    set loop [scvcutview curved_surface eval loop]
    q_assert {[dict get $loop lines] > 10 && [llength [dict get $loop lineValues]] > 20} "Enclosed surface intersection loop is missing"
    scvcutview curved_surface field cell pressure 0
    foreach value [dict get [scvcutview curved_surface eval loop] lineValues] { q_near $value 13 "Curved surface CellData" }
    scvcutview curved_surface clear

    set visual [expr {[info exists ::env(CUTVIEW_QUADRIC_VISUAL)] && $::env(CUTVIEW_QUADRIC_VISUAL)}]
    if {$visual} {
        pload VISUALIZATION
        vinit CurvedCutView -width 800 -height 600
        vsetbg -color WHITE
        vaxo
        scvcutview curved display
        scvcutview curved fit
        set quadric_output [file normalize [file join $quadric_dir .. .. build cut-view-quadric]]
        if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set quadric_output $::env(CUTVIEW_OUTPUT_DIR) }
        file mkdir $quadric_output
        source [file join $quadric_dir cut_view_manager.tcl]
    }
    foreach shape {sphere cylinder} {
        if {$shape eq "cylinder"} {
            if {$visual} { ::CutViewManager::close }
            scvcutview curved remove cut
            scvcutview curved cylinderadd cut 0.2 0.2 0.2 0 0 1 0.08
        }
        for {set mask 0} {$mask < 8} {incr mask} {
            scvcutview curved visibility cut $mask
            set result [scvcutview curved eval cut]
            set bit 1
            foreach portion {negative section positive} {
                q_assert {([dict get $result $portion cells] > 0) == (($mask & $bit) != 0)} "Curved portion mask $shape/$mask/$portion"
                set bit [expr {$bit << 1}]
            }
            if {$visual} {
                if {$mask == 0} {
                    set path [file join $quadric_output ${shape}_0.png]
                    vdump $path
                    q_assert {[dict get [scvcutimagestats $path] visible] == 0} "Hidden curved portions still draw"
                } else { q_image ${shape}_$mask }
            }
        }
        scvcutview curved visibility cut 2
        if {$visual} {
            scvcutmanager curved
            update idletasks
            q_assert {$::CutViewManager::shape eq $shape && $::CutViewManager::status eq ""} "Curved manager failed to open"
            q_drag $shape 1 0
            q_drag $shape 1 1
            if {$shape eq "cylinder"} { q_drag $shape 2 0; q_drag $shape 2 1 }
            update idletasks
            if {[auto_execok import] ne ""} {
                exec import -window [winfo id .scvcutmanager] [file join $quadric_output ${shape}_manager.png]
            }
            # Closing the manager must abort a still-held curved manipulation.
            set saved [q_cut]
            lassign [q_find_manipulator 1] px py
            scvcutview curved mouse down $px $py
            scvcutview curved mouse move [expr {$px+8}] [expr {$py-4}]
            q_assert {[dict get [scvcutview curved handleinfo] dragging]} "Close test did not start curved dragging"
            q_check 1
            ::CutViewManager::close
            q_assert {[dict get [scvcutview curved handleinfo] id] eq ""} "Close left a curved positioning aid"
            foreach actual [dict get [q_cut] center] expected [dict get $saved center] { q_near $actual $expected "Close did not cancel curved drag" }
        }
    }
    if {$visual} {
        scvcutview curved handle cut 1
        lassign [q_find_manipulator 1] px py
        scvcutview curved mouse down $px $py
        scvcutview curved mouse move [expr {$px+8}] [expr {$py-4}]
        q_assert {[dict get [scvcutview curved handleinfo] dragging]} "Replace test did not start curved dragging"
        scvcutview curved replace [file join $quadric_fixtures surface.vtp]
        q_assert {[dict get [scvcutview curved handleinfo] id] eq "" && [llength [scvcutview curved list]] == 0} "Replace retained a curved drag/definition"
        source [file join $quadric_dir cut_view_quadric_demo.tcl]
        scvcutview cut_view_curved_demo radius sphere 0.15
        source [file join $quadric_dir cut_view_quadric_demo.tcl]
        q_near [dict get [scvcutview cut_view_curved_demo eval sphere] section area] [expr {4*$pi*0.12**2}] "Repeated curved demo did not reset"
        q_assert {$::CutViewManager::selected eq "sphere" && $::CutViewManager::status eq ""} "Curved demo manager state"
        ::CutViewManager::close
        scvcutview cut_view_curved_demo clear
    }
    scvcutview curved clear
    puts "CUT_VIEW_QUADRIC_PASS"
}
if {[catch {run_quadric_tests} message options]} {
    puts stderr "CUT_VIEW_QUADRIC_FAIL: $message\n[dict get $options -errorinfo]"
    exit 1
}
exit 0
