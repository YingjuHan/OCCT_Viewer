# Recovery must preserve material traces and must not change physical resultants.
set recovery_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $recovery_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set recovery_fixtures [file normalize [file join $recovery_dir .. native fixtures]]
proc recovery_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc recovery_near {value expected message} { if {abs($value-$expected) > 2e-5} { error "$message: $value != $expected" } }
proc bgerror {message} { puts stderr "CUT_VIEW_RECOVERY_FAIL: $message\n$::errorInfo"; exit 1 }
proc recovery_jump {{shown 0}} {
    set presentation [scvcutview recovery_model presentation cut]
    set cut [scvcutview recovery_model eval cut]
    if {$shown} { recovery_assert {[dict get $presentation revision] == [dict get $cut revision]} "Stale recovered presentation" }
    recovery_assert {[dict get $cut section valueMapping] eq "element-node"} "Recovery lost element-node mapping"
    set found {}
    foreach value [dict get $cut section values] {
        lassign $value e n x y z scalar
        recovery_near $y 0.5 "Recovery is not a geometric plane section"
        recovery_assert {abs($scalar-7) < 1e-5 || abs($scalar-9) < 1e-5} "Material jump averaged"
        dict set found $scalar 1
    }
    recovery_assert {[dict size $found] == 2} "Missing material trace"
    return $cut
}
proc recovery_image {name} {
    set path [file join $::env(CUTVIEW_OUTPUT_DIR) $name.png]
    vdump $path
    set pixels [scvcutimagestats $path]
    recovery_assert {[dict get $pixels colored] > 100 && [dict get $pixels colors] >= 2} "Blank recovered cloud"
    return [dict get $pixels checksum]
}
proc run_recovery {} {
    scvcutview recovery_model read [file join $::recovery_fixtures recovery_hex.vtu]
    scvcutview recovery_model add cut 0 1 0 -0.5
    scvcutview recovery_model visibility cut 2
    scvcutview recovery_model cellmode cut projected
    scvcutview recovery_model field cell pressure 0
    set projected [scvcutview recovery_model eval cut]
    foreach value [dict get $projected section values] { lassign $value x y z scalar; recovery_near $scalar [expr {7+$x}] "Legacy projection changed" }
    scvcutview recovery_model recovery 100 0 material
    recovery_jump
    set stable [scvcutview recovery_model recovery]
    set snapshot [scvcutview recovery_model snapshot]
    scvcutview recovery_model recovery 0 1 none
    scvcutview recovery_model restore $snapshot
    recovery_assert {[scvcutview recovery_model recovery] eq $stable && [scvcutview recovery_model snapshot] eq $snapshot} "Recovery option round trip"
    set exact [string map [list {"regions":[10,20]} {"regions":[-9223372036854775808,9223372036854775807]}] $snapshot]
    scvcutview recovery_model restore $exact
    set ids [dict get [scvcutview recovery_model recovery] regions]
    recovery_assert {[lindex $ids 0] eq "-9223372036854775808" && [lindex $ids 1] eq "9223372036854775807"} "Region ID round trip lost precision"
    scvcutview recovery_model recovery 99 1 keep
    recovery_assert {[dict get [scvcutview recovery_model recovery] regions] eq $ids} "Keep regions changed exact IDs"
    scvcutview recovery_model restore $snapshot
    foreach command {{-1 0 none} {101 0 none} {100 2 none} {100 0 absent} {100 0 invalid_region} {100 0 none 0}} {
        recovery_assert {[catch {scvcutview recovery_model recovery {*}$command}]} "Invalid recovery settings accepted"
        recovery_assert {[scvcutview recovery_model recovery] eq $stable} "Invalid settings changed live recovery"
    }
    scvcutview recovery_model recovery 0 0 none
    recovery_jump
    scvcutview recovery_model recovery 99 0 none
    recovery_jump
    scvcutview recovery_model recovery 100 0 none
    scvcutview recovery_model field cell large 0
    foreach policy {{100 0 none} {100 0 material}} {
        scvcutview recovery_model recovery {*}$policy
        set large [scvcutview recovery_model eval cut]
        set shared 0
        foreach value [dict get $large section values] {
            set scalar [lindex $value end]
            recovery_assert {$scalar == $scalar && abs($scalar) <= 1.2e308} "Finite recovery overflowed"
            if {[dict get $large section valueMapping] eq "point" && abs([lindex $value 0]-1) < 1e-6} {
                recovery_near [expr {$scalar/1e308}] 1 "Default finite mean overflowed"
                set shared 1
            }
        }
        if {[dict get $large section valueMapping] eq "point"} { recovery_assert {$shared} "No large-value shared node tested" }
    }
    scvcutview recovery_model field cell extreme_jump 0
    scvcutview recovery_model recovery 99 0 none
    set extreme [scvcutview recovery_model eval cut]
    foreach value [dict get $extreme section values] { recovery_near [expr {abs([lindex $value end]/1.7e308)}] 1 "Overflowed global range ignored threshold" }
    scvcutview recovery_model recovery 100 1 none
    set extreme [scvcutview recovery_model eval cut]
    set shared 0
    foreach value [dict get $extreme section values] {
        if {abs([lindex $value 2]-1) < 1e-6} { recovery_near [lindex $value end] 0 "Opposite-sign recovery overflowed"; set shared 1 }
    }
    recovery_assert {$shared} "No opposite-sign shared node tested"
    scvcutview recovery_model recovery 100 0 none
    scvcutview recovery_model field cell opposing 0
    set after [scvcutview recovery_model eval cut]
    foreach value [dict get $after section values] {
        lassign $value x y z magnitude
        recovery_near $magnitude [expr {abs(1-$x)}] "Post-average vector magnitude"
    }
    scvcutview recovery_model recovery 100 1 none
    set before [scvcutview recovery_model eval cut]
    foreach value [dict get $before section values] { recovery_near [lindex $value end] 1 "Before-average vector magnitude" }
    scvcutview recovery_model move cut 1 0 0 -0.25
    foreach policy {{100 0 none} {100 0 material} {0 0 none} {100 1 none}} {
        scvcutview recovery_model recovery {*}$policy
        recovery_near [dict get [scvcutview recovery_model heatflow cut flux 0 1 2] heatFlow] 7 "Display recovery changed physical heat flow"
        set force [scvcutview recovery_model freebody cut stress 0 1 2 3 4 5 0 0 0]
        recovery_near [lindex [dict get $force force] 0] 7 "Display recovery changed physical force"
        recovery_near [lindex [dict get $force moment] 1] 3.5 "Display recovery changed physical moment"
    }
    scvcutview recovery_model move cut 0 1 0 -0.5
    scvcutview recovery_model field cell pressure 0
    scvcutview recovery_model recovery 100 0 material
    if {[info exists ::env(CUTVIEW_RECOVERY_VISUAL)]} {
        file mkdir $::env(CUTVIEW_OUTPUT_DIR)
        vinit RecoveryCut -width 800 -height 600
        vsetbg -color WHITE
        vaxo
        scvcutview recovery_model display
        scvcutview recovery_model handle cut 1 0
        scvcutview recovery_model fit
        set before [recovery_image before]
        lassign [scvcutview recovery_model project 0.5 0.5 0.5] px py
        scvcutview recovery_model mouse move $px $py
        recovery_assert {[dict get [scvcutview recovery_model handleinfo] detected] eq "plane"} "Cannot pick recovered section aid"
        scvcutview recovery_model mouse down $px $py
        for {set frame 1} {$frame <= 3} {incr frame} {
            set sync [dict get [scvcutview recovery_model presentation cut] synchronizations]
            scvcutview recovery_model mouse move $px [expr {$py+3*$frame}]
            recovery_assert {[dict get [scvcutview recovery_model handleinfo] dragging]} "Recovery drag ended before release"
            recovery_assert {[dict get [scvcutview recovery_model presentation cut] synchronizations] > $sync} "No recovered display before release"
            set cut [scvcutview recovery_model eval cut]
            foreach value [dict get $cut section values] { recovery_assert {[lindex $value end] == 7 || [lindex $value end] == 9} "Held recovery lost material trace" }
            set next [recovery_image held_$frame]
            recovery_assert {$next ne $before} "Recovered cloud did not update before release"
            set before $next
        }
        scvcutview recovery_model mouse abort $px $py
        recovery_jump 1
    }
    scvcutview recovery_model clear
    scvcutview recovery_model read [file join $::recovery_fixtures recovery_surface.vtp]
    scvcutview recovery_model add cut 0 1 0 -0.5
    scvcutview recovery_model visibility cut 2
    scvcutview recovery_model cellmode cut projected
    scvcutview recovery_model field cell pressure 0
    scvcutview recovery_model recovery 100 0 material
    set lines [scvcutview recovery_model eval cut]
    recovery_assert {[dict get $lines lines] == 2} "Recovered section lines missing"
    recovery_assert {[lsort -unique [dict get $lines lineValues]] eq {7 9}} "Recovered line material mapping lost"
    if {[info exists ::env(CUTVIEW_RECOVERY_VISUAL)]} {
        scvcutview recovery_model display
        scvcutview recovery_model fit
        recovery_image lines
    }
    scvcutview recovery_model clear
    puts "CUT_VIEW_RECOVERY_PASS"
}
if {[catch {run_recovery} message]} { puts stderr "CUT_VIEW_RECOVERY_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
