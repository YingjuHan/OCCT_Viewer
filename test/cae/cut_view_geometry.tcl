# Configurable native geometry, conforming curved interfaces and held-pointer acceptance.
set geometry_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $geometry_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set geometry_fixtures [file normalize [file join $geometry_dir .. .. build test native cut-geometry-fixtures]]
if {[info exists ::env(CUTVIEW_GEOMETRY_FIXTURES)]} { set geometry_fixtures $::env(CUTVIEW_GEOMETRY_FIXTURES) }
proc geometry_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc geometry_near {actual expected tolerance message} {
    if {abs($actual-$expected) > $tolerance} { error "$message: $actual != $expected" }
}
proc bgerror {message} { puts stderr "CUT_VIEW_GEOMETRY_FAIL: $message\n$::errorInfo"; exit 1 }
proc geometry_check {{shown 0}} {
    set presentation [scvcutview geometry_model presentation cut]
    set result [scvcutview geometry_model eval cut]
    if {$shown} { geometry_assert {[dict get $presentation revision] == [dict get $result revision]} "Lazy eval concealed stale curved display" }
    lassign [dict get [lindex [scvcutview geometry_model list] 0] plane] a b c d
    geometry_assert {[dict get $result section nodes] > 0} "Empty curved section"
    foreach value [dict get $result section values] {
        lassign $value x y z scalar
        geometry_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 2e-5 "Not a geometric plane section"
        geometry_near $scalar [expr {$x+2*$y+3*$z}] 0.015 "Curved original FE interpolation"
    }
    return $result
}
proc geometry_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]
    vdump $path
    set pixels [scvcutimagestats $path]
    geometry_assert {[dict get $pixels colored] > 100 && [dict get $pixels colors] > 20} "Blank curved cloud"
    return [dict get $pixels checksum]
}
proc run_geometry {} {
    scvcutview geometry_model read [file join $::geometry_fixtures joined_curved_hex.vtu]
    scvcutview geometry_model geometry 0.007 0 7 12 2000000
    scvcutview geometry_model add cut 0 1 0 -0.5
    scvcutview geometry_model visibility cut 7
    scvcutview geometry_model field point temperature 0
    set result [geometry_check]
    set length [expr {0.5*sqrt(1+0.4*0.4)+log(0.4+sqrt(1+0.4*0.4))/(2*0.4)}]
    foreach side {negative positive} {
        geometry_near [dict get $result $side volume] 1 2e-5 "Curved side volume"
        geometry_near [dict get $result $side area] [expr {6+$length}] 0.012 "Curved shared internal wall remains"
    }
    geometry_near [dict get $result section area] 2 2e-5 "Joined section area"
    set prepared [scvcutview geometry_model stats]
    geometry_assert {[dict get $prepared filteredFaces] > 0 && [dict get $prepared unmergedGroups] == 0
        && [dict get $prepared faceDepth] > 0} "Missing conforming curved refinement"
    geometry_near [dict get $prepared geometryTolerance] 0.007 1e-12 "Requested tolerance ignored"
    set saved [scvcutview geometry_model geometry]
    foreach arguments {{0 0 7 12 2000000} {-1 0 7 12 2000000} {0.007 -1 7 12 2000000}
                       {0.007 0 11 12 2000000} {0.007 0 7 21 2000000} {0.007 0 7 12 0}
                       {NaN 0 7 12 2000000} {0.007 0 1.5 12 2000000}} {
        geometry_assert {[catch {scvcutview geometry_model geometry {*}$arguments}]} "Invalid geometry options accepted"
        geometry_assert {[scvcutview geometry_model geometry] eq $saved} "Invalid options partially applied"
        geometry_assert {[dict get [scvcutview geometry_model stats] epoch] == [dict get $prepared epoch]} "Invalid options changed source epoch"
    }
    scvcutview geometry_model geometry 0.007 0 7 12 2000000
    geometry_assert {[dict get [scvcutview geometry_model stats] epoch] == [dict get $prepared epoch]} "Identical options rebuilt source"
    if {[info exists ::env(CUTVIEW_GEOMETRY_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit CurvedGeometry -width 800 -height 600
        vsetbg -color WHITE
        vaxo
        scvcutview geometry_model visibility cut 2
        scvcutview geometry_model display
        scvcutview geometry_model handle cut 1 0
        scvcutview geometry_model fit
        lassign [scvcutview geometry_model project 0.5 0.5 0.6] px py
        scvcutview geometry_model mouse move $px $py
        geometry_assert {[dict get [scvcutview geometry_model handleinfo] detected] eq "plane"} "Cannot pick curved mesh cut aid"
        scvcutview geometry_model mouse down $px $py
        set previous [geometry_image before]
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview geometry_model presentation cut] synchronizations]
            scvcutview geometry_model mouse move [expr {$px-3*$frame}] $py
            geometry_assert {[dict get [scvcutview geometry_model handleinfo] dragging]} "Curved drag ended before release"
            geometry_assert {[dict get [scvcutview geometry_model presentation cut] synchronizations] > $sync} "Held curved display not synchronized"
            set work [scvcutview geometry_model stats]
            foreach key {epoch preparations shapeBuilds bvhBuilds sourcePrimitives faceDepth} {
                geometry_assert {[dict get $work $key] == [dict get $prepared $key]} "Dragging rebuilt curved $key"
            }
            geometry_check 1
            set next [geometry_image held_$frame]
            geometry_assert {$next ne $previous} "Curved cloud pixels did not update while held"
            set previous $next
        }
        scvcutview geometry_model mouse abort $px $py
        geometry_check 1
    }
    scvcutview geometry_model clear
    puts "CUT_VIEW_GEOMETRY_PASS"
}
if {[catch {run_geometry} message]} { puts stderr "CUT_VIEW_GEOMETRY_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
