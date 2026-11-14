# TODOs

### Fixes

 - [ ] Dubious perf data
 - [ ] Missing intro data
 - [X] Inaccurate startup time measurement
 - [ ] Newlines in proc name entries

### Features

 - [X] More explanative log instead of "name population failed" (this
       happens when a process is detected but dies before information
       can be acquired about it)
 - [ ] More performant process acquitision algorithm (currently
       \$O(n^2) \cap \Omega(n)\$ with sufficient cache locality)
 - [ ] More syscalls supported by LCE 
 - [ ] Settings for more granular control of probing behavior in LCE
 - [ ] Better default in LCE

