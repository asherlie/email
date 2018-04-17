#include <stdbool.h>
#include <stdlib.h>

#define MAX_RECVRS 20
#define MAX_ATTACHMENT_SIZE 25
#define MAX_ATTACHMENT_NUM 50

struct notification_message{
	/*notification_message() : subject(""), message(""){} */
	char* email_from_username;
	char* email_from_password;

	/*char* attachments[MAX_ATTACHMENT_NUM];*/
	char** attachments;
	/*char* recievers[MAX_RECVRS];*/
	char** recievers;

	char* subject;
	char* message;
};

struct notification_message* init_nm();
bool file_exists(char* fname);
bool is_viable_nm(struct notification_message* nm);
char** get_auth_info();
int filesize(char* fn);
char* build_MIME(char* subject, char* message, char** attachments, int n_attachments);
int notify(struct notification_message* nm);
