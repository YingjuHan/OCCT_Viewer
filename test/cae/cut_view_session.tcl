namespace eval ::CutViewSession {
    variable model ""
    variable w .scvcutsession
    variable file ""
    variable cad ""
    variable status ""
    variable timer ""
}
proc ::CutViewSession::destroyed {widget} {
    variable w; variable timer
    if {$widget eq $w && $timer ne ""} { after cancel $timer; set timer "" }
}
proc ::CutViewSession::watch {} {
    variable w; variable model; variable timer; set timer ""
    if {![winfo exists $w]} { return }
    if {[catch {scvcutview $model info}]} { destroy $w; return }
    set timer [after 100 ::CutViewSession::watch]
}
proc ::CutViewSession::browseCad {} {
    variable w; variable cad
    set chosen [tk_getOpenFile -parent $w -filetypes {{{CAD BREP} {.brep}} {{All files} *}}]
    if {$chosen ne ""} { set cad $chosen }
}
proc ::CutViewSession::perform {action} {
    variable w; variable model; variable file; variable cad; variable status
    set types {{{Native Cut Session} {.json}} {{All files} *}}
    if {$action eq "save"} {
        set chosen [tk_getSaveFile -parent $w -defaultextension .json -filetypes $types -initialfile [file tail $file]]
    } else { set chosen [tk_getOpenFile -parent $w -filetypes $types] }
    if {$chosen eq ""} { return }
    set file $chosen
    if {[catch {
        if {$action eq "save"} { scvcutview $model sessionsave $file $cad } else {
            scvcutview $model sessionload $file
            set cad [dict get [scvcutview $model sessioninfo] cad]
            if {[winfo exists .scvcutmanager] && $::CutViewManager::model eq $model} {
                ::CutViewManager::refreshFields
                ::CutViewManager::refresh
            }
        }
    } message]} { set status $message; return }
    set status [expr {$action eq "save" ? "Session saved. Keep the JSON and its assets folder together." : "Session restored."}]
}
proc scvcutsession {model} {
    set w .scvcutsession
    if {[winfo exists $w]} { destroy $w }
    set ::CutViewSession::model $model
    set ::CutViewSession::cad [dict get [scvcutview $model sessioninfo] cad]
    set ::CutViewSession::status ""
    toplevel $w; wm title $w "Native Cut Session"
    bind $w <Destroy> [list ::CutViewSession::destroyed %W]
    ttk::label $w.filelabel -text "Session file"
    ttk::entry $w.file -textvariable ::CutViewSession::file -width 65
    ttk::label $w.cadlabel -text "CAD source file (for a CAD section)"
    ttk::entry $w.cad -textvariable ::CutViewSession::cad -width 65
    ttk::button $w.browse -text "Browse CAD source" -command ::CutViewSession::browseCad
    ttk::button $w.save -text "Save session…" -command {::CutViewSession::perform save}
    ttk::button $w.load -text "Restore session…" -command {::CutViewSession::perform load}
    ttk::label $w.status -textvariable ::CutViewSession::status -wraplength 520
    ttk::button $w.close -text Close -command [list destroy $w]
    pack $w.filelabel $w.file $w.cadlabel $w.cad $w.browse $w.save $w.load $w.status $w.close -fill x -padx 8 -pady 4
    ::CutViewSession::watch
}
