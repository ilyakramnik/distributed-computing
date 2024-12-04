# Cannon algorithm 

## General

### How to build
```sh
make
```

### How to run 
```sh
cd ./bin
mpiexec -n <N> ./cannon.o <rows> <cols>
# <N>    - The number of processes.
# <rows> - The number of rows.
# <cols> - The number of columns.
```

## Tests

### How to build tests
```sh
make tests
```

### How to run test
```sh
cd ./bin
mpiexec -n <N> ./cannon.o # <N> - Specify the number of processes.
```

## Stats and Results

| ![1 vs 2 vs 3 vs 4 proc](https://github.com/user-attachments/assets/1c19d29f-badf-41b0-a634-73e083145a58) |
| -- |
The screenshot above shows the results of time measurement (s) and matrix multiplication (NxN). One can notice the expected trend that with an increase in the number of processes, the time spent on counting multiplications decreases. The graph turned out to be uneven, since testing was carried out once for each size, and the measurement results were not averaged.
