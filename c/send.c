#include <stdio.h>
#include <string.h>
#include <unistd.h> // for getpass()
#include "email_notifier.h"

#define LIBB64

extern char auth_filename[];

void p_err(int ret){
      switch(ret){
            case -1: printf("login information and recipient(s) must be specified\n"); break;
            case 67: printf("authentication failure\n"); break;
            case 6:  printf("could not connect to the internet\n"); break;
            case 1:  printf("unsupported protocol\n"); break;
            case 0:  printf("email sent succesfully\n"); break;
            default: printf("unknown error: %i occured\n", ret);
      }
}

// TODO: fix ridiculous memory leaks
int main(int argc, char* argv[]){
	struct notification_message* nm = init_nm(NULL);
      bool snd = false; 
      for(int i = 1; i < argc; ++i){
            if(*argv[i] == '-'){
                  switch(argv[i][1]){
                        case 's': nm->subject = strdup(argv[i+1]); break;
                        case 'm': nm->message = strdup(argv[i+1]); break;
                        case 'r': add_reciever(nm, strdup(argv[i+1])); break;
                        case 'a': add_attachment(nm, strdup(argv[i+1])); break;
                        case 'A': memset(auth_filename, '\0', strlen(auth_filename)); strcpy(auth_filename, argv[i+1]); break;
                        case 'S': snd = true; break;
                        case 'u': nm->email_from_username = strdup(argv[i+1]); break;
                        case 'p': nm->email_from_password = strdup(argv[i+1]); break;
                        case 'h': printf(
                                  "usage:\n    -s subject\n    -m message\n    -r reciever\n    -a attachment\n    -A auth_filename"
                                  "(if none specified, auth.txt will be used)\n    -S send with no further input\n    -u "
                                  "username\n    -p password\n    -R recursively attach directory\n    -h display this help\n"
                                  "\n1-%i recievers can be specified\n1-%i attachments can be specified\n",
                                  MAX_RECVRS, MAX_ATTACHMENT_NUM);
                                  free_nm(nm);
                                  return 0;
                  }
            }
      }
      if(snd){
            bool use_auth_file = (!nm->email_from_username || !nm->email_from_password);
            if(use_auth_file && file_exists(auth_filename)){
                  char** auth_info = get_auth_info();
                  nm->email_from_username = auth_info[0];
                  nm->email_from_password = auth_info[1];
            }
            if(!nm_warn(nm, true))return 1;
            int ret = notify(nm);
            p_err(ret);
            free_nm(nm);
            return ret;
      }
	if((strcmp(auth_filename, "") == 0 || !file_exists(auth_filename)) && (!nm->email_from_username || !nm->email_from_password)){
		printf("enter username\n");
            size_t sz = 0;
            getline(&nm->email_from_username, &sz, stdin);
		printf("\nenter password\n");
		nm->email_from_password = getpass("");
	}
	if(!nm->n_recievers){
		printf("enter recievers\n");
		char* rec_tmp;
            size_t sz = 0;
		for(int i = 0; i < MAX_RECVRS; ++i){
                  getline(&rec_tmp, &sz, stdin);
			if(strcmp(rec_tmp, "\n") == 0)break;
			nm->recievers[i] = rec_tmp;
		}
	}

	if(!nm->subject){
		printf("enter email subject\n");
            size_t sz = 0;
            getline(&nm->subject, &sz, stdin);
	}

	if(!nm->message){
		printf("enter email message\n");
            size_t sz = 0;
            getline(&nm->message, &sz, stdin);
	}
	if(!nm->n_attachments){
		printf("enter attachments to send\n");
            char* atch = NULL;
            size_t sz = 0;
            ssize_t read = 0;
		for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){
                  read = getline(&atch, &sz, stdin);
                  if(atch[read-1] == '\n')atch[read-1] = '\0';
			if(strcmp(atch, "") == 0)break;
			if(file_exists(atch)){
				nm->attachments[i] = atch; //consider switching to global var at, so that these do not overwrite previously declared attachments from flags
				printf("File will be added to upload list\n");
				printf("\ncurrent upload list is: \n");
				for(int j = 0; j <= i; ++j){
                              printf("%s : %i MB\n", nm->attachments[j], filesize(nm->attachments[j]));
				}
			}
			else{
				printf("enter file that exists\n");
				--i;
			}
		}
	}
	printf("press enter to send message\n");
	while(getchar() != '\n');
      bool use_auth_file = (!nm->email_from_username || !nm->email_from_password);
	if(file_exists(auth_filename) && use_auth_file){
            char** auth_info = get_auth_info();
		nm->email_from_username = auth_info[0];
		nm->email_from_password = auth_info[1];
	}
      if(!nm_warn(nm, true))return 1;
	int ret = notify(nm);
      p_err(ret);
      free_nm(nm);
	return ret;
	
}
