set script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $script_dir .. .. source-draw ViewDraw]]
pload MODELING VISUALIZATION
pload -VIEWDRAW
set output $::env(CUTVIEW_OUTPUT_DIR); file mkdir $output
set covered {}
proc session_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc session_pump {} { set ::session_tick 0; after 30 {set ::session_tick 1}; vwait ::session_tick; update idletasks }
proc session_click {widget} {
    if {$widget ni $::covered} { lappend ::covered $widget }
    session_assert {[winfo ismapped $widget]} "Unmapped session widget $widget"
    event generate $widget <Enter>; event generate $widget <ButtonPress-1> -x 5 -y 5; event generate $widget <ButtonRelease-1> -x 5 -y 5
    session_pump
}
proc session_entry {widget value} { if {$widget ni $::covered} { lappend ::covered $widget }; $widget delete 0 end; $widget insert 0 $value; session_pump }
proc session_inventory {parent} {
    set result {}
    foreach child [winfo children $parent] {
        if {[winfo class $child] in {TButton TEntry TCombobox TCheckbutton}} { lappend result $child }
        lappend result {*}[session_inventory $child]
    }; return $result
}
proc session_dialog_complete {path cancel} {
    set dialog .scvcutsession.__tk_filedialog
    if {![winfo exists $dialog] || ![winfo ismapped $dialog]} { after 25 [list session_dialog_complete $path $cancel]; return }
    set ::session_dialog_seen 1
    if {$cancel} { $dialog.contents.f2.cancel invoke } else {
        $dialog.contents.f2.ent delete 0 end; $dialog.contents.f2.ent insert 0 $path
        $dialog.contents.f2.ok invoke
    }
}
proc session_dialog {button path {cancel 0}} {
    set ::session_dialog_seen 0; after 80 [list session_dialog_complete $path $cancel]
    session_click $button
    session_assert {$::session_dialog_seen} "File dialog was not exercised"
}
proc session_pixels {suffix} {
    set file [file join $::output $suffix.png]; vdump $file; return [dict get [scvcutimagestats $file] checksum]
}
proc bgerror {message} { puts stderr "CUT_VIEW_SESSION_FAIL $message\n$::errorInfo"; exit 1 }
proc session_run {} {
    global script_dir output covered
    vinit NativeSession -width 800 -height 600; vsetbg -color WHITE
    scvcutview session read [file join $script_dir .. native fixtures stress_heat.vtu]
    scvcutview session add p 1 0 0 -0.25
    scvcutview session field point heat_flux 0
    scvcutview session display
    vaxo; scvcutview session fit
    box session_cad 1 1 1
    set cad [file join $output source.brep]; save session_cad $cad
    scvcutview session cadsection session_cad p 1
    source [file join $script_dir cut_view_session.tcl]
    scvcutsession session
    if {$::tcl_platform(platform) eq "windows"} {
        source [file join $::tk_library tkfbox.tcl]
        rename ::tk_getOpenFile ::session_native_getOpenFile
        rename ::tk_getSaveFile ::session_native_getSaveFile
        proc ::tk_getOpenFile {args} { return [::tk::dialog::file:: open {*}$args] }
        proc ::tk_getSaveFile {args} { return [::tk::dialog::file:: save {*}$args] }
        puts "SESSION_DIALOG_BACKEND Tk (Windows shell dialogs excluded)"
    }
    session_pump; set expected [session_inventory .scvcutsession]
    set file [file join $output native.json]
    # Use a fresh path for the save dialog; repeated CTest executions retain assets.
    set file [file join $output native-[clock clicks].json]
    session_entry .scvcutsession.file $file
    session_entry .scvcutsession.cad ""
    session_dialog .scvcutsession.browse $cad
    session_assert {$::CutViewSession::cad eq $cad} "CAD browse binding lost"
    set before [session_pixels before]
    session_dialog .scvcutsession.save $file
    session_assert {[file exists $file] && [string match "Session saved.*" $::CutViewSession::status]} "Session save failed: $::CutViewSession::status"
    scvcutview session move p 1 0 0 -0.15
    session_assert {[session_pixels changed] != $before} "Fixture edit did not change pixels"
    session_dialog .scvcutsession.load $file
    session_assert {$::CutViewSession::status eq "Session restored."} "Session restore failed: $::CutViewSession::status"
    session_assert {[session_pixels restored] == $before} "Restored camera/fields/CAD pixels changed"
    set generation [scvcutview session nativeinfo]; set definitions [scvcutview session list]
    set invalid [file join $output invalid.json]; set f [open $invalid w]; puts $f "\{"; close $f
    session_dialog .scvcutsession.load $invalid
    session_assert {$::CutViewSession::status ne "Session restored."} "Invalid session accepted"
    session_assert {[scvcutview session nativeinfo] eq $generation && [scvcutview session list] eq $definitions && [session_pixels rejected] == $before} "Failed restore changed live state/pixels"
    set status $::CutViewSession::status
    session_dialog .scvcutsession.load $file 1
    session_dialog .scvcutsession.save $file 1
    session_dialog .scvcutsession.browse $cad 1
    session_assert {$::CutViewSession::status eq $status && [session_pixels cancelled] == $before} "Cancelled dialog changed live state"
    session_click .scvcutsession.close
    foreach widget $expected { session_assert {$widget in $covered} "Untested session control $widget" }
    puts "CUT_VIEW_SESSION_COVERAGE [llength $covered]/[llength $expected]"
    scvcutsession session
    scvcutview session clear
    for {set i 0} {$i<5} {incr i} { session_pump }
    session_assert {![winfo exists .scvcutsession]} "Model close left a stale session panel"
    vclose ALL
    puts "CUT_VIEW_SESSION_PASS"; exit 0
}
after 100 {if {[catch {session_run} message options]} {puts stderr "CUT_VIEW_SESSION_FAIL $message\n[dict get $options -errorinfo]"; exit 1}}
