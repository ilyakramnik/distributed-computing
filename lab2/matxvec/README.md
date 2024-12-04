# Multiplying a matrix by a vector 

## General

### How to build
```sh
make
```

### How to run 
```sh
cd ./bin
mpiexec -n <N> ./matxvec.o <rows> <cols>
# <N>    - The number of processes.
# <rows> - The number of rows.
# <cols> - The number of columns.
```

## Tests

### How to build tests
```sh
make test
```

### How to run test
```sh
cd ./bin
mpiexec -n <N> ./matxvec.o # <N> - Specify the number of processes.
```

## Stats and Results

| division by columns | division by rows |
| ------------------- | ---------------- |
| ![division by columns  20000x100000 x2000 (2)](https://github.com/user-attachments/assets/4145a4c0-8842-4e62-a297-e0363846a2e4) | ![division by row  2000 100000 x2000 (1)](https://github.com/user-attachments/assets/8efc74fd-0904-45d2-9cda-6bec6a4ca354) |
| ![division by columns 2000x 20000 100000  (2)](https://github.com/user-attachments/assets/4f4153cc-e026-4147-8300-4bfd428cafee) | ![division by row 2000x 20000 100000  (1)](https://github.com/user-attachments/assets/9c6b30c5-fb9a-4186-a9af-01f738d15bc9) |

As you can see from the screenshots above, when splitting into columns or rows, the more processes there are, the faster the algorithms work. You can also notice jumps. I assume that jumps occur because in addition to these processes, others are running on my laptop, and perhaps different system interrupts can sometimes interfere with computing. Also, it should be taken into account that when heated, my laptop on average gave worse results, and testing for each matrix size was carried out 1 time. Also, in the graphs above there are statistics of block partitions of matrices.




