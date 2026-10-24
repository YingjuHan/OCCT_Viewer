set cs_dir [file dirname [file normalize [info script]]]
source [file join $cs_dir viewdraw_runtime.tcl]
source [file join $cs_dir cut_view_manager.tcl]
proc cs_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
if {[catch {
    set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
    vinit SliceCollections -width 950 -height 700
    scvcutview slices read [file join $cs_dir .. native fixtures stress_heat.vtu]
    scvcutview slices add p 1 0 0 -.1
    scvcutview slices field point heat_flux 0
    scvcutview slices display
    scvcutmanager slices; scvcuttime slices
    set ::CutViewTime::cut p; set ::CutViewTime::slicing range; set ::CutViewTime::parameters {0 .3 4}
    .scvcuttime.slicing.apply invoke
    cs_assert {$::CutViewTime::status eq ""} "Create slices: $::CutViewTime::status"
    cs_assert {[dict get [lindex [scvcutview slices slicelist] 0] count]==4} "Wrong slice count"
    .scvcuttime.collection.show invoke
    cs_assert {$::CutViewTime::status eq ""} "Show slices: $::CutViewTime::status"
    set before [scvcutview slices snapshot]
    set request {{"cut":"p","field":{"name":"stress","nodal":true,"component":0,"step":-1},"components":[0,3,4,3,1,5,4,5,2],"sumPoint":"centroid"}}
    set csv [scvcutview slices slicescan p_range $request 1]
    cs_assert {[llength [split [string trim $csv] \n]]==5 && [string first interval_units $csv]>=0} "Scan did not emit four resultant rows"
    cs_assert {[scvcutview slices snapshot] eq $before} "Scan changed scene"
    scvcutview slices sessionsave [file join $output slices.json]
    .scvcuttime.collection.remove invoke
    cs_assert {[llength [scvcutview slices list]]==1} "Remove collection touched source"
    scvcutview slices sessionload [file join $output slices.json]
    cs_assert {[dict get [lindex [scvcutview slices slicelist] 0] count]==4} "Session lost collection"
    scvcutview slices sliceremove p_range
    scvcutview slices slicing p path_regular 4 0 0 1 .1 0 0 .2 0 0 .2 .3 0
    cs_assert {[dict get [lindex [scvcutview slices slicelist] 0] count]==4} "Resampled path failed"
    ::CutViewManager::close; scvcutview slices clear
} message options]} { puts stderr "CUT_VIEW_SLICES_FAIL $message\n[dict get $options -errorinfo]"; exit 1 }
puts CUT_VIEW_SLICES_PASS
exit 0
