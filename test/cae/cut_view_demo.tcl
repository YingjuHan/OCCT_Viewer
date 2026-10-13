# Interactive demo: replace only this demo's model and keep DRAW running.
set cut_view_demo_dir [file dirname [file normalize [info script]]]
if {[llength [info commands ::CutViewManager::close]]} { ::CutViewManager::close }
source [file join $cut_view_demo_dir viewdraw_runtime.tcl]
vinit CutView -width 800 -height 600
vsetbg -color WHITE
catch {scvcutview cut_view_demo clear}
scvcutview cut_view_demo read [file normalize [file join $cut_view_demo_dir .. native fixtures tetra.vtu]]
scvcutview cut_view_demo add x 1 0 0 -0.25
scvcutview cut_view_demo rename x X-plane
scvcutview cut_view_demo add y 0 1 0 -0.5
scvcutview cut_view_demo rename y Y-plane
scvcutview cut_view_demo enable y 0
scvcutview cut_view_demo add z 0 0 1 -0.5
scvcutview cut_view_demo rename z Z-plane
scvcutview cut_view_demo enable z 0
scvcutview cut_view_demo field point temperature 0
scvcutview cut_view_demo visibility x 2
scvcutview cut_view_demo display
vaxo
scvcutview cut_view_demo fit
source [file join $cut_view_demo_dir cut_view_manager.tcl]
scvcutmanager cut_view_demo
