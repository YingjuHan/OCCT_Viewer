# N04 recursive reference staging and ordinary Viewer preservation.
set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set fixtures $::env(VTK_REFERENCE_FIXTURES)
proc reference_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
if {[catch {
    set stage [file join $fixtures draw-stage]
    scvreadmesh reference [file join $fixtures src root.vtm] -vtkstaging $stage
    set before [scvmeshinfo reference]
    foreach bad {missing.vtm cycle.vtm invalid.vtm} {
        reference_assert {[catch {scvreadmesh reference [file join $fixtures src $bad] -vtkstaging $stage}]} "DRAW staging failure accepted"
        reference_assert {[scvmeshinfo reference] eq $before} "DRAW staging failure mutated old mesh"
    }
    scvreadmesh times [file join $fixtures src remesh.vtm.series] -vtkstaging $stage
        scvreadmesh general [file join $fixtures src general.vtm] -vtkstaging $stage
    if {[info exists ::env(VTK_REFERENCE_VISUAL)]} {
        set output $::env(VTK_REFERENCE_OUTPUT)
        file mkdir $output
        vinit View1 -width 640 -height 480
        vbackground -color white
        scvreadmesh reference [file join $fixtures src root.vtm] -vtkstaging $stage -display 1
        vfit
        vrepaint
        vdump [file join $output nested.png]
        reference_assert {[dict get [scvcutimagestats [file join $output nested.png]] visible] > 500} "Staged nested ordinary Viewer image blank"
        set before [scvmeshinfo reference]
        foreach bad {missing.vtm cycle.vtm invalid.vtm} {
            reference_assert {[catch {scvreadmesh reference [file join $fixtures src $bad] -vtkstaging $stage -display 1}]} "Visual staging failure accepted"
            reference_assert {[scvmeshinfo reference] eq $before} "Visual staging failure mutated mesh"
            vrepaint
            vdump [file join $output after-$bad.png]
            reference_assert {[dict get [scvcutimagecompare [file join $output nested.png] [file join $output after-$bad.png]] changed] == 0} "Failed staging changed displayed pixels"
        }
        verase reference
        scvreadmesh times [file join $fixtures src remesh.vtm.series] -vtkstaging $stage -display 1
        vfit
        vrepaint
        vdump [file join $output series-initial.png]
        reference_assert {[dict get [scvcutimagestats [file join $output series-initial.png]] visible] > 500} "Staged series initial image blank"
            verase times
            scvreadmesh general [file join $fixtures src general.vtm] -vtkstaging $stage -display 1
            vfit
            vrepaint
            vdump [file join $output general.png]
            reference_assert {[dict get [scvcutimagestats [file join $output general.png]] visible] > 100} "General composite ordinary Viewer image blank"
            scvmeshactiveprop general -elem 0 -comp 0
            vrepaint
            vdump [file join $output general-cell.png]
            reference_assert {[dict get [scvcutimagecompare [file join $output general.png] [file join $output general-cell.png]] changed] > 50} "General composite CellData display unchanged"
    }
} reason]} {
    puts "VTK_REFERENCE_DRAW_FAIL: $reason"
    exit 1
}
puts "VTK_REFERENCE_DRAW_PASS"
exit 0
