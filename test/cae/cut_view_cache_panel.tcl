namespace eval ::CutViewCache { variable model ""; variable source 4096; variable result 16384; variable status "" }
proc ::CutViewCache::call {action} {
    variable model; variable source; variable result; variable status
    if {[catch {
        switch $action {
            apply { scvcutview $model cachepolicy $source $result }
            request { scvcutview $model request }
            cancel { scvcutview $model cancelrequest }
            info {}
        }
        set status [scvcutview $model cacheinfo]
    } message]} { set status $message }
}
proc scvcutcache {model} {
    set w .scvcutcache; if {[winfo exists $w]} { destroy $w }; set ::CutViewCache::model $model
    set info [scvcutview $model cacheinfo]
    set ::CutViewCache::source [expr {[dict get $info sourceLimit]/1048576}]
    set ::CutViewCache::result [expr {[dict get $info resultLimit]/1048576}]
    toplevel $w; wm title $w "Cut cache policy"
    ttk::label $w.sourceLabel -text "Source reservation limit (MiB)"; ttk::entry $w.source -textvariable ::CutViewCache::source
    ttk::label $w.resultLabel -text "Result reservation limit (MiB)"; ttk::entry $w.result -textvariable ::CutViewCache::result
    pack $w.sourceLabel $w.source $w.resultLabel $w.result -fill x -padx 8 -pady 3
    foreach {name title action} {apply Apply apply info {Inspect resources} info request {Begin new request} request cancel {Cancel pending request} cancel} {
        ttk::button $w.$name -text $title -command [list ::CutViewCache::call $action]; pack $w.$name -fill x -padx 8 -pady 3
    }
    ttk::label $w.status -textvariable ::CutViewCache::status -wraplength 550; pack $w.status -fill x
    ttk::button $w.close -text Close -command [list destroy $w]; pack $w.close -fill x -padx 8 -pady 3
    ::CutViewCache::call info
}
