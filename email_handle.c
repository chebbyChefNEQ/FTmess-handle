#include "email_handle.h"
// Handle initializer, returns a handle that reads emails, from crash or not
// The ser_num is first stored in the 0-th position in the progress vector
Reader * handle_init(char * file, int ser_num) {
	printf("Creating Email Handle\n");

	char update_file[100];
	char email_file[100];

	strcpy(update_file, file);
	strcpy(email_file, file);

	strcat(update_file, "_update");
	strcat(email_file, "_email");

	Reader * new_handle = malloc(sizeof(Reader));

	if(new_handle == NULL) {return NULL;}

	if(access( update_file, F_OK ) == -1 || access( email_file, F_OK ) == -1) { // update dump no email dump DNE
		for(int i = 0; i < MAX_SERVER; ++i) {
			new_handle->size_update[i] = DEFAULT_BUFFER_SIZE;
		}
		new_handle->size_update[0] = 1;

		new_handle->size_email = DEFAULT_BUFFER_SIZE;

		new_handle->length = 0;

		new_handle->progress[0] = ser_num;
		for(int i = 1; i < MAX_SERVER; ++i) {
			new_handle->progress[i] = 0;
		}

		new_handle->email_buf = malloc(sizeof(Email) * new_handle->size_email);

		new_handle->updates = malloc(sizeof(Update*) * MAX_SERVER);

		for(int i = 1; i < MAX_SERVER; ++i) {
			new_handle->updates[i] = malloc(sizeof(Update) * new_handle->size_update[i]);
			if(!new_handle->updates[i]) {return NULL;}
		}

		if(new_handle->email_buf == NULL || new_handle->updates == NULL) {return NULL;}

		new_handle->f_update = fopen(update_file, "wb");
		new_handle->f_email = fopen(email_file, "wb");

	} else {
		FILE * f_update = fopen(update_file, "rb");
		FILE * f_email = fopen(email_file, "rb");

		fseek(f_email, 0L, SEEK_END);
		new_handle->size_email = ftell(f_email) / sizeof(Email) + DEFAULT_BUFFER_SIZE;
		rewind(f_email);

		new_handle->email_buf = malloc(sizeof(Email) * new_handle->size_email);
		new_handle->updates = malloc(sizeof(Update*) * MAX_SERVER);

		for(int i = 0; i < MAX_SERVER; ++i) {
			new_handle->size_update[i] = DEFAULT_BUFFER_SIZE;
		}
		new_handle->size_update[0] = 1;
		
		for(int i = 1; i < MAX_SERVER; ++i) {
			// new_handle->updates[i] = malloc(sizeof(Update) * new_handle->progress[i] + DEFAULT_BUFFER_SIZE);
			new_handle->updates[i] = malloc(sizeof(Update) * new_handle->size_update[i]);
			if(!new_handle->updates[i]) {return NULL;}
		}

		int ret;

		Email mail_buf;
		new_handle->length = 0;
		while(!feof(f_email)) {
			ret = fread(&mail_buf, 1, sizeof(Email), f_email);
			if(ret != sizeof(Email)) {
				printf("End read\n");
				break;
			}
			if(!mail_buf.header.is_deleted) { // Dont load deleted email
				// Possible re-dump, to optimize
				new_handle->email_buf[new_handle->length++] = mail_buf;
			}
		}

		Update read_buf;
		new_handle->progress[0] = ser_num;
		for(int i = 1; i < MAX_SERVER; ++i) {
			new_handle->progress[i] = 0;
		}
		while(!feof(f_update)) {
			ret = fread(&read_buf, 1, sizeof(Update), f_update);
			if(ret != sizeof(Update)) {
				printf("End read\n");
				break;
			}
			apply_update(new_handle, &read_buf, 1);
		}


		new_handle->f_update = freopen ( update_file, "ab", f_update );
		new_handle->f_email = freopen ( email_file,"ab", f_email );
	}
	return new_handle;
}


// ---------------------------------------- Email API's -------------------------------------

// Stamps an email that is received from a client, and get it ready to be sent
void stamp_email(Reader * r, Email * mess) {
	// dont step the counter here, because the receiver will do that
	printf("Stamping Email\n");
	printf("To: %s\n", mess->header.to);
    printf("Sub: %s\n", mess->header.subject);
    printf("Body: %s\n", mess->body);

	mess->header.sending_server = r->progress[0];
	mess->header.id = r->progress[mess->header.sending_server];
}


