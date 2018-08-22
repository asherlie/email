#include <stdbool.h>
#include <stdlib.h>

#define MAX_RECVRS 20
#define MAX_ATTACHMENT_SIZE 25
#define MAX_ATTACHMENT_NUM 50

struct notification_message{
	char* email_from_username;
	char* email_from_password;
	char** attachments;
	char** recievers;
      int n_attachments, n_recievers, atch_cap, recv_cap;
	char* subject;
	char* message;
};

struct notification_message* init_nm(struct notification_message* nm);
void free_nm(struct notification_message* nm);
bool file_exists(char* fname);
bool is_viable_nm(struct notification_message* nm);
char** get_auth_info();
int filesize(char* fn);
char* build_MIME(char* subject, char* message, char** attachments, int n_attachments);
int notify(struct notification_message* nm);
int add_attachment(struct notification_message* nm, char* atch_str);
int add_reciever(struct notification_message* nm, char* recv_str);
bool nm_warn(struct notification_message* nm, bool warn);
