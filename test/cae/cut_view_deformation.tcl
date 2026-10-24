# Interactive acceptance: exact shear geometry/FE values, GPU/CPU images and Manager.
set deform_dir [file dirname [file normalize [info script]]]
source [file join $deform_dir viewdraw_runtime.tcl]
source [file join $deform_dir cut_view_manager.tcl]
proc deform_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc deform_pump {} { set ::deform_tick 0; after 25 {set ::deform_tick 1}; vwait ::deform_tick; update idletasks }
proc deform_capture {path} { scvcutview deform handle cut 0; vdump $path }
proc bgerror {message} { puts stderr "CUT_DEFORMATION_FAIL Tk: $message\n$::errorInfo"; exit 1 }
if {[catch {
    set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
    vinit CutDeformation -width 1000 -height 800; vsetbg -color WHITE
    scvcutview deform read [file join $deform_dir .. native fixtures deformed_hex.vtu]
    scvcutview deform add cut 1 0 0 -.57
    scvcutview deform field point temperature 0
    scvcutview deform visibility cut 3
    scvcutview deform contour 1 0 6 0 12
    scvcutview deform display
    vviewparams -proj 1 1 .8 -up 0 0 1
    scvcutmanager deform; deform_pump
    foreach mode {cloud wireframe cloud_edges} {
        .scvcutmanager.results.displaymode set $mode
        event generate .scvcutmanager.results.displaymode <<ComboboxSelected>>; deform_pump
        deform_assert {$::CutViewManager::status eq ""} "Manager mode failed"
        scvcutview deform deform none 0
        scvcutview deform fit
        set before [file join $output ${mode}_before.png]; deform_capture $before
        foreach scale {.3 -.2 0} {
            scvcutview deform deform displacement $scale
            ::CutViewManager::refresh 1
            set state [scvcutview deform deform]
            deform_assert {[dict get $state field] eq "displacement" && abs([dict get $state scale]-$scale) < 1e-9} "Live deformation query differs"
            deform_assert {$::CutViewManager::fieldLabel eq "temperature (point)" && $::CutViewManager::displaymode eq $mode} "Deformation lost cloud or mode binding"
            lassign [scvcutview deform bounds] xmin ymin zmin xmax ymax zmax
            deform_assert {abs($xmin-min(0,$scale)) < 1e-6 && abs($xmax-(1+max(0,$scale))) < 1e-6} "Deformed bounds stale"
            set initial [scvcutview deform renderinfo]
            foreach mask {2 3 6 7} {
                scvcutview deform visibility cut $mask
                scvcutview deform move cut 1 0 0 -.61
                set info [scvcutview deform renderinfo]
                deform_assert {[dict get $info backend] eq "gpu_plane" && [dict get $info body_builds] == [dict get $initial body_builds]
                    && [dict get $info submitted_body_bytes] == 0} "Warm deformed cut rebuilt body"
                # Node probe evaluates the physical FE field in current coordinates.
                set probe [scvcutview deform probe cut 1 0 1]
                lassign [dict get $probe position] x y z
                deform_assert {abs($x-.61) < 1e-6 && abs([lindex [dict get $probe values] 0]-($x+2*$y+(3-$scale)*$z)) < 1e-6} "Deformed cloud/provenance mismatch"
                set gpu [file join $output ${mode}_${scale}_${mask}_gpu.png]; deform_capture $gpu
                set pixels [scvcutimagestats $gpu]
                deform_assert {[dict get $pixels visible] > 500} "Blank deformed mode"
                deform_assert {$mode eq "wireframe" ? [dict get $pixels colored] == 0 : [dict get $pixels colored] > 500} "Deformed cloud/edges pixels wrong: $mode $scale $mask $pixels"
            }
            # Compare separately: backend changes recreate resident structures.
            foreach mask {2 3 6 7} {
                if {$mode ne "wireframe" || $mask == 2} {
                    scvcutview deform visibility cut $mask
                    set gpu [file join $output ${mode}_${scale}_${mask}_gpu.png]
                    scvcutview deform renderbackend cpu
                    set cpu [file join $output ${mode}_${scale}_${mask}_cpu.png]; deform_capture $cpu
                    set compare [scvcutimagecompare $cpu $gpu]
                    deform_assert {[dict get $compare silhouette] < 1800} "Deformed CPU/GPU silhouette differs: $mode $scale $mask $compare"
                    scvcutview deform renderbackend auto
                }
            }
            scvcutview deform visibility cut 3
            if {$scale == .3} {
                set after [file join $output ${mode}_deformed.png]; deform_capture $after
                deform_assert {[dict get [scvcutimagecompare $before $after] changed] > 500} "Deformation did not change pixels"
            }
        }
    }
    # Deformation does not depend on a scalar cloud; elemental clouds also work.
    foreach binding {{field none} {field cell pressure 0}} {
        scvcutview deform {*}$binding
        scvcutview deform deform displacement .1
        scvcutview deform move cut 1 0 0 -.63
        deform_assert {[dict get [scvcutview deform renderinfo] backend] eq "gpu_plane"} "No-field/element cloud lost deformed GPU mode"
    }
    set state [scvcutview deform deform]
    deform_assert {[catch {scvcutview deform deform temperature .1}]} "Scalar accepted as displacement"
    deform_assert {[scvcutview deform deform] eq $state} "Invalid deformation changed state"
    scvcutview deform deform none 0
    deform_assert {[dict get [scvcutview deform deform] field] eq "none"} "Deformation reset failed"
    ::CutViewManager::close; scvcutview deform clear
} message options]} { puts stderr "CUT_DEFORMATION_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_DEFORMATION_PASS
exit 0
