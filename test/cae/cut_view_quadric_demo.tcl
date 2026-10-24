# Native analytic curved-cut demo; keeps DRAW running and can be sourced again.
set curved_demo_dir [file dirname [file normalize [info script]]]
if {[llength [info commands ::CutViewManager::close]]} { ::CutViewManager::close }
source [file join $curved_demo_dir viewdraw_runtime.tcl]
vinit CurvedCutView -width 800 -height 600
vsetbg -color WHITE
catch {scvcutview cut_view_curved_demo clear}
scvcutview cut_view_curved_demo read [file normalize [file join $curved_demo_dir .. native fixtures tetra.vtu]]
scvcutview cut_view_curved_demo sphereadd sphere 0.2 0.2 0.2 0.12
scvcutview cut_view_curved_demo rename sphere "Sphere"
scvcutview cut_view_curved_demo cylinderadd cylinder 0.2 0.2 0.2 0 0 1 0.12
scvcutview cut_view_curved_demo rename cylinder "Cylinder"
scvcutview cut_view_curved_demo enable cylinder 0
scvcutview cut_view_curved_demo field point temperature 0
scvcutview cut_view_curved_demo display
vaxo
scvcutview cut_view_curved_demo fit
source [file join $curved_demo_dir cut_view_manager.tcl]
scvcutmanager cut_view_curved_demo
set ::CutViewManager::selected sphere
::CutViewManager::refresh 1
