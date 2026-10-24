# Native source reuse and adjacency acceptance, with actual held-pointer events.
set cache_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $cache_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set cache_fixture_dir [file normalize [file join $cache_dir .. .. build test native general-cache-fixtures]]
if {[info exists ::env(CUTVIEW_CACHE_FIXTURES)]} { set cache_fixture_dir $::env(CUTVIEW_CACHE_FIXTURES) }
proc cache_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc cache_near {actual expected message} {
    if {abs($actual-$expected) > 1.0e-5} { error "$message: $actual != $expected" }
}
proc bgerror {message} { puts stderr "CUT_VIEW_CACHE_FAIL: $message\n$::errorInfo"; exit 1 }
proc cache_check {{shown 0}} {
    set presentation [scvcutview cache_model presentation cut]
    set result [scvcutview cache_model eval cut]
    if {$shown} { cache_assert {[dict get $presentation revision] == [dict get $result revision]} "Lazy eval concealed stale cached display" }
    set definition [lindex [scvcutview cache_model list] 0]
    lassign [dict get $definition plane] a b c d
    cache_assert {[dict get $result section nodes] > 0} "Cached section is empty"
    foreach value [dict get $result section values] {
        lassign $value x y z scalar
        cache_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 "Cached geometric section"
        cache_near $scalar [expr {$x+2*$y+3*$z}] "Cached original FE values"
    }
    return $result
}
proc cache_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]
    vdump $path
    set stats [scvcutimagestats $path]
    cache_assert {[dict get $stats colored] > 100 && [dict get $stats colors] > 20} "Blank cached contour"
    return [dict get $stats checksum]
}
proc run_cache {} {
    scvcutview cache_model read [file join $::cache_fixture_dir two_lagrange_hex.vtu]
    scvcutview cache_model add cut 0 1 0 -0.5
    scvcutview cache_model visibility cut 7
    scvcutview cache_model field point temperature 0
    set result [cache_check]
    foreach side {negative positive} {
        cache_near [dict get $result $side area] 7 "Shared internal side wall remains"
        cache_near [dict get $result $side volume] 1 "Cached side volume"
    }
    cache_near [dict get $result section area] 2 "Joined section area"
    set prepared [scvcutview cache_model stats]
    cache_assert {[dict get $prepared preparations] == 1 && [dict get $prepared preparedCells] == 2
        && [dict get $prepared shapeBuilds] == 2 && [dict get $prepared bvhBuilds] == 1
        && [dict get $prepared filteredFaces] > 0} "Missing source/BVH reuse or adjacency filtering"
    scvcutview cache_model move cut 1 0 0 -1
    scvcutview cache_model field cell pressure 0
    set interface [scvcutview cache_model eval cut]
    cache_near [dict get $interface section area] 1 "Shared face section is duplicated"
    foreach value [dict get $interface section values] { cache_near $value 7 "Interface trace owner changed" }
    scvcutview cache_model field point temperature 0
    scvcutview cache_model move cut 0 1 0 -0.5
    foreach offset {0.3 0.4 0.6 0.7} {
        scvcutview cache_model move cut 0 1 0 [expr {-$offset}]
        cache_check
        set work [scvcutview cache_model stats]
        foreach key {epoch preparations preparedCells shapeBuilds bvhBuilds} {
            cache_assert {[dict get $work $key] == [dict get $prepared $key]} "Translation invalidated prepared $key"
        }
    }
    if {[info exists ::env(CUTVIEW_CACHE_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit CachedCut -width 800 -height 600
        vsetbg -color WHITE
        vaxo
        scvcutview cache_model move cut 0 1 0 -0.5
        scvcutview cache_model visibility cut 2
        scvcutview cache_model display
        scvcutview cache_model handle cut 1 0
        scvcutview cache_model fit
        lassign [scvcutview cache_model project 0.5 0.5 0.5] px py
        scvcutview cache_model mouse move $px $py
        cache_assert {[dict get [scvcutview cache_model handleinfo] detected] eq "plane"} "Cannot pick cached cut aid"
        scvcutview cache_model mouse down $px $py
        set previous [cache_image before]
        set saved [dict get [lindex [scvcutview cache_model list] 0] plane]
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview cache_model presentation cut] synchronizations]
            scvcutview cache_model mouse move [expr {$px-3*$frame}] $py
            set work [scvcutview cache_model stats]
            cache_assert {[dict get [scvcutview cache_model handleinfo] dragging]} "Cached drag ended before release"
            cache_assert {[dict get [scvcutview cache_model presentation cut] synchronizations] > $sync} "No held-drag display synchronization"
            foreach key {epoch preparations preparedCells shapeBuilds bvhBuilds} {
                cache_assert {[dict get $work $key] == [dict get $prepared $key]} "Held drag rebuilt prepared $key"
            }
            cache_check 1
            set next [cache_image held_$frame]
            cache_assert {$next ne $previous} "Held cached cloud pixels did not change"
            set previous $next
        }
        scvcutview cache_model mouse abort $px $py
        foreach actual [dict get [lindex [scvcutview cache_model list] 0] plane] expected $saved { cache_near $actual $expected "Cached drag rollback" }
        cache_check 1
    }
    scvcutview cache_model clear
    puts "CUT_VIEW_CACHE_PASS"
}
if {[catch {run_cache} message]} { puts stderr "CUT_VIEW_CACHE_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
