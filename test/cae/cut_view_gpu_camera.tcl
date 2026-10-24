# Diagnostic only: retain camera state and screenshots; no cross-GPU PASS claim.
if {[catch {
    source [file join [file dirname [info script]] viewdraw_runtime.tcl]
    pload MODELING VISUALIZATION
    vinit CutBenchmark -width 900 -height 650
    vsetbg -color WHITE
    vaxo
    foreach mode {crossing rotating deformed} {
        set result [scvcutfilebenchmark $::env(CUTVIEW_BENCHMARK_FILE) $::env(CUTVIEW_BENCHMARK_FRAMES) $mode $::env(CUTVIEW_OUTPUT_DIR) 3 auto 0.1]
        puts "CUT_GPU_CAMERA_RESULT $result"
        foreach query {{vcamera} {vviewparams}} {
            set info [uplevel #0 $query]
            puts "CUT_CAMERA_INFO $mode $query: $info"
        }
    }
} message options]} { puts stderr "CUT_GPU_CAMERA_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_GPU_CAMERA_PASS
exit 0
