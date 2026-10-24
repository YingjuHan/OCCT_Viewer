# Log the actual OpenGL adapter; a Windows GPU preference alone is not evidence.
if {[catch {
    source [file join [file dirname [info script]] viewdraw_runtime.tcl]
    pload VISUALIZATION
    vinit CutGpuProbe -width 900 -height 650
    set info [vglinfo]
    puts "CUT_GPU_INFO_BEGIN\n$info\nCUT_GPU_INFO_END"
    if {[info exists ::env(CUTVIEW_EXPECTED_GPU)]
        && ![string match -nocase *$::env(CUTVIEW_EXPECTED_GPU)* $info]} {
        error "Expected OpenGL adapter $::env(CUTVIEW_EXPECTED_GPU)"
    }
} message options]} { puts stderr "CUT_GPU_PROBE_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_GPU_PROBE_PASS
exit 0
