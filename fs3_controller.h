#ifndef FS3_CONTROLLER_INCLUDED
#define FS3_CONTROLLER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_controller.h
//  Description    : This is the interface of the controller for the FS3
//                   filessystem interface.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Thu 16 Sep 2021 02:54:13 PM EDT
//

// Include
#include <stdint.h>

// These are the constants defining the size of the disk elements
#define FS3_MAX_TRACKS 64
#define FS3_TRACK_SIZE 1024
#define FS3_SECTOR_SIZE 1024
#define FS3_NO_TRACK (FS3_MAX_TRACKS+0xff)

// Type definitions
typedef uint64_t FS3CmdBlk;                 // The command block base data type
typedef uint16_t FS3TrackIndex;             // Index number of track
typedef uint16_t FS3SectorIndex;            // Sector index in current track
typedef char FS3Sector[FS3_SECTOR_SIZE];    // A sector
typedef FS3Sector FS3Track[FS3_TRACK_SIZE]; // A track


// These are the opcode (instructions) for the controller
typedef enum {

	FS3_OP_MOUNT  = 0,  // Mount the filesystem
	FS3_OP_TSEEK  = 1,  // Seek to a track
	FS3_OP_RDSECT = 2,  // Read a sector from the disk
	FS3_OP_WRSECT = 3,  // Write a sector to the disk
	FS3_OP_UMOUNT = 4,  // Unmount the ffilesystem
	FS3_OP_MAXVAL = 5   // Maximum opcode value

} FS3OpCodes;

//
// Global Data 
extern unsigned long FS3ControllerLLevel;  // Controller log level
extern unsigned long FS3DriverLLevel;      // Driver log level
extern unsigned long FS3SimulatorLLevel;   // Driver log level

//
// Functional Prototypes

FS3CmdBlk fs3_syscall(FS3CmdBlk cmdblock, void *buf);
	// This is the bus interface for communicating with controller

int fs3_unit_test(void);
	// This function runs the unit tests for the fs3 controller.

int fs3_log_controller_metrics(void);
	// Log the metrics for the controller 

#endif
