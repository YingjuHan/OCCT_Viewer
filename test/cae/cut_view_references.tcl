# Independent curved-family geometry and original FE references, plus precision fallbacks.
set reference_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $reference_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set reference_fixtures [file normalize [file join $reference_dir .. .. build test native cut-reference-fixtures]]
if {[info exists ::env(CUTVIEW_REFERENCE_FIXTURES)]} { set reference_fixtures $::env(CUTVIEW_REFERENCE_FIXTURES) }
proc reference_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc reference_near {actual expected tolerance message} {
    if {![expr {abs($actual-$expected) <= $tolerance}]} { error "$message: $actual != $expected" }
}
proc bgerror {message} { puts stderr "CUT_VIEW_REFERENCE_FAIL: $message\n$::errorInfo"; exit 1 }
proc reference_map {u bezier rational degree height} {
    set d [expr {$rational ? 0.7+0.4*$u : 1}]
    set a [expr {$rational ? 0.7+0.4/$degree : 1}]
    set b [expr {$rational ? 0.4*(1-1.0/$degree) : 0}]
    set x [expr {$rational ? ($a*$u+$b*$u*$u)/$d : $u}]
    set dx [expr {(($a+2*$b*$u)*$d-($rational ? 0.4 : 0)*($a*$u+$b*$u*$u))/($d*$d)}]
    set f [expr {$bezier ? 1-1.0/$degree : 1}]
    set a [expr {$rational ? 0.7+0.4/$degree : 1}]
    set b [expr {$rational ? 0.4*(1-2.0/$degree) : 0}]
    set numerator [expr {$u*(1-$u)*($a+$b*$u)}]
    set bow [expr {$height*$f*$numerator/$d}]
    set derivative [expr {$height*$f*(((1-2*$u)*($a+$b*$u)+$b*$u*(1-$u))*$d-($rational ? 0.4 : 0)*$numerator)/($d*$d)}]
    set k [expr {$rational ? (0.7+0.2*$u)/$d : 1}]
    set second [expr {$u*$u+$u*(1-$u)/$degree}]
    set third [expr {$u**3+3*$u*$u*(1-$u)/$degree+$u*(1-$u)*(1-2*$u)/($degree*$degree)}]
    set scalar [expr {!$bezier ? $u*$u : $rational ? (0.7*$second+0.4*$third)/$d : $second}]
    return [list $x $dx $bow $derivative $k $scalar]
}
proc reference_parameter {x bezier rational degree height} {
    set low 0.0; set high 1.0
    for {set n 0} {$n < 52} {incr n} {
        set mid [expr {($low+$high)/2}]
        if {[lindex [reference_map $mid $bezier $rational $degree $height] 0] < $x} { set low $mid } else { set high $mid }
    }
    return [expr {($low+$high)/2}]
}
proc reference_density {u bezier rational degree triangle tetra} {
    lassign [reference_map $u $bezier $rational $degree 0.4] x dx bow derivative k
    if {$tetra} {
        set shear [expr {$rational ? 0.18/(0.7+0.4*$u) : $bezier ? 0.2 : 0.4}]
        return [expr {$dx*$k*($k-$shear*$u)*(1-$u)**2/2}]
    }
    return [expr {($triangle ? $k*(1-$u) : 1)*sqrt($dx*$dx+$derivative*$derivative)}]
}
proc reference_integral {end bezier rational degree triangle tetra} {
    set count 512
    set sum [expr {[reference_density 0 $bezier $rational $degree $triangle $tetra]+[reference_density $end $bezier $rational $degree $triangle $tetra]}]
    for {set n 1} {$n < $count} {incr n} {
        set sum [expr {$sum+($n%2 ? 4 : 2)*[reference_density [expr {$end*1.0*$n/$count}] $bezier $rational $degree $triangle $tetra]}]
    }
    return [expr {$sum*$end/(3*$count)}]
}
proc reference_check {type rational degree {shown 0}} {
    set presentation [scvcutview reference_model presentation cut]
    set cut [scvcutview reference_model eval cut]
    if {$shown} { reference_assert {[dict get $presentation revision] == [dict get $cut revision]} "Lazy evaluation hid a stale reference presentation" }
    set bezier [expr {$type >= 75}]
    set curve [expr {$type in {21 35 68 75}}]
    set tetra [expr {$type in {71 78}}]
    set triangle [expr {$type in {22 34 69 76}}]
    set x [expr {-[lindex [dict get [lindex [scvcutview reference_model list] 0] plane] 3]}]
    set u [reference_parameter $x $bezier $rational $degree [expr {$curve ? 1.0 : 0.4}]]
    lassign [reference_map $u $bezier $rational $degree [expr {$curve ? 1.0 : 0.4}]] mapped dx bow derivative k squared
    reference_assert {[dict get $cut sourceGeometry] eq "analytic" && [dict get $cut section nodes] > 0} "Missing analytic reference section"
    foreach value [dict get $cut section values] {
        lassign $value px py pz scalar
        reference_near $px $x 2e-6 "Reference section is not a geometric plane"
        if {!$tetra} { reference_near [expr {$curve ? $py : $pz}] $bow 2e-6 "Curved map replaced by a corner chord" }
        reference_near $scalar [expr {$px+2*$py+3*$pz}] 2e-5 "Original affine FE mapping"
    }
    if {!$curve} {
        set quantity [expr {$tetra ? "volume" : "area"}]
        reference_near [dict get $cut negative $quantity] [reference_integral $u $bezier $rational $degree $triangle $tetra] 3e-6 "Independent curved partial measure"
        if {[dict get $cut visible] & 4} {
            reference_near [expr {[dict get $cut negative $quantity]+[dict get $cut positive $quantity]}] [reference_integral 1 $bezier $rational $degree $triangle $tetra] 3e-6 "Independent curved source measure"
        }
        if {$tetra} {
            reference_near [dict get $cut section area] [expr {[reference_density $u $bezier $rational $degree $triangle $tetra]/$dx}] 3e-6 "Independent tetra section area"
        } else { reference_assert {[dict get $cut lines] > 0 && [llength [dict get $cut lineValues]] > 0} "Missing original FE section lines" }
    }
    return $cut
}
proc reference_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]; vdump $path
    set pixels [scvcutimagestats $path]
    reference_assert {[dict get $pixels colored] > 100 && [dict get $pixels colors] > 20} "Blank curved reference cloud"
    return [dict get $pixels checksum]
}
proc run_references {} {
    foreach stem {21 35 68 75 22 23 28 30 34 69 70 76 77 71 78 75_rational 76_rational 77_rational 78_rational
                  68_d6x0 75_rational_d6x0 70_d2x3 77_rational_d2x3 70_d6x2} {
        regexp {^([0-9]+)} $stem unused type
        set rational [string match *_rational* $stem]; set degree 2
        if {[regexp {_d([0-9]+)x} $stem unused order]} { set degree $order }
        scvcutview reference_model read [file join $::reference_fixtures curved_reference_$stem.vtu]
        scvcutview reference_model geometrymode required
        scvcutview reference_model geometry 0.01 0 7 12 2000000
        scvcutview reference_model add cut 1 0 0 -0.25
        scvcutview reference_model visibility cut 7
        scvcutview reference_model field point temperature 0
        reference_check $type $rational $degree
        scvcutview reference_model field point u_squared 0
        set cut [scvcutview reference_model eval cut]
        set u [reference_parameter 0.25 [expr {$type >= 75}] $rational $degree 0.4]
        set expected [lindex [reference_map $u [expr {$type >= 75}] $rational $degree 0.4] 5]
        reference_assert {[llength [dict get $cut section values]] > 0} "Missing nonlinear FE values"
        foreach value [dict get $cut section values] { reference_near [lindex $value end] $expected 2e-5 "Independent nonlinear FE reference" }
        scvcutview reference_model field cell pressure 0
        set constant [scvcutview reference_model eval cut]
        foreach value [dict get $constant section values] { reference_near $value 7 1e-12 "Constant original CellData" }
        foreach value [dict get $constant lineValues] { reference_near $value 7 1e-12 "Constant section-line CellData" }
        scvcutview reference_model clear
        puts "CUT_VIEW_REFERENCE_FAMILY_PASS $stem"
    }
    foreach stem {unsupported_degree_seven unsupported_weight_precision} {
        scvcutview reference_model read [file join $::reference_fixtures $stem.vtu]
        scvcutview reference_model geometry 2 0 7 12 2000000
        scvcutview reference_model geometrymode required
        scvcutview reference_model add cut 1 0 0 -0.25
        reference_assert {[catch {scvcutview reference_model eval cut}]} "Unsupported analytic representation accepted"
        reference_assert {[dict get [scvcutview reference_model stats] preparations] == 0} "Failed analytic source was published"
        scvcutview reference_model geometrymode preferred
        # The extreme-weight fallback has intentionally coarse geometry, not a reference accuracy claim.
        reference_assert {[dict get [scvcutview reference_model eval cut] sourceGeometry] eq "numerical"} "Unsupported representation did not recover numerically"
        scvcutview reference_model clear
    }
    if {[info exists ::env(CUTVIEW_REFERENCE_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        scvcutview reference_model read [file join $::reference_fixtures curved_reference_69.vtu]
        scvcutview reference_model geometrymode required
        scvcutview reference_model geometry 0.01 0 7 12 2000000
        scvcutview reference_model add cut 1 0 0 -0.25
        # A visible clipped boundary makes cloud changes observable during motion.
        scvcutview reference_model visibility cut 3
        scvcutview reference_model field point temperature 0
        vinit CurvedReferences -width 800 -height 600; vsetbg -color WHITE; vaxo
        scvcutview reference_model display; scvcutview reference_model handle cut 1 0; scvcutview reference_model fit
        reference_check 69 0 2 1
        set work [scvcutview reference_model stats]
        lassign [scvcutview reference_model project 0.25 0.3 0.075] px py
        scvcutview reference_model mouse move $px $py
        reference_assert {[dict get [scvcutview reference_model handleinfo] detected] eq "plane"} "Cannot pick curved surface plane"
        scvcutview reference_model mouse down $px $py
        set previous [reference_image before]
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview reference_model presentation cut] synchronizations]
            scvcutview reference_model mouse move [expr {$px-3*$frame}] $py
            reference_assert {[dict get [scvcutview reference_model handleinfo] dragging]} "Surface reference drag ended before release"
            reference_assert {[dict get [scvcutview reference_model presentation cut] synchronizations] > $sync} "Held surface reference was not synchronized"
            reference_check 69 0 2 1
            set warm [scvcutview reference_model stats]
            foreach key {epoch preparations shapeBuilds bvhBuilds} { reference_assert {[dict get $warm $key] == [dict get $work $key]} "Held reference rebuilt source $key" }
            set next [reference_image held_$frame]
            reference_assert {$next ne $previous} "Surface reference pixels did not change before release"
            set previous $next
        }
        scvcutview reference_model mouse abort $px $py
        reference_check 69 0 2 1
        reference_near [lindex [dict get [lindex [scvcutview reference_model list] 0] plane] 3] -0.25 1e-12 "Reference drag rollback failed"
        scvcutview reference_model clear
    }
    puts "CUT_VIEW_REFERENCE_PASS"
}
if {[catch {run_references} message]} { puts stderr "CUT_VIEW_REFERENCE_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
