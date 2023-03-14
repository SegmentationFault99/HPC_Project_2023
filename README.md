# HPC_Project_2023
Final Project of the High Performance Computing course, 2022-2023 edition, University of Genova
<br><br><br>

## Sequential Implementation

This implementation makes use of the icc compiler over the INFN cluster.
<br>

Compilation
```
icc -O0 Sequential-Code.c -o sequential
```

Execution
```
./sequential <width> <heigth> 0
```

<br>

Compilation (vectorized version)
```
icc -O2 Sequential-Code.c -o sequential
```

Execution (vectorized version)
```
./sequential <width> <heigth> 1
```

<br>
The third parameter (0 or 1) tells the program to which result file write, since the same source code is used both for the vectorized and the non-vectorized version.
<br><br>

## OpenMP Implementation

This implementation makes use of the icc compiler over the INFN cluster.
<br>

Compilation
```
icc OpenMP-Code.c -fopenmp -o omp
```

Execution
```
./omp <width> <heigth> <number of threads>
```

<br>

## MPI Implementation

This implementation makes use of the mpiicc compiler over the INFN cluster.
<br>

Compilation
```
mpiicc MPI-Code.c -o mpi
```

Execution
```
mpiiexec -hostfile machinefile.txt -perhost <processes per node> -np <total processes> ./mpi <width> <heigth> <number of nodes>
```
NB: processes per node = total processes / number of nodes.

<br>

## CUDA Implementation

This implementation makes use of the nvcc compiler over the CUDA cluster.
<br>

Compilation
```
nvcc CUDA-Code.cu -o cuda
```

Execution
```
./cuda <width> <heigth> <number of threads>
```
