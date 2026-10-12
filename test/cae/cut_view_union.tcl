set ::env(CUTVIEW_KEEP_RUNNING) 1
source [file join [file dirname [info script]] cut_view.tcl]

proc cut_union_image {path} {
    vdump $path
    set stats [scvcutimagestats $path]
    cut_assert {[dict get $stats colored] > 500 && [dict get $stats colors] > 20} "Union cloud is blank or uncolored"
}
proc cut_union_compare {reference actual} {
    set difference [scvcutimagecompare $reference $actual]
    puts "UNION_IMAGE_DIFF [file tail $actual]: $difference"
    cut_assert {[dict get $difference mean] < 0.01} "Duplicate or covered region changed transparent opacity"
}
proc run_cut_union {} {
    global cut_view_fixtures
    scvcutview union_model read [file join $cut_view_fixtures union_cube.vtu]
    scvcutview union_model add a 1 0 0 -0.4
    scvcutview union_model visibility a 3
    scvcutview union_model field point temperature 0
    scvcutview union_model copy a b
    set duplicate [scvcutview union_model union]
    cut_near [dict get $duplicate volume] 0.4 "Duplicate retained volume"
    cut_near [dict get $duplicate area] 3.6 "Duplicate exposed area"
    cut_assert {[dict get $duplicate unified] && [dict get $duplicate primitives] > 0} "Union artifact absent"
    foreach primitive [dict get $duplicate provenance] {
        cut_assert {[dict get $primitive cut] eq "a" && [dict get $primitive source] == 0} "Unstable cap/source ownership"
        if {[dict get $primitive cap]} { cut_assert {[dict get $primitive portion] == 1} "Cap is not owned by On" }
    }
    set independent [scvcutview union_model eval a]
    scvcutview union_model move b 0 1 0 -0.3
    set crossed [scvcutview union_model union]
    cut_near [dict get $crossed volume] 0.58 "Crossed OR volume"
    cut_near [dict get $crossed area] 5.16 "Crossed exposed area"
    cut_near [dict get [scvcutview union_model eval a] negative volume] [dict get $independent negative volume] "Independent volume changed"
    scvcutview union_model visibility a 7
    scvcutview union_model visibility b 7
    set full [scvcutview union_model union]
    cut_near [dict get $full volume] 1 "Full OR volume"
    cut_near [dict get $full area] 6 "Internal caps remain"
    foreach primitive [dict get $full provenance] { cut_assert {![dict get $primitive cap]} "Interior cap was displayed" }
    scvcutview union_model style negative 0.55
    scvcutview union_model style on 0.55
    scvcutview union_model style positive 0.55
    cut_assert {[catch {scvcutview union_model style on 1.5}]} "Invalid transparency accepted"
    cut_near [dict get [scvcutview union_model style on] transparency] 0.55 "Invalid transparency changed style"
    if {[info exists ::env(CUTVIEW_UNION_VISUAL)]} {
        pload VISUALIZATION
        vinit UnionView -width 800 -height 600
        vsetbg -color WHITE
        # Isolate contour color/opacity from lighting and primitive draw order.
        vrenderparams -shadingModel unlit -oit weighted
        set output $::env(CUTVIEW_OUTPUT_DIR)
        file mkdir $output
        scvcutview union_model display
        vaxo
        vfit
        scvcutview union_model visibility a 1
        scvcutview union_model visibility b 1
        scvcutview union_model enable b 0
        set single [file join $output single.png]
        cut_union_image $single
        scvcutview union_model move b 1 0 0 -0.4
        scvcutview union_model enable b 1
        set duplicate [file join $output duplicate.png]
        cut_union_image $duplicate
        cut_union_compare $single $duplicate
        scvcutview union_model move b 1 0 0 -0.6
        scvcutview union_model enable a 0
        set reference [file join $output contained_reference.png]
        cut_union_image $reference
        scvcutview union_model enable a 1
        set contained [file join $output contained.png]
        cut_union_image $contained
        cut_union_compare $reference $contained
        scvcutview union_model move b 0 1 0 -0.3
        cut_union_image [file join $output crossing.png]
        scvcutview union_model visibility a 7
        scvcutview union_model visibility b 7
        set full [file join $output all_masks.png]
        cut_union_image $full
        scvcutview union_model enable a 0
        scvcutview union_model move b 0 1 0 -2
        scvcutview union_model visibility b 1
        set source [file join $output source_reference.png]
        cut_union_image $source
        cut_union_compare $source $full

        # A constant contour also checks opacity with ordinary alpha blending.
        # Changing transparency must visibly change it, so a blank/opaque render
        # cannot accidentally satisfy the duplicate-image assertions.
        vrenderparams -oit off
        scvcutview union_model field cell pressure 0
        set constant [file join $output constant_source.png]
        vdump $constant
        cut_assert {[dict get [scvcutimagestats $constant] colored] > 500} "Constant transparent contour absent"
        scvcutview union_model enable a 1
        scvcutview union_model move b 0 1 0 -0.3
        scvcutview union_model visibility b 7
        set constantFull [file join $output constant_all_masks.png]
        vdump $constantFull
        cut_union_compare $constant $constantFull
        foreach portion {negative on positive} { scvcutview union_model style $portion 0 }
        set opaque [file join $output opaque.png]
        vdump $opaque
        cut_assert {[dict get [scvcutimagecompare $constantFull $opaque] mean] > 5} "Transparency did not affect the contour"
    }
    scvcutview union_model clear
    puts "CUT_VIEW_UNION_PASS"
}
if {[catch {run_cut_union} message options]} {
    puts stderr "CUT_VIEW_UNION_FAIL: $message"
    puts stderr [dict get $options -errorinfo]
    exit 1
}
exit 0