// Dump email to disk, and inc the progress counter
// an email from a client will be saved to the local buffer to
// combat the possibility that the process can fail during send.
// These will trigger a write to the disk that update progress file and email file
int save_email(Reader * r, Email * mess) {
	if(r->length == r->size_email) {assert(!resize_email(r));} // do this anyways
	if(find_email_id(r, mess->header.sending_server, mess->header.id)) {return 0;} // just double checking

	printf("Saving Email\n");

	if(r->progress[mess->header.sending_server] != mess->header.id) {return 1;} // Fail on bad id sequence

	(r->email_buf[(r->length)++]) = *mess;


	// Creat an update to use later
	Update u;
	u.action = SEND;
	u.updating_server = mess->header.sending_server;
	u.id = mess->header.id;
	u.sending_server = mess->header.sending_server;
	u.sending_id = mess->header.id;
	apply_update(r, &u, 0);

	fwrite(mess, sizeof(Email), 1, r->f_email);

	fflush(r->f_email);
	sync();

	return 0;
}


// Returns all emails that has the given user name
Email * find_email(Reader * r, char * user, int * return_size) {
	*return_size = 0;
	Email * return_buf = malloc(sizeof(Email) * 100); // Assume no more then 100 Emails for now
	for(int i = 0; i < r->length; ++i) {
		if(!strcmp(user, r->email_buf[i].header.to) && 
			!r->email_buf[i].header.is_deleted) {
				return_buf[(*return_size)++] = r->email_buf[i];
		}
	}
	return return_buf;
}

Email * find_email_id(Reader * r, int process, int id) {
	for(int i = 0; i < r->length; ++i) {
		if(r->email_buf[i].header.sending_server == process &&
			r->email_buf[i].header.id == id &&
			!r->email_buf[i].header.is_deleted) {
			return r->email_buf + i;
		}
	}
	return NULL; // Deleted email
}

// ------------------------------------ Update API's -----------------------------------------

// Stamps an update reveiced from client
void stamp_update(Reader * r, Update * u) {
	u->updating_server = r->progress[0];
	u->id = r->progress[u->updating_server];
}

// Apply an update
// Rules:
// You can't read twice, progress recorded
// You can't delete twice, progress recorded
int apply_update(Reader * r, Update * u, int reboot) {

	assert(u->updating_server); // server number as 0 is gonna be bad

	printf("Applying Update\n");
	// Check id consecutiveness
	if(r->progress[u->updating_server] != u->id) {
		printf("Non-Consecutive update, ignore!\n");
		return 1;
	}
	if(u->id >= r->size_update[u->updating_server]) { // resize all update size.
		// I know i should have implemented size feild for each server
		// This is just quicker to implement, might change later
		resize_update(r, u->updating_server);
	}

	if(!reboot) {
		fwrite(u, sizeof(Update), 1, r->f_update);
		fflush(r->f_update);
		sync();
	}

	Email * e;
	r->updates[u->updating_server][u->id] = *u;
	r->progress[u->updating_server] += 1;
	switch(u->action) {
		case SEND:
			return 0; // pass nothing to do, here for clearity
			break;
		case READ:
			printf("Reading Email\n");
			e = find_email_id(r, u->sending_server, u->sending_id);
			if(e == NULL) {
				printf("Deleted email, incoming server behind\n");
				return 1;
			} else {
				e->header.is_read = 1;
			}
			break;
		case DELETE:
			printf("Deleting Email\n");
			e = find_email_id(r, u->sending_server, u->sending_id);
			if(e == NULL) {
				printf("Deleted email, incoming server behind\n");
			} else {
				e->header.is_deleted = 1;
				return 1;
			}
			break;
		default:
			printf("This shoud not happen, EVER\n");
			return 1;
	}

	return 0;
}



// Returns an update from given ID, Email returned if id is an email, and is not deleted
// Update is returned if id correponse to an update
// Assume safe bound
Update * get_update(Reader * r, int process, int id, Email ** e) {
	if(id >= r->progress[process]) {*e = NULL; return NULL;}
	Update * u = &(r->updates[process][id]);
	if(u->action == SEND) {
		*e = find_email_id(r, process, id);
	} else {
		*e = NULL;
	}
	return u;
}

// ------------------------------------ Misc API's ---------------------------------------------
// Returns the progress vector
int * get_progress(Reader * r) {
	printf("progress vector requested, This is server %d\n", r->progress[0]);
	for(int i = 1; i < MAX_SERVER; ++i) {
		printf("%d ", r->progress[i]);
	}
	printf("\n");
	return r->progress;
}

int resize_email(Reader * r) {
	r->size_email = r->length * 1.25 + 3;
	Email * new_buf = malloc(sizeof(Email) * r->size_email);
	if(new_buf == NULL) {return 1;}
	for(int i = 0; i < r->length; ++i) {
		new_buf[i] = r->email_buf[i];
	}
	free(r->email_buf);
	r->email_buf = new_buf;
	return 0;
}


// REDO!!!!!
int resize_update(Reader * r, int n) {
	r->size_update[n] = r->size_update[n] * 1.25 + 3;

	Update * new_buf = malloc(sizeof(Update) * r->size_update[n]);
	if(new_buf == NULL) {return 1;}

	for(int j = 0; j < r->progress[j]; ++j) {
		new_buf[j] = r->updates[n][j];
	}

	free(r->updates[n]);
	r->updates[n] = new_buf;

	return 0;
}