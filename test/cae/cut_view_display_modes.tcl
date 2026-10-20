# Render real line primitives, mode/mask switches, field changes and empty cuts.
source [file join [file dirname [info script]] viewdraw_runtime.tcl]
proc mode_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
set output $::env(CUTVIEW_OUTPUT_DIR)
file mkdir $output
if {[catch {
    vinit CutModes -width 1000 -height 800
    vsetbg -color WHITE
    scvcutview modes read [file join [file dirname [info script]] .. native fixtures sine_plate_6x6.vtu]
    scvcutview modes add cut 0 1 0 -0.43
    scvcutview modes field point temperature 0
    scvcutview modes visibility cut 7
    scvcutview modes display
    scvcutview modes contour 1 0 1 0 12
    vviewparams -proj 1 1 .8 -up 0 0 1
    scvcutview modes fit
    foreach mode {cloud wireframe cloud_edges cloud wireframe} {
        scvcutview modes displaymode $mode
        mode_assert {[scvcutview modes displaymode] eq $mode} "Mode query differs"
        scvcutview modes visibility cut 3
        set initial [scvcutview modes renderinfo]
        mode_assert {[dict get $initial backend] eq "gpu_plane"} "Mode fell back: $mode $initial"
        foreach mask {0 1 2 3 4 5 6 7} {
            scvcutview modes visibility cut $mask
            scvcutview modes move cut 0 1 0 -0.47
            set stats [scvcutview modes renderinfo]
            mode_assert {[dict get $stats body_builds] == [dict get $initial body_builds]
                && [dict get $stats submitted_body_bytes] == 0} "Warm mode/mask edit uploaded body: $mode $mask"
            set gpu [file join $output ${mode}_${mask}_gpu.png]
            vdump $gpu
            set pixels [scvcutimagestats $gpu]
            if {$mask == 0} {
                mode_assert {[dict get $pixels visible] == 0} "Hidden mode retained geometry: $mode"
            } else {
                mode_assert {[dict get $pixels visible] > 300} "Blank mode: $mode $mask"
                if {$mode eq "wireframe"} {
                    mode_assert {[dict get $pixels colored] == 0} "Wireframe unexpectedly contains cloud fill"
                } else { mode_assert {[dict get $pixels colored] > 300} "Cloud lost field coloring" }
            }
        }
        # Backend comparison separate from resident-cache assertions.
        foreach mask {2 3 5 7} {
            # CPU body union exposes tessellation edges; GPU preserves original
            # FE boundary edges. Only the standalone section has identical lines.
            if {$mode eq "wireframe" && $mask != 2} { continue }
            scvcutview modes visibility cut $mask
            set gpu [file join $output ${mode}_${mask}_gpu.png]
            set cpu [file join $output ${mode}_${mask}_cpu.png]
            scvcutview modes renderbackend cpu
            vdump $cpu
            set comparison [scvcutimagecompare $cpu $gpu]
            mode_assert {[dict get $comparison silhouette] < 1800} "CPU/GPU silhouette differs: $mode $mask $comparison"
            scvcutview modes renderbackend auto
        }
        foreach pair {{1 3} {4 6}} {
            lassign $pair a b
            set comparison [scvcutimagecompare [file join $output ${mode}_${a}_gpu.png] [file join $output ${mode}_${b}_gpu.png]]
            mode_assert {[dict get $comparison changed] == 0} "Implicit cap differs from explicit section: $mode $pair"
        }
        scvcutview modes visibility cut 2
        scvcutview modes move cut 0 1 0 -2
        set empty [file join $output ${mode}_empty.png]
        vdump $empty
        mode_assert {[dict get [scvcutimagestats $empty] visible] == 0} "Empty slice left stale primitives"
        scvcutview modes move cut 0 1 0 -0.43
        scvcutview modes field none
        scvcutview modes field point temperature 0
    }
    mode_assert {[catch {scvcutview modes displaymode invalid}]} "Invalid mode accepted"
    scvcutview modes clear
} message options]} { puts stderr "CUT_DISPLAY_MODES_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_DISPLAY_MODES_PASS
exit 0
