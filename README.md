# performanceCounting

Required Libraries
==================

This work requires lpthread, libpfm4 and hwloc. test.c uses papi to measure perf counters.

Building
========

` $CC -O0 -qopenmp perftest.c -o perfoutput -std=c99 -lpfm -lpthread -lhwloc` will build the perftest.c example. This measures hardware counters for a simple openmp loop, with each thread detecting the CPU affinity they're set to and measuring their own hardware counters. Without affinity the behaviour is not sensibly defined (`OMP_PLACES=cores OMP_PROC_BIND=close for example`).
The code is setup to read from performance counters that are part of the Intel Skylake Xeon system. If you remove the pe3 and pe4 counters then the code should be generic as it only uses more standard counters as part of perf.

`$CC -O0 -qopenmp test.c -o papioutput -std=c99 -lpapi` will build the papi example. This is significantly less developed but will work similarly (per thread counters).

Authors & Copyright
===================

The code is copyright STFC 2018 and written by Aidan Chalk. The code is licensed under the wtfpl2 license
