# Single-plane resident body, display masks and opaque cap/body seam regression.
source [file join [file dirname [info script]] viewdraw_runtime.tcl]
pload MODELING VISUALIZATION
proc gpu_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
set output $::env(CUTVIEW_OUTPUT_DIR)
file mkdir $output
if {[catch {
    vinit GpuPlane -width 1000 -height 800
    vsetbg -color WHITE
    foreach offset {0 100000} {
        set fixture [file join $output cube_$offset.vtk]
        set file [open $fixture w]
        puts $file "# vtk DataFile Version 3.0\nSeam cube\nASCII\nDATASET UNSTRUCTURED_GRID\nPOINTS 8 float"
        foreach xyz {{0 0 0} {16 0 0} {16 16 0} {0 16 0} {0 0 16} {16 0 16} {16 16 16} {0 16 16}} {
            puts $file [join [lmap p $xyz {expr {$p+$offset}}]]
        }
        puts $file "CELLS 1 9\n8 0 1 2 3 4 5 6 7\nCELL_TYPES 1\n12"
        close $file
        scvcutview seam read $fixture
        scvcutview seam add p 1 0 0 [expr {-$offset-7.31}]
        scvcutview seam visibility p 3
        scvcutview seam display
        vviewparams -proj 1 1 1 -up 0 0 1
        vfit
        set initial [scvcutview seam renderinfo]
        gpu_assert {[dict get $initial backend] eq "gpu_plane"} "GPU plane backend absent"
        foreach normal {{1 0 0} {1 .37 .22} {.11 1 .43} {.5 .77 1}} {
            lassign $normal a b c
            scvcutview seam move p $a $b $c [expr {-($offset+7.31)*($a+$b+$c)}]
            set path [file join $output seam_${offset}_[lindex $normal 0]_[lindex $normal 1].png]
            vdump $path
            set pixels [scvcutimagestats $path]
            gpu_assert {[dict get $pixels visible] > 10000} "Seam fixture is empty"
            gpu_assert {[dict get $pixels interior_white] == 0} "Visible cap/body seam: $path $pixels"
            set info [scvcutview seam renderinfo]
            gpu_assert {[dict get $info body_builds] == [dict get $initial body_builds] && [dict get $info submitted_body_bytes] == 0} "Plane motion rebuilt/submitted body"
        }
        for {set mask 0} {$mask < 8} {incr mask} {
            scvcutview seam visibility p $mask
            set before [scvcutview seam renderinfo]
            scvcutview seam move p 1 0 0 [expr {-$offset-8.72}]
            set after [scvcutview seam renderinfo]
            gpu_assert {[dict get $after body_builds] == [dict get $initial body_builds]} "Mask rebuilt body"
            if {$mask == 0 || $mask == 5} {
                gpu_assert {[dict get $before slice_evaluations] == [dict get $after slice_evaluations]} "Mask evaluated hidden slice"
            }
        }
        scvcutview seam renderbackend cpu
        gpu_assert {[dict get [scvcutview seam renderinfo] fallback] eq "requested_cpu"} "CPU override ignored"
        scvcutview seam renderbackend auto
        scvcutview seam clear
    }
} message options]} { puts stderr "CUT_GPU_PLANE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_GPU_PLANE_PASS
exit 0
