## System and Project Overview

There are two parts to this new assignment. These include:

1. **Workload:** The new workload (`assign3-workload.txt`) contains multiple files (up to 10) and each of the files may increase in size to a maximum of 10KB. Your code must be modified to support these new kinds of opens, reads, writes, and closes. The major new logic for your code will involve dealing with reads and writes that span multiple sectors.
  
2. **Cache:** You are to create a write-through Least Recently Used (LRU) cache that will support caching sectors in memory. The cache rules are:

  - a) Every read or write should check the cache for the desired sector before accessing the disk via the system calls (`fs3_syscall`).

  - b) Every time a sector is retrieved it should be inserted into the cache.

  - c) If the cache is full when a sector is being inserted, the least recently used (LRU) sector should be ejected and the associated memory freed.
 
You are to implement the cache in the file (`fs3_cache.c`) whose declarations are made in (`fs3_cache.h`). The functions are:

- `int fs3_init_cache(uint16_t cachelines);` - This function will initialize the cache with a parameter that sets the maximum size of the cache (number of sectors that may be held in the cache). **Note that this function is called by the simulator**, so you don’t need to call it yourself. We will test the program with different cache sizes, including a cache size of zero.
 
- `int fs3_close_cache(void);` - This function closes the cache, freeing any sectors held in it. This is called by the simulator at the end of the workload, so you don’t need to call it yourself.

- `int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf);` - This inserts a sector into the cache. Note the cache uses the track and sector numbers to know which sectors are held. Be careful to make sure that the sector is malloc'd already before trying to use it. If the cache is full the least recently sector will be freed. Newly inserted sectors should be marked as the most recently used.
 
- `void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct);` - This function gets an element from the cache (returns `NULL` if not found). It uses the track and sector numbers as passed into the put cache method. Returned sectors should be marked as most recently used.
 
- `int fs3_log_cache_metrics(void);` - This function will use the logMessage interface to log the performance of the cache, including hits, misses, attempts, and hit ratio (see sample output). Note that this will require you to create some global data structures to hold statistics that are continuously updated by the above functions.
 
The key to this assignment is to design a data structure that holds the cache items and is resizable based on the parameter passed to the init cache function. We strongly suggest you work out the details of the cache and its function prior to implementing it.

---
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
