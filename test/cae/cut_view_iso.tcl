# Native scalar-cut regression. CUTVIEW_ISO_VISUAL=1 enables pixels and Tk controls.
set ::env(CUTVIEW_KEEP_RUNNING) 1
source [file join [file dirname [file normalize [info script]]] cut_view.tcl]
proc bgerror {message} {
    puts stderr "CUT_VIEW_ISO_FAIL: Tk callback: $message"
    puts stderr $::errorInfo
    exit 1
}

proc run_cut_view_iso {} {
    global cut_view_fixtures cut_view_script_dir
    scvcutview iso read [file join $cut_view_fixtures iso_tetra.vtu]
    scvcutview iso isoadd threshold point cut_x 0 0.25
    scvcutview iso field point temperature 0
    for {set mask 0} {$mask < 8} {incr mask} {
        scvcutview iso visibility threshold $mask
        set result [scvcutview iso eval threshold]
        cut_assert {[dict get $result visible] == $mask} "Iso visibility"
        set bit 1
        foreach portion {negative section positive} {
            cut_assert {([dict get $result $portion cells] > 0) == (($mask & $bit) != 0)} "Iso portions $mask/$portion"
            set bit [expr {$bit << 1}]
        }
    }
    scvcutview iso visibility threshold 7
    set result [scvcutview iso eval threshold]
    cut_near [dict get $result positive volume] [expr {pow(0.75,3)/6}] "Iso upper volume"
    cut_near [dict get $result negative volume] [expr {(1-pow(0.75,3))/6}] "Iso lower volume"
    cut_near [dict get $result section area] 0.28125 "Iso section area"
    cut_check_linear [dict get $result section values] 0.25
    cut_assert {[scvcutview iso range threshold] eq {0 1}} "Iso field range"
    cut_assert {[catch {scvcutview iso move threshold 1 0 0 -0.5}]} "Plane movement accepted for iso"
    cut_assert {[catch {scvcutview iso rotate threshold 1 30}]} "Plane rotation accepted for iso"
    cut_assert {[catch {scvcutview iso isoadd invalid point missing 0 0.5}]} "Unknown iso field accepted"
    cut_assert {[catch {scvcutview iso isoadd invalid point cut_x 9 0.5}]} "Invalid iso component accepted"
    for {set frame 1} {$frame <= 9} {incr frame} {
        set value [expr {$frame/10.0}]
        scvcutview iso isovalue threshold $value
        set result [scvcutview iso eval threshold]
        cut_check_linear [dict get $result section values] $value
        cut_near [dict get $result positive volume] [expr {pow(1-$value,3)/6}] "Live iso threshold"
    }
    scvcutview iso isovalue threshold 0.25
    scvcutview iso field cell pressure 0
    cut_near [lindex [dict get [scvcutview iso eval threshold] section values] 0] 7 "Iso CellData"
    scvcutview iso cellmode threshold projected
    foreach point [dict get [scvcutview iso eval threshold] section values] {
        cut_near [lindex $point 0] 0.25 "Display field moved iso"
        cut_near [lindex $point 3] 7 "Projected iso contour"
    }
    scvcutview iso copy threshold copied
    scvcutview iso enable copied 0
    set snapshot [scvcutview iso snapshot]
    scvcutview iso remove copied
    scvcutview iso restore $snapshot
    cut_assert {[scvcutview iso snapshot] eq $snapshot} "Iso snapshot round trip"
    cut_assert {[catch {scvcutview iso restore [string map {cut_x missing} $snapshot]}]} "Missing binding restored"
    cut_assert {[scvcutview iso snapshot] eq $snapshot} "Invalid restore changed cuts"
    set legacy {{"version":1,"cuts":[{"id":"legacy","name":"Legacy X","plane":[1,0,0,-0.25],"visibility":2,"enabled":true,"cellMode":"constant"}]}}
    scvcutview iso restore $legacy
    cut_assert {[dict get [lindex [scvcutview iso list] 0] shape] eq "plane"} "Legacy plane migration"
    scvcutview iso restore $snapshot
    scvcutview iso remove copied

    scvcutview surface_iso read [file join $cut_view_fixtures surface.vtp]
    scvcutview surface_iso isoadd threshold point temperature 0 0.5
    scvcutview surface_iso field point temperature 0
    set result [scvcutview surface_iso eval threshold]
    cut_assert {[dict get $result lines] == 1} "Surface iso line missing"
    foreach value [dict get $result lineValues] { cut_near $value 0.5 "Surface iso interpolation" }
    scvcutview surface_iso field cell pressure 0
    foreach value [dict get [scvcutview surface_iso eval threshold] lineValues] { cut_near $value 13 "Surface iso CellData" }
    scvcutview surface_iso clear

    if {[info exists ::env(CUTVIEW_ISO_VISUAL)] && $::env(CUTVIEW_ISO_VISUAL)} {
        pload VISUALIZATION
        vinit IsoCutView -width 800 -height 600
        vsetbg -color WHITE
        scvcutview iso field point temperature 0
        scvcutview iso display
        vaxo
        scvcutview iso fit
        set output [file join $cut_view_script_dir .. .. build cut-view-iso-images]
        if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set output $::env(CUTVIEW_OUTPUT_DIR) }
        file mkdir $output
        for {set mask 0} {$mask < 8} {incr mask} {
            scvcutview iso visibility threshold $mask
            set path [file join $output iso_$mask.png]
            vdump $path
            set stats [scvcutimagestats $path]
            if {$mask == 0} { cut_assert {[dict get $stats visible] == 0} "Hidden iso is not blank" } else {
                cut_assert {[dict get $stats colored] > 500 && [dict get $stats colors] > 20} "Iso contour is blank"
            }
        }
        scvcutview iso visibility threshold 3
        source [file join $cut_view_script_dir cut_view_manager.tcl]
        scvcutmanager iso
        update idletasks
        cut_assert {$::CutViewManager::shape eq "isosurface"} "Iso selection unavailable"
        cut_assert {$::CutViewManager::motion eq "Value"} "Iso uses plane motion"
        cut_assert {[dict get [scvcutview iso handleinfo] id] eq ""} "Iso has misleading plane guide"
        .scvcutmanager.controls.translate set 0.5
        update
        cut_near [dict get [scvcutview iso eval threshold] section area] 0.125 "Manager threshold movement"
        set path [file join $output iso_threshold_05.png]
        vdump $path
        cut_assert {[dict get [scvcutimagestats $path] colored] > 500} "Moving iso contour is blank"
        set ::CutViewManager::name "Temperature slice"
        set ::CutViewManager::positionText 0.25
        ::CutViewManager::apply
        cut_assert {[dict get [lindex [scvcutview iso list] 0] name] eq "Temperature slice"} "Iso manager rename"
        set ::CutViewManager::fieldLabel "pressure (cell)"
        ::CutViewManager::field
        cut_near [dict get [scvcutview iso eval threshold] section area] 0.28125 "Manager contour changed cut binding"
        set ::CutViewManager::isoFieldLabel "temperature (point)"
        ::CutViewManager::isoBinding
        cut_near [dict get [scvcutview iso eval threshold] section area] [expr {sqrt(14)*0.25*0.25/12}] "Manager cut field binding"
        set ::CutViewManager::isoFieldLabel "cut_x (point)"
        ::CutViewManager::isoBinding
        update idletasks
        cut_assert {[winfo ismapped .scvcutmanager.controls.isofield] && [winfo ismapped .scvcutmanager.controls.isocomponent]} \
            "Isosurface field controls disappeared after refresh"
        if {[auto_execok import] ne ""} {
            update idletasks
            exec import -window [winfo id .scvcutmanager] [file join $output iso_manager_panel.png]
        }
        set ::CutViewManager::createShape Isosurface
        ::CutViewManager::create
        cut_assert {[llength [scvcutview iso list]] == 2} "Manager iso create"
        ::CutViewManager::create 1
        cut_assert {[llength [scvcutview iso list]] == 3} "Manager iso copy"
        ::CutViewManager::remove
        scvcutview iso add plane 1 0 0 -0.4
        set ::CutViewManager::selected plane
        ::CutViewManager::refresh 1
        update idletasks
        cut_assert {[winfo ismapped .scvcutmanager.controls.nx] && ![winfo ismapped .scvcutmanager.controls.isofield]} \
            "Switching to a plane did not restore plane controls"
        cut_assert {[dict get [scvcutview iso handleinfo] id] eq "plane"} "Plane guide not restored"
        set ::CutViewManager::selected threshold
        ::CutViewManager::refresh 1
        update idletasks
        cut_assert {[winfo ismapped .scvcutmanager.controls.isofield] && ![winfo ismapped .scvcutmanager.controls.nx]} \
            "Switching back to iso did not restore field controls"
        cut_assert {[dict get [scvcutview iso handleinfo] id] eq ""} "Plane guide leaked into iso selection"
        ::CutViewManager::close
        # Save the original independent-field contour after UI lifecycle checks.
        scvcutview iso restore $snapshot
        scvcutview iso remove copied
        scvcutview iso field point temperature 0
        vdump [file join $output iso_manager_result.png]
        source [file join $cut_view_script_dir cut_view_iso_demo.tcl]
        scvcutview cut_view_iso_demo isovalue threshold 0.5
        source [file join $cut_view_script_dir cut_view_iso_demo.tcl]
        cut_near [dict get [scvcutview cut_view_iso_demo eval threshold] section area] 0.28125 "Repeated iso demo reset"
        ::CutViewManager::close
        scvcutview cut_view_iso_demo clear
    }
    scvcutview iso clear
    puts "CUT_VIEW_ISO_PASS"
}

if {[catch {run_cut_view_iso} message options]} {
    puts stderr "CUT_VIEW_ISO_FAIL: $message"
    puts stderr [dict get $options -errorinfo]
    exit 1
}
exit 0
