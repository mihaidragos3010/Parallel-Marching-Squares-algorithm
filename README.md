# Parallel-Marching-Squares-algorithm
 The purpose of the homework was to transform a serial program into a parallel program through Pthread.
 --
 Within this project I rececived the serial Marching Squares algorithm. Based on this code I parallelized the steps of the
algorithm. I improved the performance of the algorithm by almost 30%. As a first step, I applied memory for all the arguments needed by the algorithm, 
in the main function, knowing that the heap memory will be shared and accessible by all future threads. I calculated, according to the number of threads and the 
id, an interval in each matrix for each thread that does not overlap with others. We have included in the parallel algorithm a set of barriers whose purpose is 
to ensure correctness from one step to another of the algorithm. I needed to guarantee the completion of a previous step for the current step. At the end I forced 
the main thread to wait for the completion of the other child threads with the "join" function. All steps of the algorithm can be found in the file "marchingSquares.cpp".
