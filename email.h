#ifndef EMAIL_H
#define EMAIL_H

#define MAX_USER_NAME_LEN 100
#define MAX_SUBJECT_LEN 100
#define MAX_BODY_LEN 1200
#define MAX_SERVER 10

#define GROUP_NAME "OINNOIJ:LKMLKNOINOIN"
#define SERVER_NAME_PREFIX "(*)&BUHOIB*)&GVBIOU*("

// This emial struct sums to 1362 bytes in space
typedef struct _header {
	// UUID
	int sending_server;
	int id;

	// read flad
	int is_read;

	// delete flag
	int is_deleted;

	// Actual Email
	char from[MAX_USER_NAME_LEN];
	char to[MAX_USER_NAME_LEN];
	char subject[MAX_SUBJECT_LEN];
}Header;


typedef struct _email {

	// This split structure is for a more efficient email sending
	// Only header is send when listing
	// the bosy send will trigger
	Header header;

	char body[MAX_BODY_LEN];

} Email;


#define SEND   0  // This flag is only used if the email has been deleted
#define READ   1
#define DELETE 2

typedef struct _client_Update {

	int action;

	int process;

	int id;

} ClientUpdate;




#endif