////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3sim.c
//  Description    : This is the main program for the CMPSC311 programming
//                   assignment #2 (beginning of FS3 interface).
//
//   Author        : Patrick McDaniel
//   Last Modified : Wed 15 Sep 2021 01:47:31 PM EDT
//

// Include Files
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Project Includes
#include <fs3_driver.h>
#include <fs3_controller.h>
#include <fs3_cache.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>

// Defines
#define FS3_WORKLOAD_DIR "workload"
#define FS3_SIM_MAX_OPEN_FILES 256
#define FS3_ARGUMENTS "huvc:l:"
#define USAGE \
	"USAGE: fs3_sim [-h] [-v] [-c <cache size>] [-l <logfile>] <workload-file>\n" \
	"\n" \
	"where:\n" \
	"    -h - help mode (display this message)\n" \
	"    -v - verbose output\n" \
	"    -c - set the cache size (in number of sectors)\n" \
	"    -l - write log messages to the filename <logfile>\n" \
	"\n" \
	"    <workload-file> - file contain the workload to simulate\n" \
	"\n" \

// This is the file table
typedef struct {
	char     *filename;  // This is the filename for the test file
	int16_t   fhandle;   // This is a file handle for the opened file
} FS3SimulationTable;

//
// Global Data
int verbose;
uint16_t fs3CacheSize = FS3_DEFAULT_CACHE_SIZE; 

//
// Functional Prototypes

int simulate_FS3( char *wload );              // control loop of the FS3 simulation
int validate_file(char *fname, int16_t mfh);  // Validate a file in the filesystem

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : main
// Description  : The main function for the FS3 simulator
//
// Inputs       : argc - the number of command line parameters
//                argv - the parameters
// Outputs      : 0 if successful test, -1 if failure

