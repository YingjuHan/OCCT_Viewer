# Interactive general-cell demo; run vtk_cell_cut_tests once to generate its fixture.
set vtk_demo_dir [file dirname [file normalize [info script]]]
source [file join $vtk_demo_dir viewdraw_runtime.tcl]
source [file join $vtk_demo_dir cut_view_manager.tcl]
set vtk_demo_file [file normalize [file join $vtk_demo_dir .. .. build test native vtk-cell-fixtures cell_79.vtu]]
if {[info exists ::vtk_cut_demo_file]} { set vtk_demo_file $::vtk_cut_demo_file }
if {![file isfile $vtk_demo_file]} { error "Missing fixture: $vtk_demo_file; run build/bind/vtk_cell_cut_tests first" }
vinit CutViewVtk -width 1000 -height 720
vsetbg -color WHITE
catch {scvcutview cut_view_vtk_demo clear}
scvcutview cut_view_vtk_demo read $vtk_demo_file
scvcutview cut_view_vtk_demo defaults
scvcutview cut_view_vtk_demo enable x 1
scvcutview cut_view_vtk_demo visibility x 2
foreach field [scvcutview cut_view_vtk_demo fields] {
    if {[dict get $field mapping] eq "point"} {
        scvcutview cut_view_vtk_demo field point [dict get $field name] 0
        break
    }
}
scvcutview cut_view_vtk_demo display
vaxo
scvcutview cut_view_vtk_demo fit
scvcutmanager cut_view_vtk_demo
