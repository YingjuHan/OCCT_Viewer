# Analytic bowed-cell sections, original FE values and held-pointer recomputation.
set analytic_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $analytic_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set analytic_fixtures [file normalize [file join $analytic_dir .. .. build test native cut-analytic-fixtures]]
if {[info exists ::env(CUTVIEW_ANALYTIC_FIXTURES)]} { set analytic_fixtures $::env(CUTVIEW_ANALYTIC_FIXTURES) }
proc analytic_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc analytic_near {actual expected tolerance message} {
    if {abs($actual-$expected) > $tolerance} { error "$message: $actual != $expected" }
}
proc bgerror {message} { puts stderr "CUT_VIEW_ANALYTIC_FAIL: $message\n$::errorInfo"; exit 1 }
proc analytic_check {{shown 0}} {
    set presentation [scvcutview analytic_model presentation cut]
    set result [scvcutview analytic_model eval cut]
    if {$shown} { analytic_assert {[dict get $presentation revision] == [dict get $result revision]} "Lazy evaluation concealed a stale analytic display" }
    analytic_assert {[dict get $result sourceGeometry] eq "analytic"} "Required mode returned numerical geometry"
    lassign [dict get [lindex [scvcutview analytic_model list] 0] plane] a b c d
    analytic_assert {[dict get $result section nodes] > 0} "Empty analytic section"
    foreach value [dict get $result section values] {
        lassign $value x y z scalar
        analytic_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 2e-6 "Not a geometric plane section"
        set w [expr {($z-0.4*$x*(1-$x-$y))/(1-0.4*$x)}]
        analytic_near $scalar [expr {$x*$x+2*$y*$y+3*$w*$w}] 2e-5 "Not original quadratic FE values"
    }
    return $result
}
proc analytic_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]
    vdump $path
    set pixels [scvcutimagestats $path]
    analytic_assert {[dict get $pixels colored] > 100 && [dict get $pixels colors] > 20} "Blank analytic cloud"
    return [dict get $pixels checksum]
}
proc analytic_wedge_density {u h rational} {
    set d [expr {0.7+0.4*$u}]
    set k [expr {$rational ? (0.7+0.2*$u)/$d : 1}]
    set section [expr {$k*((1-$u)+($rational ? 0.09/$d : $h/2)*$u*(1-$u)**2)}]
    set derivative [expr {$rational ? ((0.9+0.4*$u)*$d-0.4*(0.9*$u+0.2*$u*$u))/($d*$d) : 1}]
    return [list $section [expr {$section*$derivative}]]
}
proc analytic_wedge_volume {end h rational} {
    # Integrate an independent Bernstein-moment reference, not the cut output.
    set count 512
    set sum [expr {[lindex [analytic_wedge_density 0 $h $rational] 1]+[lindex [analytic_wedge_density $end $h $rational] 1]}]
    for {set n 1} {$n < $count} {incr n} {
        set value [lindex [analytic_wedge_density [expr {$end*$n/$count}] $h $rational] 1]
        set sum [expr {$sum+($n%2 ? 4 : 2)*$value}]
    }
    return [expr {$sum*$end/(3*$count)}]
}
proc analytic_family_references {} {
    foreach fixture {curved_wedge_26 curved_wedge_31 curved_wedge_32 curved_wedge_73 curved_wedge_80 curved_wedge_80_rational curved_pyramid_27 curved_pyramid_37} {
        scvcutview analytic_family read [file join $::analytic_fixtures $fixture.vtu]
        scvcutview analytic_family geometrymode required
        scvcutview analytic_family geometry 0.03 0 7 12 2000000
        set pyramid [string match curved_pyramid_* $fixture]
        scvcutview analytic_family add cut {*}[expr {$pyramid ? {0 0 1 -0.5} : {1 0 0 -0.25}}]
        scvcutview analytic_family visibility cut 7
        scvcutview analytic_family field point temperature 0
        set cut [scvcutview analytic_family eval cut]
        analytic_assert {[dict get $cut sourceGeometry] eq "analytic" && [dict get $cut section nodes] > 0} "Missing analytic $fixture section"
        foreach part {negative section positive} {
            foreach value [dict get $cut $part values] {
                lassign $value x y z scalar
                analytic_near $scalar [expr {$x+2*$y+3*$z}] 2e-5 "$fixture original FE interpolation"
            }
        }
        if {$pyramid} {
            analytic_near [dict get $cut section area] 0.25 2e-6 "$fixture section area"
            analytic_near [dict get $cut negative volume] [expr {7.0/24}] 2e-6 "$fixture negative volume"
            analytic_near [dict get $cut positive volume] [expr {1.0/24}] 2e-6 "$fixture positive volume"
            set low Inf; set high -Inf
            foreach value [dict get $cut section values] {
                set x [lindex $value 0]; set low [expr {min($low,$x)}]; set high [expr {max($high,$x)}]
            }
            analytic_near $low 0.35 2e-6 "$fixture curved left edge"
            analytic_near $high 0.85 2e-6 "$fixture curved right edge"
            scvcutview analytic_family field point z_squared 0
            foreach value [dict get [scvcutview analytic_family eval cut] section values] {
                analytic_near [lindex $value end] 0.25 2e-5 "$fixture original quadratic field"
            }
        } else {
            set rational [string match *_rational $fixture]
            set h [expr {[string match curved_wedge_80* $fixture] ? 0.2 : 0.4}]
            set u [expr {$rational ? (-0.8+sqrt(0.64+0.14))/0.4 : 0.25}]
            analytic_near [dict get $cut section area] [lindex [analytic_wedge_density $u $h $rational] 0] 2e-6 "$fixture section reference"
            analytic_near [dict get $cut negative volume] [analytic_wedge_volume $u $h $rational] 2e-6 "$fixture partial volume reference"
            analytic_near [expr {[dict get $cut negative volume]+[dict get $cut positive volume]}] [analytic_wedge_volume 1.0 $h $rational] 2e-6 "$fixture source volume reference"
        }
        scvcutview analytic_family field cell pressure 0
        foreach value [dict get [scvcutview analytic_family eval cut] section values] { analytic_near $value 7 1e-12 "$fixture constant CellData" }
        scvcutview analytic_family clear
        puts "CUT_VIEW_ANALYTIC_FAMILY_PASS $fixture"
    }
}
proc run_analytic {} {
    analytic_family_references
    scvcutview analytic_model read [file join $::analytic_fixtures bowed_tetra.vtu]
    scvcutview analytic_model geometrymode required
    scvcutview analytic_model geometry 0.03 0 7 12 2000000
    analytic_assert {[scvcutview analytic_model geometrymode] eq "required"} "Geometry edits changed analytic mode"
    scvcutview analytic_model add cut 1 0 0 -0.25
    scvcutview analytic_model visibility cut 7
    scvcutview analytic_model field point quadratic 0
    set result [analytic_check]
    analytic_near [dict get $result section area] 0.253125 2e-6 "Bowed tetra section is faceted"
    analytic_near [expr {[dict get $result negative volume]+[dict get $result positive volume]}] 0.15 2e-6 "Bowed tetra source volume is faceted"
    set prepared [scvcutview analytic_model stats]
    analytic_assert {[dict get $prepared analyticCells] == 1 && [dict get $prepared numericalCells] == 0
        && [dict get $prepared analyticResidual] < 1e-9} "Missing analytic conversion evidence"
    analytic_assert {[catch {scvcutview analytic_model geometrymode unknown}]} "Invalid analytic mode accepted"
    analytic_assert {[scvcutview analytic_model geometrymode] eq "required"
        && [dict get [scvcutview analytic_model stats] epoch] == [dict get $prepared epoch]} "Invalid mode changed source state"
    scvcutview analytic_model geometrymode numerical
    set numeric [scvcutview analytic_model eval cut]
    analytic_assert {[dict get $numeric sourceGeometry] eq "numerical"
        && abs([dict get $numeric section area]-0.253125) > 1e-4} "Numerical and analytic source paths were conflated"
    scvcutview analytic_model geometrymode required
    analytic_check
    set prepared [scvcutview analytic_model stats]
    scvcutview analytic_model field cell pressure 0
    set constant [scvcutview analytic_model eval cut]
    foreach value [dict get $constant section values] { analytic_near $value 7 1e-12 "Analytic CellData provenance" }
    scvcutview analytic_model field point quadratic 0
    if {[info exists ::env(CUTVIEW_ANALYTIC_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit AnalyticCut -width 800 -height 600
        vsetbg -color WHITE
        vaxo
        scvcutview analytic_model visibility cut 2
        scvcutview analytic_model display
        scvcutview analytic_model handle cut 1 0
        scvcutview analytic_model fit
        lassign [scvcutview analytic_model project 0.25 0.2 0.2] px py
        scvcutview analytic_model mouse move $px $py
        analytic_assert {[dict get [scvcutview analytic_model handleinfo] detected] eq "plane"} "Cannot pick analytic cut aid"
        scvcutview analytic_model mouse down $px $py
        set previous [analytic_image before]
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview analytic_model presentation cut] synchronizations]
            scvcutview analytic_model mouse move [expr {$px-3*$frame}] $py
            analytic_assert {[dict get [scvcutview analytic_model handleinfo] dragging]} "Analytic drag ended before release"
            analytic_assert {[dict get [scvcutview analytic_model presentation cut] synchronizations] > $sync} "No held analytic synchronization"
            set work [scvcutview analytic_model stats]
            foreach key {epoch preparations shapeBuilds bvhBuilds analyticCells numericalCells} {
                analytic_assert {[dict get $work $key] == [dict get $prepared $key]} "Held drag rebuilt analytic $key"
            }
            analytic_check 1
            set next [analytic_image held_$frame]
            analytic_assert {$next ne $previous} "Analytic cloud did not change before release"
            set previous $next
        }
        scvcutview analytic_model mouse abort $px $py
        set restored [analytic_check 1]
        analytic_near [dict get $restored section area] 0.253125 2e-6 "Analytic abort rollback"
    }
    scvcutview analytic_model clear
    puts "CUT_VIEW_ANALYTIC_PASS"
}
if {[catch {run_analytic} message]} { puts stderr "CUT_VIEW_ANALYTIC_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
