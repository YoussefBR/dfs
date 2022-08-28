## How to compile and test

- To cleanup your compiled objects, run:
  ```
  make clean
  ```

- To compile your code, you have to use:
  ```
  make
  ```

- To test your program with the provided workload files, run the code specifying a workload file as follows: 
  ```
  ./fs3_sim -v assign3-workload.txt
  ``` 
  Note that you don't necessarily have to use the `-v` option, but it provides a lot of debugging information that is helpful. The following `make` command will also run the same test (see `Makefile` for why):
  ```
  make test
  ```


- If the program completes successfully, the following should be displayed as the last log entry:
  ```
  FS3 simulation: all tests successful!!!
  ```

---
