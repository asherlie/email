#include <stdio.h>
#include <string.h>
#include <unistd.h> //for getpass()
#include <dirent.h> //for files in directory
//#include <vector>
#include "email_notifier.h"

#define LIBB64

extern char auth_filename[];

void p_err(int ret){
	if(ret == -2)printf("login information and recipient(s) must be specified\n");
	else if(ret == 67)printf("authentication failure\n");
	else if(ret == 6)printf("could not connect to the internet\n");
      else if(ret == 1)printf("unsupported protocol\n");
	else if(ret == 0)printf("email sent succesfully\n");
      else printf("unknown error: %i occured\n", ret);
}
/*
 *
 * //std::string* files_in_dir(std::string directory){
 * std::vector<std::string> files_in_dir(std::string directory){
 *      //std::string* dir_files = new std::string[MAX_ATTACHMENT_NUM];
 *      std::vector<std::string> dir_files;
 *      DIR* dir = opendir(directory.c_str());
 *      dirent* pdir;
 *      int c = 0; 	
 *      while((pdir = readdir(dir))){ //in paren to silence warning
 *            if(strncmp(pdir->d_name, ".", 2) != 0 && strncmp(pdir->d_name, "..", 2) != 0)dir_files.push_back(pdir->d_name);
 *            //dir_files[c++] = pdir->d_name;
 *      }
 *      return dir_files;
 *}
 *
 */

int main(int argc, char* argv[]){
	struct notification_message* nm = init_nm();
	bool atch_sp = false, reciever_sp = false, sub_sp = false, msg_sp = false; //sp - specified
	if(argc > 1){
		//std::string flag = "";
            char flag[2];
		int rc = 0; int at = 0; bool snd = false; 
		for(int i = 0; i < argc; ++i){
			flag[0] = argv[i][0];
			flag[1] = argv[i][1];
			if(strcmp(flag, "-s") == 0){
				nm->subject = argv[i+1];
				sub_sp = true;
			}
			if(strcmp(flag, "-m") == 0){
				nm->message = argv[i+1];
				msg_sp = true;
			}
			if(strcmp(flag, "-r") == 0){
				nm->recievers[rc++] = argv[i+1];
				reciever_sp = true;
			}
			if(strcmp(flag, "-a") == 0){
				nm->attachments[at++] = argv[i+1];
				atch_sp = true;
			}
			if(strcmp(flag, "-A") == 0){
                        memset(auth_filename, '\0', strlen(auth_filename));
                        strcpy(auth_filename, argv[i+1]);
                  }
			if(strcmp(flag, "-S") == 0)snd = true; //mail should be sent immediately with no further input
			if(strcmp(flag, "-u") == 0)strcpy(nm->email_from_username, argv[i+1]);
			if(strcmp(flag, "-p") == 0)strcpy(nm->email_from_password, argv[i+1]);
                  /*
			 *if(strcmp(flag, "-R") == 0){
			 *      std::vector<std::string> dir = files_in_dir(argv[i+1]);
			 *      for(int j=0; j <dir.size(); ++j){
			 *            std::string fname(argv[i+1]);// uses both dir and filenames
			 *            fname += ("/" + dir[j]); // to get full directory
			 *            nm->attachments[at++] = fname;
			 *      }
			 *}
                   */
			if(strcmp(flag, "-h") == 0){
				//std::cout << 
                        printf(
				"usage:\n-s subject\n-m message\n-r reciever\n-a attachment\n-A auth_filename (if none specified, auth.txt will be used)"
				"\n-S send with no further input\n-u username\n-p password\n-R recursively attach directory\n-h display this help\n\n1-%i recievers can be specified\n1-%i attachments can be specified", MAX_RECVRS, MAX_ATTACHMENT_NUM);
                        /*
				 *std::cout << "\n1-" << MAX_RECVRS << " recievers can be specified" << std::endl;
				 *std::cout << "1-" << MAX_ATTACHMENT_NUM << " attachments can be specified" << std::endl;
                         */
				return 0;
			}

			//flag = "";
		}
		if(snd){
			bool use_auth_file = (strcmp(nm->email_from_username, "") == 0 || strcmp(nm->email_from_password, "") == 0);
			if(file_exists(auth_filename) && use_auth_file){
				char** auth_info = get_auth_info();
                        strcpy(nm->email_from_username, auth_info[0]);
                        strcpy(nm->email_from_password, auth_info[1]);
                        /*
				 *nm->email_from_username = auth_info[0];
				 *nm->email_from_password = auth_info[1];
                         */
				//delete[] auth_info;
                        free(auth_info); // lol
			}
			int ret = notify(nm);
                  p_err(ret);
			return ret;
		}
	}
	if((strcmp(auth_filename, "") == 0 || !file_exists(auth_filename)) && (strcmp(nm->email_from_username, "") == 0 || strcmp(nm->email_from_password, "") == 0)){ // auth filename will never be "", initialized to auth.txt
		printf("enter username\n");
		//std::getline(std::cin, nm->email_from_username);
            size_t sz = 0;
            getline(&nm->email_from_username, &sz, stdin);

		printf("\nenter password\n");
            //sz = 0;
		nm->email_from_password = getpass("");
	}
	if(!reciever_sp){
		printf("enter recievers\n");
		char* rec_tmp;
            size_t sz = 0;
		for(int i = 0; i < MAX_RECVRS; ++i){
			//std::getline(std::cin, rec_tmp);
                  getline(&rec_tmp, &sz, stdin);
			if(strcmp(rec_tmp, "\n") == 0)break;
			nm->recievers[i] = rec_tmp;
		}
	}

	if(!sub_sp){
		printf("enter email subject\n");
		//std::getline(std::cin, nm->subject);
            size_t sz = 0;
            getline(&nm->subject, &sz, stdin);
	}

	if(!msg_sp){
		printf("enter email message\n");
		//std::getline(std::cin, nm->message);
            size_t sz = 0;
            getline(&nm->message, &sz, stdin);
	}
		
	if(!atch_sp){
	//TODO: 
	//count size. if size >= no more attachments can be attached
		printf("enter attachments to send\n");
		//std::string atch = "";
            char* atch;
            size_t sz = 0;
            ssize_t read = 0;
		for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){
			//std::getline(std::cin, atch);
                  read = getline(&atch, &sz, stdin);
                  if(atch[read-1] == '\n')atch[read-1] = '\0';
			if(strcmp(atch, "") == 0)break;
			if(file_exists(atch)){
				nm->attachments[i] = atch; //consider switching to global var at, so that these do not overwrite previously declared attachments from flags
				printf("File will be added to upload list\n");
				printf("\ncurrent upload list is: \n");
				for(int j = 0; j <= i; ++j){
					//std::cout << nm->attachments[j] << " : " << filesize(nm->attachments[j]) << " MB " <<  std::endl;
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
	bool use_auth_file = (strcmp(nm->email_from_username, "") == 0 || strcmp(nm->email_from_password, "") == 0);
	if(file_exists(auth_filename) && use_auth_file){
		//std::string* auth_info = get_auth_info();
            char** auth_info = get_auth_info();
		nm->email_from_username = auth_info[0];
		nm->email_from_password = auth_info[1];
		/*delete[] auth_info;*/
	}
	int ret = notify(nm);
      p_err(ret);
	return ret;
	
}
