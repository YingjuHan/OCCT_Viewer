# Non-convex thin-body regression. White gaps between horizontal bands are valid;
# a convex-hull "interior_white == 0" check would incorrectly reject this mesh.
source [file join [file dirname [info script]] viewdraw_runtime.tcl]
pload MODELING VISUALIZATION
proc sine_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc sine_run {} {
    set fixtures [file normalize [file join [file dirname [info script]] .. native fixtures]]
    set output $::env(CUTVIEW_OUTPUT_DIR)
    file mkdir $output
    vinit SinePlateTest -width 1000 -height 800
    vsetbg -color WHITE
    foreach format {vtk vtu} {
        scvcutview sine_test read [file join $fixtures sine_plate_6x6.$format]
        scvcutview sine_test add cut 0 1 0 -0.5
        scvcutview sine_test field point temperature 0
        scvcutview sine_test visibility cut 7
        scvcutview sine_test display
        vviewparams -proj 1 1 0.8 -up 0 0 1
        scvcutview sine_test fit
        set index 0
        foreach plane {{0 1 0 -0.5} {1 0 0 -0.37} {1 0 0 -0.5} {0 0 1 -0.225} {0 0 1 -0.25} {-0.2 0 1 -0.12}} {
            foreach sign {1 -1} {
                lassign $plane a b c d
                scvcutview sine_test move cut [expr {$sign*$a}] [expr {$sign*$b}] [expr {$sign*$c}] [expr {$sign*$d}]
                foreach mask {0 1 2 3 4 5 6 7} {
                    scvcutview sine_test visibility cut $mask
                    set result [scvcutview sine_test eval cut]
                    if {$mask & 2} {
                        sine_assert {[dict get $result section nodes] > 0} "Empty sine section: $plane / $sign / $mask"
                        foreach value [dict get $result section values] {
                            lassign $value x y z scalar
                            sine_assert {abs($scalar-$z) < 2e-6} "Sine temperature != z"
                            sine_assert {abs($a*$x+$b*$y+$c*$z+$d) < 2e-6} "Sine section off plane"
                        }
                    }
                    scvcutview sine_test renderbackend auto
                    sine_assert {[dict get [scvcutview sine_test renderinfo] backend] eq "gpu_plane"} "GPU backend absent"
                    set gpu [file join $output ${format}_${index}_${sign}_${mask}_gpu.png]
                    vdump $gpu
                    set visible [dict get [scvcutimagestats $gpu] visible]
                    if {$mask != 0} { sine_assert {$visible > 500} "Retained sine geometry vanished: $plane / $sign / $mask" }
                    scvcutview sine_test renderbackend cpu
                    set cpu [file join $output ${format}_${index}_${sign}_${mask}_cpu.png]
                    vdump $cpu
                    set comparison [scvcutimagecompare $cpu $gpu]
                    sine_assert {[dict get $comparison silhouette] < 1000} "Sine GPU/CPU silhouette mismatch: $plane / $sign / $mask: $comparison"
                }
            }
            incr index
        }
        scvcutview sine_test clear
    }
}
if {[catch {sine_run} message options]} {
    puts stderr "CUT_SINE_PLATE_DRAW_FAIL $message\n[dict get $options -errorinfo]"
    exit 1
}
puts CUT_SINE_PLATE_DRAW_PASS
exit 0
