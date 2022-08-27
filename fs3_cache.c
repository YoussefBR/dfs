////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_cache.c
//  Description    : This is the implementation of the cache for the 
//                   FS3 filesystem interface.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 17 Oct 2021 09:36:52 AM EDT
//

// Includes
#include <cmpsc311_log.h>
#include <stdlib.h>

// Project Includes
#include <fs3_cache.h>
#include <fs3_driver.h>

//
// Support Macros/Data
int32_t cachelineCount;
int32_t cachelineMax;
cacheEntry *cache;
uint64_t lastCount;

// METRICS VALS
int64_t inserts;
int64_t getCount;
int64_t hits;
int64_t misses;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_init_cache
// Description  : Initialize the cache with a fixed number of cache lines
//
// Inputs       : cachelines - the number of cache lines to include in cache
// Outputs      : 0 if successful, -1 if failure

int fs3_init_cache(uint16_t cachelines) {
    cache = malloc(sizeof(cacheEntry) * cachelines);
    cachelineCount = 0;
    lastCount = 0;
    cachelineMax = cachelines;
    // Metrics vals
    inserts = 0;
    getCount = 0;
    hits = 0;
    misses = 0;
    // Return
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close_cache
// Description  : Close the cache, freeing any buffers held in it
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_close_cache(void)  {
    // Only malloced the cache
    free(cache);
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_put_cache
// Description  : Put an element in the cache
//
// Inputs       : trk - the track number of the sector to put in cache
//                sct - the sector number of the sector to put in cache
// Outputs      : 0 if inserted, -1 if not inserted

int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf) {
    // Add an insert
    inserts++;
    // Load the new cache entry
    cacheEntry entry;
    uint16_t i, indexOfLRU = 0;
    entry.track = trk;
    entry.sector = sct;
    entry.count = lastCount;
    lastCount++;
    memcpy(&(entry.sectorContent), (char *)buf, FS3_SECTOR_SIZE);
    // If cache is full, kick out the least recently used entry
    if(cachelineCount == cachelineMax){
        // finds the least recently used entry (lru)
        uint64_t lru = UINT_FAST64_MAX;
        for(i = 1; i < cachelineMax; i++){
            if(cache[i].count < lru){
                lru = cache[i].count;
                indexOfLRU = i;
            }
        }
        cache[indexOfLRU] = entry;
    // Otherwise, just fill the next open cache entry
    }else{
        cache[cachelineCount] = entry;
        cachelineCount++;
    }
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_get_cache
// Description  : Get an element from the cache (
//
// Inputs       : trk - the track number of the sector to find
//                sct - the sector number of the sector to find
// Outputs      : returns NULL if not found or failed, pointer to buffer if found

void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct)  {
    // Add a get call
    getCount++;
    uint16_t cacheFound = 0, i = 0;
    // Loop through cache entries and see if the track and sector correspond to one
    while(i < cachelineCount && !cacheFound){
        if(cache[i].sector == sct && cache[i].track == trk){
            // If a cache entry is found return the sector content and add a hit
            cacheFound = 1;
            hits++;
            cache[i].count = lastCount;
            lastCount++;
            return((void *)&(cache[i].sectorContent));
        }
        i++;
    }
    // Add a miss if nothing is found
    misses++;
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_log_cache_metrics
// Description  : Log the metrics for the cache 
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_log_cache_metrics(void) {
    logMessage(LOG_OUTPUT_LEVEL, "Cache inserts    [    %d]\n", inserts);
    logMessage(LOG_OUTPUT_LEVEL, "Cache gets       [    %d]\n", getCount);
    logMessage(LOG_OUTPUT_LEVEL, "Cache hits       [    %d]\n", hits);
    logMessage(LOG_OUTPUT_LEVEL, "Cache misses     [    %d]\n", misses);
    logMessage(LOG_OUTPUT_LEVEL, "Cache hit ratio  [%%%.2f]", ((double)hits/getCount) * 100);
    return(0);
}
