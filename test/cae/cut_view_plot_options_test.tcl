# Semantic/UI regression. Performance is tested separately on million-cell data.
set plot_dir [file dirname [file normalize [info script]]]
set plot_output [file normalize [file join $plot_dir .. .. build cut-plot-options]]
if {[info exists ::env(CUTVIEW_OUTPUT_DIR)]} { set plot_output $::env(CUTVIEW_OUTPUT_DIR) }
file mkdir $plot_output
proc plot_assert {condition message} { if {![uplevel 1 [list expr $condition]]} { error $message } }
proc plot_pump {} { update idletasks; update }
proc bgerror {message} { puts stderr "CUT_VIEW_PLOT_OPTIONS_FAIL $message\n$::errorInfo"; exit 1 }
proc plot_apply {} {
    .scvcutoptions.actions.apply invoke
    plot_pump
    plot_assert {$::CutViewOptions::status eq ""} "Plot options: $::CutViewOptions::status"
}
proc plot_scope {scope} {
    .scvcutoptions.scope set $scope
    event generate .scvcutoptions.scope <<ComboboxSelected>>
    plot_pump
    plot_assert {$::CutViewOptions::status eq ""} "Scope: $::CutViewOptions::status"
}
if {[catch {
    source [file join $plot_dir cut_view_demo.tcl]
    set model cut_view_demo
    set definitions [scvcutview $model snapshot]
    set cache [scvcutview $model cacheinfo]
    .scvcutmanager.options invoke
    plot_pump
    plot_assert {$::CutViewOptions::status eq "" && [scvcutview $model snapshot] eq $definitions} "Opening options changed the model"
    plot_assert {[dict get [scvcutview $model cacheinfo] resultReserved] == [dict get $cache resultReserved]} "Opening options allocated cut geometry"

    # Update only one tab; the other pending tab must remain editable.
    set ::CutViewOptions::values(2,width) 7
    .scvcutoptions.tabs select 1
    set ::CutViewOptions::values(1,transparency) .1
    set ::CutViewOptions::values(1,width) 3
    plot_apply
    plot_assert {$::CutViewOptions::values(2,width) == 7} "Applying one tab discarded another tab's edits"
    set common [scvcutview $model cutstyle defaults 1]
    plot_assert {abs([dict get $common transparency]-.1) < 1e-6 && [dict get $common width] == 3} "Common style not applied"
    plot_scope "Selected cut override"
    set ::CutViewOptions::values(1,override) 1
    set ::CutViewOptions::values(1,mode) cloud_edges
    set ::CutViewOptions::values(1,transparency) .25
    set ::CutViewOptions::values(1,r) .2
    set ::CutViewOptions::values(1,g) .4
    set ::CutViewOptions::values(1,b) .8
    plot_apply
    set custom [scvcutview $model cutstyle x 1]
    plot_assert {[dict get $custom override] && [dict get $custom mode] == 2 && abs([dict get $custom transparency]-.25) < 1e-6} "Custom style not applied"
    plot_assert {[scvcutview $model cutstyle defaults 1] eq $common && [scvcutview $model snapshot] eq $definitions} "Style changed defaults or geometry"
    foreach key {transparency width r g b} invalid {NaN -1 1.5 Inf -0.01} {
        set previous $::CutViewOptions::values(1,$key)
        set ::CutViewOptions::values(1,$key) $invalid
        .scvcutoptions.actions.apply invoke
        plot_assert {$::CutViewOptions::status ne "" && [scvcutview $model cutstyle x 1] eq $custom} "Invalid UI value changed live state: $key"
        set ::CutViewOptions::values(1,$key) $previous
    }
    foreach command {
        {cutstyle x 1 0 2 1 0.2 -1 0.8} {cutstyle x 1 0 2 99}
        {cutstyle missing 1} {cutstyle missing 1 0 2 1} {clearstyle missing 1} {cutstyle x 3}
    } {
        plot_assert {[catch {scvcutview $model {*}$command}] && [scvcutview $model cutstyle x 1] eq $custom} "Invalid command changed live style: $command"
    }

    # Copy overrides by value, then remove them even if the model is undisplayed.
    scvcutview $model enable x 0
    scvcutview $model copy x copy
    plot_assert {[scvcutview $model cutstyle copy 1] eq $custom} "Copy lost the style override"
    scvcutview $model cutstyle x 1 0.4 1 1
    plot_assert {[scvcutview $model cutstyle copy 1] eq $custom} "Copied style aliases its original"
    verase $model
    scvcutview $model remove copy
    scvcutview $model add copy 1 0 0 -.25
    scvcutview $model enable copy 0
    plot_assert {![dict get [scvcutview $model cutstyle copy 1] override]} "Recreated ID retained removed style"
    scvcutview $model remove copy
    scvcutview $model display

    # The dialog keeps its original target while tree selection changes.
    set ::CutViewManager::selected y
    ::CutViewManager::refresh 1
    set ::CutViewOptions::values(1,override) 0
    plot_apply
    plot_assert {![dict get [scvcutview $model cutstyle x 1] override] && [scvcutview $model cutstyle x 1] eq [scvcutview $model cutstyle y 1]} "Override reset or pinned owner failed"
    set beforeRename [scvcutview $model snapshot]
    # Invalid uncommitted geometry text must not prevent an independent rename.
    set ::CutViewManager::positionText nonsense
    .scvcutmanager.actions.rename invoke
    set ::CutViewRename::name {  }
    .scvcutrename.apply invoke
    plot_assert {$::CutViewRename::status ne "" && [scvcutview $model snapshot] eq $beforeRename} "Invalid rename changed geometry"
    set ::CutViewRename::name {Renamed {Y} with spaces}
    .scvcutrename.apply invoke
    foreach definition [scvcutview $model list] {
        if {[dict get $definition id] eq "y"} {
            plot_assert {[dict get $definition name] eq {Renamed {Y} with spaces} && ![dict get $definition enabled]} "Rename activated inactive cut or lost quoting"
            plot_assert {[dict get $definition plane] eq {0 1 0 -0.5}} "Rename applied invalid geometry text"
        }
    }

    # Field-free color is visible; the cloud binding must take precedence afterward.
    scvcutview $model enable x 1
    scvcutview $model field none
    scvcutview $model cutstyle x 1 0 2 1 1 0 0
    vrepaint; vdump [file join $plot_output red.png]
    scvcutview $model cutstyle x 1 0 2 1 0 0 1
    vrepaint; vdump [file join $plot_output blue.png]
    plot_assert {[dict get [scvcutimagecompare [file join $plot_output red.png] [file join $plot_output blue.png]] changed] > 500} "Face color did not change the picture"
    scvcutview $model field point temperature 0
    vrepaint; vdump [file join $plot_output cloud-blue.png]
    scvcutview $model cutstyle x 1 0 2 1 1 0 0
    vrepaint; vdump [file join $plot_output cloud-red.png]
    plot_assert {[dict get [scvcutimagecompare [file join $plot_output cloud-blue.png] [file join $plot_output cloud-red.png]] changed] < 20} "Face color overrode bound scalar field"

    # Complete sessions, unlike definitions-only JSON, retain style overrides.
    set sessionStyle [scvcutview $model cutstyle x 1]
    set bundle [file join $plot_output session]
    scvcutview $model sessionsave $bundle
    scvcutview $model cutstyle x 1 0 5 0 0 1 0
    scvcutview $model sessionload $bundle
    plot_pump
    plot_assert {[scvcutview $model cutstyle x 1] eq $sessionStyle} "Full session lost the style override or RGB"
    ::CutViewManager::refresh 1

    # Close removes dependent dialogs without changing portion visibility.
    set saved [scvcutview $model snapshot]
    .scvcutmanager.actions.rename invoke
    ::CutViewManager::close
    plot_assert {![winfo exists .scvcutoptions] && ![winfo exists .scvcutrename] && [scvcutview $model snapshot] eq $saved} "Manager Close leaked dialogs or changed definitions"
    scvcutview $model clear
} message options]} {
    puts stderr "CUT_VIEW_PLOT_OPTIONS_FAIL $message\n[dict get $options -errorinfo]"
    exit 1
}
puts "CUT_VIEW_PLOT_OPTIONS_PASS $plot_output"
exit 0