int main( int argc, char *argv[] ) {

	// Local variables
	int ch, verbose = 0, log_initialized = 0, unit_tests = 0;

	// Process the command line parameters
	while ((ch = getopt(argc, argv, FS3_ARGUMENTS)) != -1) {

		switch (ch) {
		case 'h': // Help, print usage
			fprintf( stderr, USAGE );
			return( -1 );

		case 'v': // Verbose Flag
			verbose = 1;
			break;

		case 'u': // Unit test Flag
			unit_tests = 1;
			break;

		case 'l': // Set the log filename
			initializeLogWithFilename( optarg );
			log_initialized = 1;
			break;

		case 'c': // Set the cache size
			if ( sscanf(optarg, "%hu", &fs3CacheSize) != 1) {
				logMessage(LOG_ERROR_LEVEL, "Failed parsing cache size [%s]", optarg);
				return(-1);
			}
			break;

		default:  // Default (unknown)
			fprintf( stderr, "Unknown command line option (%c), aborting.\n", ch );
			return( -1 );
		}
	}

	// Setup the log as needed
	if ( ! log_initialized ) {
		initializeLogWithFilehandle( CMPSC311_LOG_STDERR );
	}
	FS3ControllerLLevel = registerLogLevel("FS3_CONTROLLER", 0); // Controller log level
	FS3DriverLLevel= registerLogLevel("FS3_DRIVER", 0);          // Driver log level
	FS3SimulatorLLevel= registerLogLevel("FS3_SIMULATOR", 0);    // Driver log level
	if ( verbose ) {
		enableLogLevels(LOG_INFO_LEVEL);
		enableLogLevels(FS3ControllerLLevel | FS3DriverLLevel | FS3SimulatorLLevel);
	}

	// If extracting file from data
	if (unit_tests) {

		// Run the unit tests
		enableLogLevels( LOG_INFO_LEVEL );
		logMessage(LOG_INFO_LEVEL, "Running unit tests ....\n\n");
		if (fs3_unit_test() == 0) {
			logMessage(LOG_INFO_LEVEL, "Unit tests completed successfully.\n\n");
		} else {
			logMessage(LOG_ERROR_LEVEL, "Unit tests failed, aborting.\n\n");
		}

	} else {

		// The filename should be the next option
		if ( optind >= argc ) {
			fprintf( stderr, "Missing command line parameters, use -h to see usage, aborting.\n" );
			return( -1 );
		}

		// Run the simulation
		if ( simulate_FS3(argv[optind]) == 0 ) {
			logMessage( LOG_INFO_LEVEL, "FS3 simulation completed successfully.\n\n" );
		} else {
			logMessage( LOG_INFO_LEVEL, "FS3 simulation failed.\n\n" );
		}
	}

	// Return successfully
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : simulate_FS3
// Description  : The main control loop for the processing of the FS3
//                simulation.
//
// Inputs       : wload - the name of the workload file
// Outputs      : 0 if successful test, -1 if failure

int simulate_FS3( char *wload ) {

	// Local variables
	char line[1024], fname[128], command[128], text[1025], *sep, *rbuf;
	FILE *fhandle = NULL;
	int32_t err=0, len, off, fields, linecount;
	FS3SimulationTable ftable[FS3_SIM_MAX_OPEN_FILES];
	int idx, i;

	// Setup the file table
	memset(ftable, 0x0, sizeof(FS3SimulationTable)*FS3_SIM_MAX_OPEN_FILES);

	// Open the workload file
	linecount = 0;
	if ( (fhandle=fopen(wload, "r")) == NULL ) {
		logMessage( LOG_ERROR_LEVEL, "Failure opening the workload file [%s], error: %s.\n",
			wload, strerror(errno) );
		return( -1 );
	}

	// Startup the interface
	if ( (fs3_mount_disk() == -1) || (fs3_init_cache(fs3CacheSize) == -1) ){
		logMessage( LOG_ERROR_LEVEL, "FS3 simulator failed initialization.");
		fclose( fhandle );
		return( -1 );
	}
	logMessage(FS3SimulatorLLevel, "FS3 simulator initialization complete.");

	// While file not done
	while (!feof(fhandle)) {

		// Get the line and bail out on fail
		if (fgets(line, 1024, fhandle) != NULL) {

			// Parse out the string
			linecount ++;
			fields = sscanf(line, "%s %s %d %d", fname, command, &len, &off);
			sep = strchr(line, ':');
			if ( (fields != 4) || (sep == NULL) ) {
				logMessage( LOG_ERROR_LEVEL, "FS3 un-parsable workload string, aborting [%s], line %d",
						line, linecount );
				fclose( fhandle );
				return( -1 );
			}

			// Just log the contents
			logMessage(FS3SimulatorLLevel, "File [%s], command [%s], len=%d, offset=%d",
					fname, command, len, off);

			// Now walk the the table looking for the file
			idx = -1;
			i = 0;
			while ( (i < FS3_SIM_MAX_OPEN_FILES) && (idx == -1) ) {
				if ( (ftable[i].filename != NULL) && (strcmp(ftable[i].filename,fname) == 0) ) {
					idx = i;
				}
				i++;
			}

			// File is not found, open the file
			if (idx == -1) {

				// Log message, find unused index and save filename for later use
				logMessage(FS3SimulatorLLevel, "FS3_SIM : Opening file [%s]", fname);
				idx = 0;
				while ((ftable[idx].filename != NULL) && (idx < FS3_SIM_MAX_OPEN_FILES)) {
					idx++;
				}
				CMPSC311_ASSERT1(idx<FS3_SIM_MAX_OPEN_FILES, "Too many open files on FS3 sim [%d]", idx);
				ftable[idx].filename = strdup(fname);

				// Now perform the open
				ftable[idx].fhandle = fs3_open(ftable[idx].filename);
				if (ftable[idx].fhandle == -1) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Open of new file [%s] failed, aborting simulation.", fname);
					return(-1);
				}

			}

			// Now execute the specific command
			if (strncmp(command, "WRITEAT", 7) == 0) {

				// Log the command executed
				logMessage(FS3SimulatorLLevel, "FS3_SIM : Writing %d bytes at position %d from file [%s]", len, off, fname);

				// First perform the seek
				if (fs3_seek(ftable[idx].fhandle, off)) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Seek/WriteAt file [%s] to position %d failed, aborting simulation.", fname, off);
					return(-1);
				}

				// Now see if we need more data to fill, terminate the lines
				CMPSC311_ASSERT1(len<1024, "Simulated workload command text too large [%d]", len);
				CMPSC311_ASSERT2((strlen(sep+1)>=len), "Workload str [%d<%d]", strlen(sep+1), len);
				strncpy(text, sep+1, len);
				text[len] = 0x0;
				for (i=0; i<strlen(text); i++) {
					if (text[i] == '^') {
						text[i] = '\n';
					}
				}

				// Now perform the write
				if (fs3_write(ftable[idx].fhandle, text, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "WriteAt of file [%s], length %d failed, aborting simulation.", fname, len);
					return(-1);
				}


			} else if (strncmp(command, "WRITE", 5) == 0) {

				// Now see if we need more data to fill, terminate the lines
				CMPSC311_ASSERT1(len<1024, "Simulated workload command text too large [%d]", len);
				CMPSC311_ASSERT2((strlen(sep+1)>=len), "Workload str [%d<%d]", strlen(sep+1), len);
				strncpy(text, sep+1, len);
				text[len] = 0x0;
				for (i=0; i<strlen(text); i++) {
					if (text[i] == '^') {
						text[i] = '\n';
					}
				}

				// Log the command executed
				logMessage(FS3SimulatorLLevel, "FS3_SIM : Writing %d bytes to file [%s]", len, fname);

				// Now perform the write
				if (fs3_write(ftable[idx].fhandle, text, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Write of file [%s], length %d failed, aborting simulation.", fname, len);
					return(-1);
				}


			} else if (strncmp(command, "SEEK", 4) == 0) {

				// Log the command executed
				logMessage(FS3SimulatorLLevel, "FS3_SIM : Seeking to position %d in file [%s]", off, fname);

				// Now perform the seek
				if (fs3_seek(ftable[idx].fhandle, off) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Seek in file [%s] to position %d failed, aborting simulation.", fname, off);
					return(-1);
				}

			} else if (strncmp(command, "READ", 4) == 0) {

				// Log the command executed
				logMessage(FS3SimulatorLLevel, "FS3_SIM : Reading %d bytes from file [%s]", len, fname);

				// Now perform the read
				rbuf = malloc(len);
				if (fs3_read(ftable[idx].fhandle, rbuf, len) != len) {
					// Failed, error out
					logMessage(LOG_ERROR_LEVEL, "Read file [%s] of length %d failed, aborting simulation.", fname, off);
					return(-1);
				}
				free(rbuf);
				rbuf = NULL;

			} else {

				// Bomb out, don't understand the command
				CMPSC311_ASSERT1(0, "FS3_SIM : Failed, unknown command [%s]", command);

			}
		}

		// Check for the virtual level failing
		if ( err ) {
			logMessage( LOG_ERROR_LEVEL, "CRUS system failed, aborting [%d]", err );
			fclose( fhandle );
			return( -1 );
		}
	}

	// Now walk the the table looking for the file
	for (i=0; i<FS3_SIM_MAX_OPEN_FILES; i++) {
		if (ftable[i].filename != NULL) {
			if (validate_file(ftable[i].filename, ftable[i].fhandle) != 0) {
				logMessage(LOG_ERROR_LEVEL, "FS3 Validation failed on file [%s].", ftable[i].filename,fname);
				fclose( fhandle );
				return(-1);
			}

			// Clean up the file
			logMessage(FS3SimulatorLLevel, "Contents of file [%s] validated.", ftable[i].filename);
			fs3_close(ftable[i].fhandle);
			free(ftable[i].filename);
			ftable[i].filename = NULL;
		}
	}

	// Log cache metrics, shut down the interface
	if ( fs3_log_cache_metrics() == -1 ) {
		logMessage(LOG_ERROR_LEVEL, "FS3 simulation failed, controller metrics failed");
		return(-1);
	}
	if ((fs3_unmount_disk() == -1) || (fs3_close_cache() == -1)) {
		logMessage( LOG_ERROR_LEVEL, "FS3 simulator failed shutdown.");
		fclose( fhandle );
		return( -1 );
	}
	fs3_log_controller_metrics();
	logMessage(FS3SimulatorLLevel, "FS3 simulator shutdown complete.");
	logMessage(LOG_OUTPUT_LEVEL, "FS3 simulation: all tests successful!!!.");

	// Close the workload file, successfully
	fclose( fhandle );
	return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : validate_file
// Description  : Vadliate a file in the filesystem
//
// Inputs       : fname - the name of the file to validate
//                mfh - the disk file handle
// Outputs      : 0 if successful test, -1 if failure

int validate_file(char *fname, int16_t mfh) {

	// Local variables
	char filename[256], bkfile[256], *filbuf, *membuf;
	struct stat stats;
	int idx, fh;

	// First figure out how big the file is, setup buffer
	snprintf(filename, 256, "%s/%s", FS3_WORKLOAD_DIR, fname);
	if ((stat(filename, &stats) != 0) || (stats.st_size == 0)) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], missing or "
			"unknown source.", filename);
		return(-1);		
	}
	if ( ((filbuf = malloc(stats.st_size)) == NULL) || ((membuf = malloc(stats.st_size)) == NULL) ) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], failed "
			"buffer allocation.", filename);
		return(-1);		
	}

	// Now open the file and read the contents
	if ((fh=open(filename, O_RDONLY)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], open failed ", filename);
		return(-1);		
	}
	if ((read(fh, filbuf, stats.st_size)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure validating file [%s], read failed ", filename);
		return(-1);
	}
	close(fh);

	// Seek to the beginning of the disk file, read the contents
	if (fs3_seek(mfh, 0) == -1) {
		// Failed, error out
		logMessage(LOG_ERROR_LEVEL, "Read fs3 file [%s] see to zero failed.", fname);
		return(-1);
	}
	if (fs3_read(mfh, membuf, stats.st_size) != stats.st_size) {
		// Failed, error out
		logMessage(LOG_ERROR_LEVEL, "Read fs3 file [%s] of length %d failed.", fname, stats.st_size);
		return(-1);
	}

	// Now create a backup of the disk file so people can debug
	snprintf(bkfile, 256, "%s/%s.cmm", FS3_WORKLOAD_DIR, fname);
	if ((fh=open(bkfile, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure creating backup file [%s], open failed (%s) ", 
			bkfile, strerror(errno));
		return(-1);		
	}
	if ((write(fh, membuf, stats.st_size)) == -1) {
		logMessage(LOG_ERROR_LEVEL, "Failure writing backup file [%s].", bkfile);
		return(-1);
	}
	close(fh);

	// Now walk the buffers and compare byte for byte
	for (idx=0; idx<stats.st_size; idx++) {
		if (membuf[idx] != filbuf[idx]) {
			logMessage(LOG_ERROR_LEVEL, "Validation of [%s] failed at offset %d (mem %x/'%c' "
				"!= fil %x/'%c')", fname, idx, membuf[idx], membuf[idx], filbuf[idx], filbuf[idx]);
			return(-1);
		}
	}

	// Free the buffers, log success, and return successfully
	free(filbuf);
	free(membuf);
	logMessage(LOG_OUTPUT_LEVEL, "Validation of [%s], length %d sucessful.", fname, stats.st_size);
	return( 0 );
}
