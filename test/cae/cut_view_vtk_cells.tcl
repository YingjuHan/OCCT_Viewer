# Concrete VTK cells: geometric sections, all portion masks, and held pointer updates.
set vtk_cells_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $vtk_cells_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set vtk_cells_fixtures [file normalize [file join $vtk_cells_dir .. .. build test native vtk-cell-fixtures]]
if {[info exists ::env(CUTVIEW_VTK_FIXTURES)]} { set vtk_cells_fixtures $::env(CUTVIEW_VTK_FIXTURES) }
proc bgerror {message} { puts stderr "CUT_VIEW_VTK_CELLS_FAIL: $message\n$::errorInfo"; exit 1 }
proc vc_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc vc_near {actual expected message} {
    if {abs($actual-$expected) > 1.0e-4} { error "$message: $actual != $expected" }
}
proc vc_check {shape {shown 0}} {
    set result [scvcutview vtk_cells eval cut]
    if {$shown} {
        vc_assert {[dict get [scvcutview vtk_cells presentation cut] revision] == [dict get $result revision]} "Lazy evaluation concealed a stale display"
    }
    set definition [lindex [scvcutview vtk_cells list] 0]
    foreach part {negative section positive} {
        foreach value [dict get $result $part values] {
            lassign $value x y z scalar
            vc_near $scalar [expr {$x+2*$y+3*$z}] "Original-cell affine interpolation"
            if {$part ne "section"} { continue }
            if {$shape eq "plane"} {
                lassign [dict get $definition plane] a b c d
                vc_near [expr {$a*$x+$b*$y+$c*$z+$d}] 0 "Geometric plane section"
            } else {
                lassign [dict get $definition center] cx cy cz
                set squared [expr {($x-$cx)**2+($y-$cy)**2+($z-$cz)**2}]
                if {$shape eq "cylinder"} { set squared [expr {$squared-($z-$cz)**2}] }
                vc_near $squared [expr {[dict get $definition radius]**2}] "Geometric curved section"
            }
        }
    }
    return $result
}
proc vc_image {name {minimum 20}} {
    set path [file join $::vtk_cells_output $name.png]
    vdump $path
    set stats [scvcutimagestats $path]
    vc_assert {[dict get $stats colored] >= $minimum} "Blank VTK contour: $name"
    return [dict get $stats checksum]
}
proc vc_visual {type bounds} {
    lassign $bounds x0 y0 z0 x1 y1 z1
    scvcutview vtk_cells display
    scvcutview vtk_cells enable cut 0
    scvcutview vtk_cells fit
    vc_image cell_${type}_source
    scvcutview vtk_cells enable cut 1
    scvcutview vtk_cells visibility cut 2
    scvcutview vtk_cells handle cut 1 0
    scvcutview vtk_cells fit
    set previous [vc_image cell_${type}_section]
    if {$type ni {12 24 41 42 72 79}} { scvcutview vtk_cells handle cut 0; return }
    set x [expr {($x0+$x1)/2.0}]; set y [expr {($y0+$y1)/2.0}]; set z [expr {($z0+$z1)/2.0}]
    lassign [scvcutview vtk_cells project $x $y $z] px py
    scvcutview vtk_cells mouse move $px $py
    vc_assert {[dict get [scvcutview vtk_cells handleinfo] detected] eq "plane"} "Cannot pick VTK plane $type"
    scvcutview vtk_cells mouse down $px $py
    set source_work [scvcutview vtk_cells stats]
    for {set frame 1} {$frame <= 3} {incr frame} {
        set before [scvcutview vtk_cells presentation cut]
        scvcutview vtk_cells mouse move [expr {$px-2*$frame}] $py
        vc_assert {[dict get [scvcutview vtk_cells handleinfo] dragging]} "VTK drag released early"
        vc_assert {[dict get [scvcutview vtk_cells presentation cut] synchronizations] > [dict get $before synchronizations]} "No display update while VTK pointer is held"
        set work [scvcutview vtk_cells stats]
        foreach key {epoch preparations preparedCells shapeBuilds bvhBuilds} {
            vc_assert {[dict get $work $key] == [dict get $source_work $key]} "VTK held drag rebuilt source $key"
        }
        set result [vc_check plane 1]
        vc_assert {[dict get $result section nodes] > 0} "Held-drag VTK section is empty: type $type, [scvcutview vtk_cells list]"
        set next [vc_image cell_${type}_held_$frame]
        vc_assert {$next ne $previous} "VTK held-drag pixels did not change: type $type, frame $frame, [scvcutview vtk_cells list]"
        set previous $next
    }
    scvcutview vtk_cells mouse up [expr {$px-6}] $py
    vc_check plane 1
    scvcutview vtk_cells handle cut 0
}
proc run_vtk_cells {} {
    global vtk_cells_fixtures vtk_cells_output
    set visual [info exists ::env(CUTVIEW_VTK_VISUAL)]
    if {$visual} {
        set vtk_cells_output $::env(CUTVIEW_OUTPUT_DIR)
        file mkdir $vtk_cells_output
        vinit VtkCells -width 800 -height 600
        vsetbg -color WHITE
        vaxo
    }
    set count 0
    foreach path [lsort -dictionary [glob -nocomplain -directory $vtk_cells_fixtures cell_*.vtu]] {
        regexp {cell_([0-9]+)\.vtu$} $path -> type
        if {[info exists ::env(CUTVIEW_VTK_TYPES)] && $type ni $::env(CUTVIEW_VTK_TYPES)} { continue }
        scvcutview vtk_cells read $path
        if {$type == 0} { scvcutview vtk_cells clear; incr count; continue }
        set bounds [scvcutview vtk_cells bounds]
        lassign $bounds x0 y0 z0 x1 y1 z1
        set offset [expr {($x0+$x1)/2.0}]
        set cx [expr {$x0+0.25*($x1-$x0)}]
        set cy [expr {$y0+0.25*($y1-$y0)}]
        set cz [expr {$z0+0.25*($z1-$z0)}]
        foreach shape {plane sphere cylinder} {
            if {$shape eq "plane"} { scvcutview vtk_cells add cut 1 0 0 [expr {-$offset}] }
            if {$shape eq "sphere"} { scvcutview vtk_cells sphereadd cut $cx $cy $cz 0.15 }
            if {$shape eq "cylinder"} { scvcutview vtk_cells cylinderadd cut $cx $cy $cz 0 0 1 0.15 }
            scvcutview vtk_cells field point temperature 0
            foreach mask {0 1 2 3 4 5 6 7} {
                scvcutview vtk_cells visibility cut $mask
                set result [vc_check $shape]
                vc_assert {[dict get $result visible] == $mask} "Incorrect VTK portion mask"
                set index 0
                foreach part {negative section positive} {
                    if {($mask & (1 << $index)) == 0} { vc_assert {[dict get $result $part nodes] == 0} "Hidden VTK portion remains populated" }
                    incr index
                }
                if {$shape eq "plane" && ($mask & 2)} { vc_assert {[dict get $result section nodes] > 0} "Empty concrete VTK plane section $type, mask $mask, offset $offset, [scvcutview vtk_cells list]" }
            }
            scvcutview vtk_cells field cell pressure 0
            foreach part {negative section positive} {
                foreach value [dict get [scvcutview vtk_cells eval cut] $part values] { vc_near $value 7 "Original CellData mapping" }
            }
            foreach value [dict get [scvcutview vtk_cells eval cut] lineValues] { vc_near $value 7 "Original line CellData mapping" }
            scvcutview vtk_cells field point temperature 0
            if {$visual && $shape eq "plane" && $type in {1 3 5 9 12 21 24 36 41 42 72 79}} { vc_visual $type $bounds }
            scvcutview vtk_cells remove cut
        }
        scvcutview vtk_cells clear
        incr count
        puts "CUT_VIEW_VTK_CELL_PASS $type"
    }
    set expected 48
    if {[info exists ::env(CUTVIEW_VTK_TYPES)]} { set expected [llength $::env(CUTVIEW_VTK_TYPES)] }
    vc_assert {$count == $expected} "Expected $expected concrete VTK fixtures, got $count; run vtk_cell_cut_tests first"
    puts "CUT_VIEW_VTK_CELLS_PASS $count"
}
if {[catch {run_vtk_cells} message]} { puts stderr "CUT_VIEW_VTK_CELLS_FAIL: $message\n$::errorInfo"; exit 1 }
exit 0
