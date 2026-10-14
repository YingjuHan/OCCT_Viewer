# Run from DRAW -i with CUTVIEW_BENCHMARK_FILE, CUTVIEW_OUTPUT_DIR and optional
# CUTVIEW_BENCHMARK_FRAMES / CUTVIEW_BENCHMARK_MASK. Uses actual AIS mouse routing.
set drag_dir [file dirname [file normalize [info script]]]
proc bgerror {message} {
    puts stderr "CUT_DRAG_BENCHMARK_FAIL Tk callback: $message\n$::errorInfo"
    exit 1
}
proc drag_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc drag_plane {} {
    foreach cut [scvcutview cut_view_vtk_demo list] {
        if {[dict get $cut id] eq "x"} { return [dict get $cut plane] }
    }
    error "Missing drag plane"
}
if {[catch {
    set frames 60; set mask 2
    if {[info exists ::env(CUTVIEW_BENCHMARK_FRAMES)]} { set frames $::env(CUTVIEW_BENCHMARK_FRAMES) }
    if {[info exists ::env(CUTVIEW_BENCHMARK_MASK)]} { set mask $::env(CUTVIEW_BENCHMARK_MASK) }
    drag_assert {$frames >= 2 && $frames <= 120 && $mask in {2 3 7}} "Invalid frame count or drag mask"
    set ::vtk_cut_demo_file $::env(CUTVIEW_BENCHMARK_FILE)
    source [file join $drag_dir cut_view_vtk_demo.tcl]
    set model cut_view_vtk_demo
    file mkdir $::env(CUTVIEW_OUTPUT_DIR)
    update
    scvcutview $model visibility x $mask
    scvcutview $model handle x 1 0
    scvcutview $model fit
    lassign [scvcutview $model bounds] xmin ymin zmin xmax ymax zmax
    set x [expr {($xmin+$xmax)/2.0}]
    set y [expr {$ymin+0.35*($ymax-$ymin)}]
    set z [expr {$zmin+0.35*($zmax-$zmin)}]
    lassign [scvcutview $model project $x $y $z] px py
    scvcutview $model mouse move $px $py
    drag_assert {[dict get [scvcutview $model handleinfo] detected] eq "plane"} "Plane not picked"
    scvcutview $model mouse down $px $py
    scvcutview $model mouse move [expr {$px-5}] $py
    scvcutframewait
    drag_assert {[dict get [scvcutview $model handleinfo] dragging]} "Drag not started"
    set initial [scvcutview $model renderinfo]
    set previous [drag_plane]
    vdump [file join $::env(CUTVIEW_OUTPUT_DIR) drag-first.png]
    set csv [open [file join $::env(CUTVIEW_OUTPUT_DIR) drag.csv] w]
    puts $csv "frame,event_ms,manager_ms,total_ms,source_ms,intersection_ms,FE_ms,presentation_ms,synchronizations,submitted_body_bytes,submitted_slice_bytes,bvh_ms,node_prepare_ms,cell_cut_ms,merge_ms,candidates,fallbacks,trace_origins_ms,trace_classify_ms,trace_prepare_ms,trace_maps_ms,trace_fill_ms,trace_general_ms,interior_hits_total,interior_misses_total,interior_evictions_total"
    set timings {}; set sum 0; set managerSum 0
    for {set i 1} {$i <= $frames} {incr i} {
        set before [dict get [scvcutview $model presentation x] synchronizations]
        set start [clock microseconds]
        scvcutview $model mouse move [expr {$px-5-$i}] $py
        scvcutframewait
        set eventMs [expr {([clock microseconds]-$start)/1000.0}]
        set after [dict get [scvcutview $model presentation x] synchronizations]
        set current [drag_plane]
        drag_assert {$current ne $previous} "Pointer move did not move plane"
        drag_assert {$after == $before+1} "Pointer move synchronized more than once"
        set previous $current
        set info [scvcutview $model renderinfo]
        drag_assert {[dict get $info backend] eq "gpu_plane" && [dict get $info body_builds] == [dict get $initial body_builds] && [dict get $info submitted_body_bytes] == 0} "Drag rebuilt or uploaded the body"
        set start [clock microseconds]
        ::CutViewManager::refresh
        set managerMs [expr {([clock microseconds]-$start)/1000.0}]
        drag_assert {[dict get [scvcutview $model presentation x] synchronizations] == $after} "Manager poll recomputed slice during held drag"
        set total [expr {$eventMs+$managerMs}]
        puts $csv "$i,$eventMs,$managerMs,$total,[dict get $info source_ms],[dict get $info slice_ms],[dict get $info fields_ms],[dict get $info presentation_ms],[expr {$after-$before}],0,[dict get $info submitted_slice_bytes],[dict get $info bvh_ms],[dict get $info node_prepare_ms],[dict get $info cell_cut_ms],[dict get $info merge_ms],[dict get $info candidates],[dict get $info fallbacks],[dict get $info trace_origins_ms],[dict get $info trace_classify_ms],[dict get $info trace_prepare_ms],[dict get $info trace_maps_ms],[dict get $info trace_fill_ms],[dict get $info trace_general_ms],[dict get $info interior_hits_total],[dict get $info interior_misses_total],[dict get $info interior_evictions_total]"
        lappend timings $total; set sum [expr {$sum+$total}]; set managerSum [expr {$managerSum+$managerMs}]
    }
    close $csv
    scvcutview $model mouse up [expr {$px-5-$frames}] $py
    drag_assert {![dict get [scvcutview $model handleinfo] dragging] && [drag_plane] eq $previous} "Release changed the final plane"
    vdump [file join $::env(CUTVIEW_OUTPUT_DIR) drag-last.png]
    drag_assert {[dict get [scvcutimagestats [file join $::env(CUTVIEW_OUTPUT_DIR) drag-last.png]] colored] > 500} "Dragged contour is blank"
    drag_assert {[dict get [scvcutimagecompare [file join $::env(CUTVIEW_OUTPUT_DIR) drag-first.png] [file join $::env(CUTVIEW_OUTPUT_DIR) drag-last.png]] changed] > 500} "Drag did not change framebuffer pixels"
    # A subsequent held drag must also restore the paired display on abort.
    ::CutViewManager::refresh 1
    scvcutview $model handle x 1 0
    set x [expr {-[lindex $previous 3]/[lindex $previous 0]}]
    lassign [scvcutview $model project $x $y $z] px py
    scvcutview $model mouse move $px $py
    scvcutview $model mouse down $px $py
    scvcutview $model mouse move [expr {$px+10}] $py
    drag_assert {[drag_plane] ne $previous} "Abort probe did not move plane"
    scvcutview $model mouse abort $px $py
    foreach actual [drag_plane] expected $previous {
        drag_assert {abs($actual-$expected) < 1.0e-9} "Abort did not restore plane"
    }
    set timings [lsort -real $timings]
    puts "CUT_DRAG_BENCHMARK_RESULT frames $frames mask $mask fps [expr {1000.0*$frames/$sum}] p50_ms [lindex $timings [expr {$frames/2}]] p95_ms [lindex $timings [expr {int(ceil(.95*$frames))-1}]] manager_mean_ms [expr {$managerSum/$frames}] paired_frames 1 body_uploads 0 release_abort 1"
    ::CutViewManager::close
    scvcutview $model clear
} message options]} {
    puts stderr "CUT_DRAG_BENCHMARK_FAIL $message\n[dict get $options -errorinfo]"
    exit 1
}
puts "CUT_DRAG_BENCHMARK_PASS"
exit 0
