# TODOs

### Fixes

 - [ ] Dubious perf data
 - [ ] Missing intro data
 - [ ] Inaccurate startup time measurement

### Features

 - [ ] More explanative log instead of "name population failed" (this
       happens when a process is detected but dies before information
       can be acquired about it)
 - [ ] Hashtable instead of sorted doubly linked list (performance
       improvement (\$O(n)\$ instead of \$O(n^s) \cap \Omega(n)\$ for process
       acquisition algorithm)
 - [ ] More syscalls supported by LCE 
 - [ ] Settings for more granular control of probing behavior in LCE
 - [ ] Better default in LCE

