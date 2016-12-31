#ifndef EMAIL_HANDLE_H
#define EMAIL_HANDLE_H

#define DEFAULT_BUFFER_SIZE 100

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "email.h"
// This is probably a good time to start encapsulating things

// ASSUME THE RETURNED MEMORY WILL BE FREED

typedef struct _update {

	// read or delete
	int action;

	// update needs an id
	int updating_server;
	int id;

	// email's id
	int sending_server;
	int sending_id;

} Update;

typedef struct _reader {

	int length;

	int size_update[MAX_SERVER];
	int size_email;

	FILE * f_update;

	FILE * f_email;

	// First element is ser_num
	int progress[MAX_SERVER];

	Email * email_buf; // Emails

	Update ** updates; // Number of servers -> Update

} Reader;



// Handle initializer, returns a handle that reads emails, from crash or not
// The ser_num is first stored in the 0-th position in the progress vector
Reader * handle_init(char * file, int ser_num);

// ---------------------------------------- Email API's -------------------------------------

// Stamps an email that is received from a client, and get it ready to be sent
void stamp_email(Reader * r, Email * mess);

// Dump email to disk, and inc the progress counter
// an email from a client will be saved to the local buffer to
// combat the possibility that the process can fail during send.
// These will trigger a write to the disk that update progress file and email file
int save_email(Reader * r, Email * mess);

// Returns all emails that has the given user name
Email * find_email(Reader * r, char * user, int * return_size);

Email * find_email_id(Reader * r, int process, int id);


// ------------------------------------ Update API's -----------------------------------------

// Stamps an update reveiced from client
void stamp_update(Reader * r, Update * u);

// Apply an update
// Rules:
// You can't read twice, progress recorded
// You can't delete twice, progress recorded
int apply_update(Reader * r, Update * u, int reboot);

// Returns an update from given ID, Email returned if id is an email, and is not deleted
// Update is returned if id correponse to an update
Update * get_update(Reader * r, int process, int id, Email ** e);


// ------------------------------------ Misc API's ---------------------------------------------

// Returns the progress vector
int * get_progress(Reader * r);

int resize_email(Reader * r);

int resize_update(Reader * r, int n);

/***********************************************************
 * File Structures: (Location in Bytes                     *
 *     Email Dump:                                         *
 *         0 - n * sizeof(Email)           Emails          *
 *     Update Dump:                                        *
 *         (MAX_SERVER * 4) + 1 - ...   Updates            *
 ***********************************************************/

 // If an email is marked delete, then that Email will not be read in to the new buffer
 // I might have the process rewrite that whole thing again just to 

#endif