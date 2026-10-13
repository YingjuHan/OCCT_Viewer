# Initialize the DLL search path in an already-running Windows DRAW session.
namespace eval ::ViewDrawRuntime {
    variable repository [file normalize [file join [file dirname [info script]] .. ..]]

    proc load {} {
        variable repository
        variable runtimeFile
        variable libraryFile
        variable libraryDirs
        set ::env(CSF_VIEWDRAWDefaults) [file join $repository source-draw ViewDraw]
        if {[llength [info commands scvcutview]]} { return }

        if {$::tcl_platform(platform) eq "windows"} {
            set runtimeFile ""
            if {[info exists ::env(OCCTV_DRAW_RUNTIME)]} {
                set runtimeFile [file normalize $::env(OCCTV_DRAW_RUNTIME)]
                if {![file isfile $runtimeFile]} {
                    error "OCCTV_DRAW_RUNTIME does not exist: $runtimeFile"
                }
            } else {
                # Prefer the current build and existing PATH before standard builds.
                set candidates [list [pwd]]
                if {[info exists ::env(PATH)]} {
                    set candidates [concat $candidates [split $::env(PATH) ";"]]
                }
                foreach build [list [file join $repository build mingw] [file join $repository build]] {
                    foreach bin {bind bin} { lappend candidates [file join $build $bin] }
                }
                foreach directory $candidates {
                    if {$directory eq ""} { continue }
                    set candidate [file join $directory viewdraw-runtime.tcl]
                    if {[file isfile $candidate] && ([file isfile [file join $directory libViewDraw.dll]] ||
                                                    [file isfile [file join $directory ViewDraw.dll]])} {
                        set runtimeFile [file normalize $candidate]
                        break
                    }
                }
            }
            if {$runtimeFile ne ""} {
                unset -nocomplain libraryFile libraryDirs
                source $runtimeFile
                if {![info exists libraryFile] || ![info exists libraryDirs] || ![llength $libraryDirs]} {
                    error "Invalid VIEWDRAW runtime file: $runtimeFile"
                }
                if {![file isfile $libraryFile]} {
                    error "ViewDraw is not built for $runtimeFile; run cmake --build on that build directory --target ViewDraw"
                }
                set paths $libraryDirs
                if {[info exists ::env(PATH)]} {
                    foreach directory [split $::env(PATH) ";"] {
                        if {[lsearch -nocase -exact $paths $directory] < 0} { lappend paths $directory }
                    }
                }
                set ::env(PATH) [join $paths ";"]
            }
        }

        if {[catch {pload -VIEWDRAW} reason options]} {
            if {$::tcl_platform(platform) eq "windows"} {
                if {$runtimeFile ne ""} {
                    append reason "\nVIEWDRAW runtime: $runtimeFile\nDLL directories: [join $libraryDirs {;}]"
                } else {
                    append reason "\nConfigure/build ViewDraw, then set ::env(OCCTV_DRAW_RUNTIME) to its bin/bind/viewdraw-runtime.tcl file."
                }
            }
            return -options $options $reason
        }
    }
}
::ViewDrawRuntime::load
