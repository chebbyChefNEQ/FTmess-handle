#include "sync_helper.h"

SyncHelper * helper_init() {
	SyncHelper * new_helper = malloc(sizeof(SyncHelper));

	new_helper->received = 0;
	new_helper->current_alive = 0;
	for(int i = 0; i < MAX_SERVER; ++i) {
		new_helper->upper_bound[i] = INT_MIN;
		new_helper->lower_bound[i] = INT_MAX;
		new_helper->leading_server[i] = -1;
		new_helper->has_received[i] = 0;
	}
	return new_helper;
}

// returns 0 when need more vector, returns 1 when received enough vectors
// This is done on the basis of the order in which the vectors are received
// Thank to the AGREED order messaging, it is possible to get the same result
// On all syncing machines
int helper_update(SyncHelper * h, int vector[MAX_SERVER]) {
	int from_server = vector[0];
	printf("Vector from %d\n", from_server);
	if(!h->has_received[from_server]) {h->received += 1; h->has_received[from_server] = 1;}

	for(int i = 1; i < MAX_SERVER; ++i) {
		if(vector[i] > h->upper_bound[i]) {

			h->leading_server[i] = from_server; // -1 is no need to sync
				
		}
		h->upper_bound[i] = max(vector[i], h->upper_bound[i]);
		h->lower_bound[i] = min(vector[i], h->lower_bound[i]);
	}
	printf("Sync status: %d reveived, %d expecting\n", h->received, h->current_alive);
	return h->received == h->current_alive;
}

void helper_reset(SyncHelper * h, int num_server) {
	h->received = 0;
	h->current_alive = num_server;
	for(int i = 0; i < MAX_SERVER; ++i) {
		h->upper_bound[i] = INT_MIN;
		h->lower_bound[i] = INT_MAX;
		h->leading_server[i] = -1;
		h->has_received[i] = 0;
	}
}

void print_vectors(SyncHelper * h) {
	printf("upper_bound \t");
	for(int i = 1; i < MAX_SERVER; ++i) {
		printf("%d ", h->upper_bound[i]);
	}
	printf("\n");

	printf("lower_bound \t");
	for(int i = 1; i < MAX_SERVER; ++i) {
		printf("%d ", h->lower_bound[i]);
	}
	printf("\n");

	printf("leading_server \t");
	for(int i = 1; i < MAX_SERVER; ++i) {
		printf("%d ", h->leading_server[i]);
	}
	printf("\n");
}