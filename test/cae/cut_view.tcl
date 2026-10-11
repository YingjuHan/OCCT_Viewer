# DRAW batch numerical acceptance; no graphics window is required.
set cut_view_script_dir [file dirname [file normalize [info script]]]
set ::env(CSF_VIEWDRAWDefaults) [file normalize [file join $cut_view_script_dir .. .. source-draw ViewDraw]]
pload -VIEWDRAW
set cut_view_fixtures [file normalize [file join $cut_view_script_dir .. native fixtures]]

proc cut_assert {condition message} {
    if {![uplevel 1 [list expr $condition]]} { error $message }
}
proc cut_near {actual expected message} {
    if {abs($actual - $expected) > 1.0e-6} { error "$message: $actual != $expected" }
}
proc cut_check_linear {values offset} {
    foreach value $values {
        lassign $value x y z temperature
        cut_near $x $offset "Section plane position"
        cut_near $temperature [expr {$x + 2*$y + 3*$z}] "Nodal interpolation"
    }
}
proc cut_test_encodings {} {
    global cut_view_fixtures cut_view_script_dir
    set file [open [file join $cut_view_fixtures tetra.vtu] r]
    set template [read $file]
    close $file
    set bytes [binary format d* {0 0 0 1 0 0 0 1 0 0 0 1}]
    set generated [file normalize [file join $cut_view_script_dir .. .. build cut-view-generated]]
    file mkdir $generated
    foreach compressed {0 1} {
        if {$compressed} {
            set body [zlib compress $bytes]
            set header [binary format i* [list 1 [string length $bytes] [string length $bytes] [string length $body]]]
        } else {
            set body $bytes
            set header [binary format i [string length $bytes]]
        }
        foreach encoding {binary base64 raw} {
            set xml $template
            if {$compressed} {
                set xml [string map [list {byte_order="LittleEndian"} {byte_order="LittleEndian" compressor="vtkZLibDataCompressor"}] $xml]
            }
            if {$encoding eq "raw"} { set payload $header$body } else {
                set payload [binary encode base64 -maxlen 0 $header][binary encode base64 -maxlen 0 $body]
            }
            if {$encoding eq "binary"} {
                set points "<DataArray type=\"Float64\" NumberOfComponents=\"3\" format=\"binary\">$payload</DataArray>"
            } else {
                set points {<DataArray type="Float64" NumberOfComponents="3" format="appended" offset="0"/>}
                set xml [string map [list </VTKFile> "<AppendedData encoding=\"$encoding\">_$payload</AppendedData></VTKFile>"] $xml]
            }
            regsub {<Points>.*?</Points>} $xml "<Points>$points</Points>" xml
            set path [file join $generated ${encoding}_${compressed}.vtu]
            set file [open $path wb]
            puts -nonewline $file $xml
            close $file
            scvcutview encoded read $path
            scvcutview encoded add x 1 0 0 -0.25
            scvcutview encoded field point temperature 0
            cut_check_linear [dict get [scvcutview encoded eval x] section values] 0.25
            scvcutview encoded clear
        }
    }
}
proc run_cut_view_tests {} {
    global cut_view_fixtures
    foreach extension {vtu vtk} {
        set info [scvcutview tetra read [file join $cut_view_fixtures tetra.$extension]]
        cut_assert {[dict get $info points] == 4 && [dict get $info cells] == 1} "VTK topology"
        foreach command {{scvcutview tetra displa} {scvcutview tetra display extra}} {
            cut_assert {[catch {eval $command} message]} "Malformed command accepted"
            cut_assert {[string first $command $message] >= 0} "Error does not identify the received command"
            cut_assert {[string first {help scvcutview} $message] >= 0} "Error does not point to command syntax"
        }
        scvcutview tetra add x 2 0 0 -0.5
        scvcutview tetra field point temperature 0
        for {set mask 0} {$mask < 8} {incr mask} {
            scvcutview tetra visibility x $mask
            set result [scvcutview tetra eval x]
            cut_assert {[dict get $result visible] == $mask} "Visibility mask"
            set i 0
            foreach key {negative section positive} {
                set cells [dict get $result $key cells]
                cut_assert {($cells > 0) == (($mask & (1 << $i)) != 0)} "Portion combination $mask/$key"
                incr i
            }
        }
        scvcutview tetra visibility x 7
        set result [scvcutview tetra eval x]
        cut_near [dict get $result negative volume] [expr {(1-pow(0.75,3))/6}] "Negative volume"
        cut_near [dict get $result positive volume] [expr {pow(0.75,3)/6}] "Positive volume"
        cut_near [dict get $result section area] 0.28125 "Section area"
        cut_check_linear [dict get $result section values] 0.25
        for {set frame 1} {$frame <= 9} {incr frame} {
            set offset [expr {$frame/10.0}]
            scvcutview tetra move x 1 0 0 [expr {-$offset}]
            set result [scvcutview tetra eval x]
            cut_near [dict get $result positive volume] [expr {pow(1-$offset,3)/6}] "Moving section volume"
            cut_check_linear [dict get $result section values] $offset
        }
        scvcutview tetra field cell pressure 0
        set result [scvcutview tetra eval x]
        cut_near [lindex [dict get $result section values] 0] 7 "CellData constant section"
        scvcutview tetra copy x copy
        scvcutview tetra rename copy "Second cut"
        scvcutview tetra enable copy 0
        set snapshot [scvcutview tetra snapshot]
        scvcutview tetra remove copy
        scvcutview tetra restore $snapshot
        cut_assert {[dict get [scvcutview tetra eval copy] visible] == 0} "Disabled cut"
        scvcutview tetra remove copy
        cut_assert {[catch {scvcutview tetra eval copy}]} "Removed cut still accessible"
        cut_assert {[catch {scvcutview tetra add invalid 0 0 0 0}]} "Zero normal accepted"
        scvcutview tetra move x 1 0 0 -2
        set result [scvcutview tetra eval x]
        cut_assert {[dict get $result section nodes] == 0} "Outside section"
        cut_near [dict get $result negative volume] [expr {1.0/6}] "Uncut retained volume"
        scvcutview tetra clear
    }

    scvcutview projected read [file join $cut_view_fixtures two_tetra.vtu]
    scvcutview projected add x 1 0 0 -0.25
    scvcutview projected field cell pressure 0
    scvcutview projected cellmode x projected
    foreach value [dict get [scvcutview projected eval x] section values] {
        lassign $value x y z pressure
        cut_near $pressure [expr {4-2*$z}] "Projected CellData interpolation"
    }
    scvcutview projected clear

    scvcutview surface read [file join $cut_view_fixtures surface.vtp]
    scvcutview surface add x 1 0 0 -0.25
    scvcutview surface field point temperature 0
    set result [scvcutview surface eval x]
    cut_assert {[dict get $result lines] == 1} "Surface section line"
    set values [lsort -real [dict get $result lineValues]]
    cut_near [lindex $values 0] 0.25 "Surface line start value"
    cut_near [lindex $values 1] 1.75 "Surface line end value"
    scvcutview surface field cell pressure 0
    foreach value [dict get [scvcutview surface eval x] lineValues] { cut_near $value 13 "CellData line value" }
    scvcutview surface move x 0 0 1 0
    cut_assert {[dict get [scvcutview surface eval x] section cells] == 1} "Coplanar surface cell"
    scvcutview surface clear

    set info [scvcutview series read [file join $cut_view_fixtures tetra.vtm.series]]
    cut_assert {[dict get $info fields] == 4} "Series field count"
    scvcutview series add x 1 0 0 -0.25
    foreach step {0 1} {
        scvcutview series field point temperature 0 $step
        cut_check_linear [dict get [scvcutview series eval x] section values] 0.25
    }
    scvcutview series clear

    scvcutview deformed read [file join $cut_view_fixtures deformed_tetra.vtu]
    scvcutview deformed add x 1 0 0 -0.25
    scvcutview deformed field point temperature 0
    set scale [dict get [scvcutview deformed deform displacement 1] effectiveScale]
    scvcutview deformed move x 1 0 0 [expr {-0.25-$scale}]
    set result [scvcutview deformed eval x]
    foreach value [dict get $result section values] {
        lassign $value x y z temperature
        cut_near $x [expr {0.25+$scale}] "Deformed section position"
        cut_near $temperature [expr {$x-$scale+2*$y+3*$z}] "Deformed result interpolation"
    }
    scvcutview deformed deform none 0
    scvcutview deformed move x 1 0 0 -0.25
    cut_check_linear [dict get [scvcutview deformed eval x] section values] 0.25
    scvcutview deformed clear

    scvcutview mechanics read [file join $cut_view_fixtures stress_heat.vtu]
    scvcutview mechanics add x 1 0 0 -0.25
    set integral [scvcutview mechanics freebody x stress 0 1 2 3 4 5 0 0 0]
    cut_near [dict get $integral area] 0.28125 "Integrated area"
    cut_near [lindex [dict get $integral force] 0] 0.9140625 "Integrated stress force"
    cut_near [lindex [dict get $integral moment] 1] 0.24609375 "Integrated stress moment Y"
    cut_near [lindex [dict get $integral moment] 2] -0.23291015625 "Integrated stress moment Z"
    set flow [scvcutview mechanics heatflow x heat_flux 0 1 2]
    cut_near [dict get $flow heatFlow] 0.5625 "Integrated heat flow"
    scvcutview mechanics move x -1 0 0 0.25
    set integral [scvcutview mechanics freebody x stress 0 1 2 3 4 5 0 0 0]
    cut_near [lindex [dict get $integral force] 0] -0.9140625 "Reversed plane force"
    scvcutview mechanics clear
    set info [scvcutview pieces read [file join $cut_view_fixtures multiple_piece.vtu]]
    cut_assert {[dict get $info points] == 8 && [dict get $info cells] == 2} "Multi-Piece topology"
    scvcutview pieces add x 1 0 0 -0.25
    scvcutview pieces field cell pressure 0
    set values [lsort -real [dict get [scvcutview pieces eval x] section values]]
    cut_near [lindex $values 0] 7 "First Piece CellData"
    cut_near [lindex $values 1] 11 "Second Piece CellData"
    scvcutview pieces clear
    scvcutview polyhedron read [file join $cut_view_fixtures polyhedron.vtu]
    scvcutview polyhedron add x 1 0 0 -0.25
    scvcutview polyhedron visibility x 7
    set result [scvcutview polyhedron eval x]
    cut_near [dict get $result section area] 0.28125 "Polyhedron section area"
    cut_near [dict get $result positive volume] [expr {pow(0.75,3)/6}] "Polyhedron clip volume"
    scvcutview polyhedron clear
    cut_test_encodings
    puts "CUT_VIEW_DRAW_PASS"
}

if {[catch {run_cut_view_tests} message options]} {
    puts stderr "CUT_VIEW_DRAW_FAIL: $message"
    puts stderr [dict get $options -errorinfo]
    exit 1
}
if {![info exists ::env(CUTVIEW_KEEP_RUNNING)]} { exit 0 }
