# Optional: set ::cut_deformation_file to a VTK/VTU with nodal displacement.
set deform_dir [file dirname [file normalize [info script]]]
set ::cut_modes_file [file normalize [file join $deform_dir .. native fixtures deformed_hex.vtu]]
if {[info exists ::cut_deformation_file]} { set ::cut_modes_file $::cut_deformation_file }
source [file join $deform_dir cut_view_display_modes_demo.tcl]
if {![info exists ::cut_deformation_scale]} { set ::cut_deformation_scale .3 }
# Apply after display to exercise live source/BVH/body/section invalidation.
scvcutview cut_modes deform displacement $::cut_deformation_scale
scvcutview cut_modes fit
::CutViewManager::refresh 1
puts {Deformed geometry, temperature cloud and mesh edges share the same cut plane.}
puts {scvcutview cut_modes deform displacement 0.1}
puts {scvcutview cut_modes deform none 0}
puts {scvcutview cut_modes deform}
puts [scvcutview cut_modes deform]
