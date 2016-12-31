#ifndef SYNC_HELPER_H
#define SYNC_HELPER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>

#include "email.h"


// I did not write this function myself, stackoverflow did
// http://stackoverflow.com/questions/3437404/min-and-max-in-c
// Not that I can't write a marco, this is a way better implementation
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct _sync_helper {

	int received;
	int current_alive;
	int upper_bound[MAX_SERVER];
	int lower_bound[MAX_SERVER];
	int leading_server[MAX_SERVER];
	int has_received[MAX_SERVER];

} SyncHelper;

SyncHelper * helper_init();

// returns 0 when need more vector, returns 1 when received enough vectors
int helper_update(SyncHelper * h, int vector[MAX_SERVER]);

void helper_reset(SyncHelper * h, int num_server);

void print_vectors(SyncHelper * h);

#endif