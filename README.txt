Names: David Wang and Kirsten Choi

CONTRIBUTIONS

Kirsten and David worked on parsort.c and README.txt equally.

REPORT

My test data is reported below (after using ./run_experiments.sh command). I only copied the real time data to
look at the total wall-clock time the program takes to sort, and ran the commands multiple times to ensure
no outliers:

Wrote 16777216 bytes
Test run with threshold 2097152
real    0m0.405s

Test run with threshold 1048576
real    0m0.246s

Test run with threshold 524288
real    0m0.177s

Test run with threshold 262144
real    0m0.143s

Test run with threshold 131072
real    0m0.147s

Test run with threshold 65536
real    0m0.158s

Test run with threshold 32768
real    0m0.159s

Test run with threshold 16384
real    0m0.179s

Since the tests (from top to bottom) start from sequential sorting to running increasingly parallel processes
(decreasing thresholds), it makes sense that the test results would show decreasing total program runtimes.
Since running processes in parallel should in theory make a program more efficient(take less time) since 
multiple processes run at the same time instead of one after the other.

However, our results show that after a certain point of lowering thresholds, the runtime reaches a minimum
and starts increasing a little. This is because at a certain point, the amount of parallel processes exceeds
the number of CPU cores being used. At that point, the OS kernel has to switch between the processes, which will
generate additional overhead, adding to the total runtime. 

As seen in the results, the runtime stops decreasing in size and starts increasing instead 
(because the optimum amount of parallel processes to run is specific to the amount of CPU cores used.) 
Additional tasks beyond the optimum will slightly decrease performance. The parts of the computation being 
affected by this would be the parallel process section, AKA the merge_sort functions, whenever fork() is called.