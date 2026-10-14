# Run under test/capture-cut-upload.py in GDB, separately from frame timing.
source [file join [file dirname [info script]] viewdraw_runtime.tcl]
pload MODELING VISUALIZATION
vinit UploadCapture -width 900 -height 650
vsetbg -color WHITE
scvcutview upload read $::env(CUTVIEW_UPLOAD_FIXTURE)
scvcutview upload add p 1 0 0 -3.125
scvcutview upload field point temperature 0
scvcutview upload visibility p 3
scvcutview upload display
vaxo
vfit
puts "UPLOAD_PHASE warm [scvcutmemory]"
foreach d {-3.2 -3.3 -3.4} { scvcutview upload move p 1 0 0 $d }
puts "UPLOAD_PHASE field [scvcutmemory]"
scvcutview upload field cell pressure 0
puts "UPLOAD_PHASE slice_only [scvcutmemory]"
scvcutview upload visibility p 2
scvcutview upload move p 1 0 0 -3.5
puts "UPLOAD_PHASE complete [scvcutmemory]"
scvcutview upload clear
exit 0
