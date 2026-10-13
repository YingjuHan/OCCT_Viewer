# Interactive independent-field isosurface demo; keeps DRAW running.
set cut_view_iso_demo_dir [file dirname [file normalize [info script]]]
if {[llength [info commands ::CutViewManager::close]]} { ::CutViewManager::close }
source [file join $cut_view_iso_demo_dir viewdraw_runtime.tcl]
vinit IsoCutView -width 800 -height 600
vsetbg -color WHITE
catch {scvcutview cut_view_iso_demo clear}
scvcutview cut_view_iso_demo read [file normalize [file join $cut_view_iso_demo_dir .. native fixtures iso_tetra.vtu]]
scvcutview cut_view_iso_demo isoadd threshold point cut_x 0 0.25
scvcutview cut_view_iso_demo rename threshold "cut_x = 0.25"
scvcutview cut_view_iso_demo visibility threshold 3
scvcutview cut_view_iso_demo field point temperature 0
scvcutview cut_view_iso_demo display
vaxo
scvcutview cut_view_iso_demo fit
source [file join $cut_view_iso_demo_dir cut_view_manager.tcl]
scvcutmanager cut_view_iso_demo
