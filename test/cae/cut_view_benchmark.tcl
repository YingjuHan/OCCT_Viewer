set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload MODELING VISUALIZATION
pload -VIEWDRAW
if {[catch {
    vinit CutBenchmark -width 900 -height 650; vsetbg -color WHITE; vaxo
    if {[info exists ::env(CUTVIEW_BENCHMARK_LIFECYCLE)]} {
        set baseline 0; set maximum 0
        for {set cycle 0} {$cycle<60} {incr cycle} {
            scvcutbenchmark 10 5 rotating $::env(CUTVIEW_OUTPUT_DIR)
            set bytes [dict get [scvcutmemory] private_bytes]
            if {$cycle==9} { set baseline $bytes }
            if {$cycle>=10} { set maximum [expr {max($maximum,$bytes)}] }
        }
        if {$maximum-$baseline>33554432} { error "Native long-session private-memory growth: $baseline -> $maximum" }
        puts "CUT_LIFECYCLE_MEMORY baseline $baseline peak $maximum cycles 60 frames 300"
    }
    set failures {}
    set mask 2; set backend auto
    if {[info exists ::env(CUTVIEW_BENCHMARK_MASK)]} { set mask $::env(CUTVIEW_BENCHMARK_MASK) }
    if {[info exists ::env(CUTVIEW_BENCHMARK_BACKEND)]} { set backend $::env(CUTVIEW_BENCHMARK_BACKEND) }
    set scenarios {crossing rotating deformed}
    if {[info exists ::env(CUTVIEW_BENCHMARK_SCENARIOS)]} { set scenarios $::env(CUTVIEW_BENCHMARK_SCENARIOS) }
    foreach mode $scenarios {
        if {[info exists ::env(CUTVIEW_BENCHMARK_FILE)]} {
            set result [scvcutfilebenchmark $::env(CUTVIEW_BENCHMARK_FILE) $::env(CUTVIEW_BENCHMARK_FRAMES) $mode $::env(CUTVIEW_OUTPUT_DIR) $mask $backend]
        } else {
            set result [scvcutbenchmark $::env(CUTVIEW_BENCHMARK_GRID) $::env(CUTVIEW_BENCHMARK_FRAMES) $mode $::env(CUTVIEW_OUTPUT_DIR) $mask $backend]
        }
        puts "CUT_BENCHMARK_RESULT $result"
        if {![dict get $result actual_redraw] || ![dict get $result original_FE]} { error "Missing full-frame measurement" }
        if {$::env(CUTVIEW_BENCHMARK_GRID)>=100 && ![info exists ::env(CUTVIEW_BENCHMARK_FILE)] && ![dict get $result target]} { lappend failures $result }
    }
    if {[llength $failures]} { error "Million-cell full-frame target failed: $failures" }
} message options]} { puts stderr "CUT_BENCHMARK_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts "CUT_BENCHMARK_PASS"
exit 0
