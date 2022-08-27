#ifndef CMPSC311_UTIL_INCLUDED
#define CMPSC311_UTIL_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File          : cmpsc311_util.h
//  Description   : This is a set of general-purpose utility functions we use
//                  for the 311 homework assignments.
//
//  Author   : Patrick McDaniel
//  Created  : Sat Sep 21 06:47:40 EDT 2013
//
//  Change Log:
//
//  10/11/13	Added the timer comparison function definition (PDM)
//

// Includes
#include <stdint.h>
#include <gcrypt.h>

// Defines
#define CMPSC311_HASH_TYPE GCRY_MD_SHA1
#define CMPSC311_HASH_LENGTH (gcry_md_get_algo_dlen(CMPSC311_HASH_TYPE))
#define CMPSC311_SAFE_FREE(x) if ( x != NULL ) { free(x); x=NULL; }
#define CMPSC311_MAXVAL(a,b) ((a>b) ? a:b)
#define CMPSC311_MINVAL(a,b) ((a>b) ? b:a)
#define CMPSC311_ALLCHARS " !#$%&()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz{|}~"
#define CMPSC311_ALLWORDCHARS "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

// Functional prototypes

int generate_md5_signature(char *buf, uint32_t size, char *sig, uint32_t *sigsz);
    // Generate MD5 signature from buffer

int bufToString( char *buf, uint32_t blen, char *str, uint32_t slen );
    // Convert the buffer into a readable hex string

int charReplace( char *str, char replace, char with );
	// Replace all instances of one character in string

int stringToInt( char *str, uint32_t *val );
	// Convert the string into an integer

uint32_t getRandomValue( uint32_t min, uint32_t max );
    // Using strong randomness, generate random number

int32_t getRandomSignedValue( int32_t min, int32_t max );
    // Using strong randomness, generate random number (signed)

int getRandomData( char *blk, uint32_t sz );
	// Using gcrypt randomness, generate random data

int getRandomAlphanumericData( char *blk, uint32_t sz );
	// Using gcrypt randomness, generate random alphanumberic text data

int getRandomAlphanumericWord( char *blk, uint32_t sz );
	// Using gcrypt randomness, generate random word text data

long compareTimes(struct timeval * tm1, struct timeval * tm2);
    // Compare two timer values 

uint64_t htonll64(uint64_t val);
	// Create a 64-byte host-to-network conversion

uint64_t ntohll64(uint64_t val);
	// Create a 64-byte network-to-host conversion

int tokenizeString(char *inp, int maxwords, char *tokens[]);
	// create an array of tokens from an input string
	//
	// ** NOTE ** : strtok is used, so inp is mangled, so DO NOT
	//              pass constant string or it will SEGFAULT

int freeTokens( int maxwords, char *tokens[] );
	// safely free the tokens from a prior use of the tokenizer

int textToFile( const char *fname, const char *text, int ovr );
	// Create a text file containing the text passed in

// Unit tests

int b64UnitTest( void );
	// 64-bit conversion unit test

int tokenizeStringUnitTest( void );
	// tokenization unit test

#endif
