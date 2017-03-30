#include <iostream>
#include "email_notifier.cpp"


int main(int argc, char* argv[]){
	notification_message nm;
	bool atch_sp = false, reciever_sp = false, sub_sp = false, msg_sp = false; //sp - specified
	if(argc > 1){
		std::string flag = "";
		int rc = 0; int at = 0; bool snd = false; 
		for(int i = 0; i < argc; ++i){
			flag += argv[i][0];
			flag += argv[i][1];
			if(flag == "-s"){
				nm.subject = argv[i+1];
				sub_sp = true;
			}
			if(flag == "-m"){
				nm.message = argv[i+1];
				msg_sp = true;
			}
			if(flag == "-r"){
				nm.recievers[rc++] = argv[i+1];
				reciever_sp = true;
			}
			if(flag == "-a"){
				nm.attachments[at++] = argv[i+1];
				atch_sp = true;
			}
			if(flag == "-A")auth_filename = argv[i+1];
			if(flag == "-S")snd = true; //mail should be sent immediately with no further input
			if(flag == "-u")nm.email_from_username = argv[i+1];
			if(flag == "-p")nm.email_from_password = argv[i+1];
			if(flag == "-h"){
				std::cout << 
				"usage:\n-s subject\n-m message\n-r reciever\n-a attachment\n-A auth_filename (if none specified, auth.txt will be used)"
				"\n-S send with no further input\n-u username\n-p password\n-h help" 
				<< std::endl;
				std::cout << "\n1-" << MAX_RECVRS << " recievers can be specified" << std::endl;
				std::cout << "1-" << MAX_ATTACHMENT_NUM << " attachments can be specified" << std::endl;
				return 0;
			}

			flag = "";
		}
		if(snd){
			bool use_auth_file = (nm.email_from_username == "" || nm.email_from_password == "");
			if(file_exists(auth_filename) && use_auth_file){
				std::string* auth_info = get_auth_info();
				nm.email_from_username = auth_info[0];
				nm.email_from_password = auth_info[1];
				delete[] auth_info;
			}
			int ret = notify(nm);
			if(ret == 67)std::cout << "authentication failure" << std::endl;
			if(ret == 6)std::cout << "could not connect to the internet" << std::endl;
			if(ret == 0)std::cout << "email sent succesfully" << std::endl;
			return ret;
		}
	}
	if((auth_filename == "" || !file_exists(auth_filename)) && (nm.email_from_username == "" || nm.email_from_password == "")){ // auth filename will never be "", initialized to auth.txt
		std::cout << "enter username" << std::endl;
		std::getline(std::cin, nm.email_from_username);

		std::cout << "\nenter password" << std::endl;
		std::getline(std::cin, nm.email_from_password);
	}
	if(!reciever_sp){
		std::cout << "enter recievers" << std::endl;
		std::string rec_tmp = "";
		for(int i = 0; i < MAX_RECVRS; ++i){
			std::getline(std::cin, rec_tmp);
			if(rec_tmp == "")break;
			nm.recievers[i] = rec_tmp;
		} }

	if(!sub_sp){
		std::cout << "enter email subject" << std::endl;
		std::getline(std::cin, nm.subject);
	}

	if(!msg_sp){
		std::cout << "enter email message" << std::endl;
		std::getline(std::cin, nm.message);
	}
		
	if(!atch_sp){
	//TODO: 
	//count size. if size >= no more attachments can be attached
		std::cout << "enter attachments to send" << std::endl;
		std::string atch = "";
		for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){
			std::getline(std::cin, atch);
			if(atch == "")break;

			if(file_exists(atch)){
				nm.attachments[i] = atch;
				std::cout << "File will be added to upload list" << std::endl;
				std::cout << "\ncurrent upload list is: " << std::endl;
				for(int j = 0; j <= i; ++j){
					std::cout << nm.attachments[j] << " : " << filesize(nm.attachments[j]) << " MB " <<  std::endl;
				}
			}
			else{
				std::cout << "enter file that exists" << std::endl;
				--i;
			}
		}
	}
	std::cout << "press enter to send message" << std::endl;
	while(std::getchar() != '\n');
	bool use_auth_file = (nm.email_from_username == "" || nm.email_from_password == "");
	if(file_exists(auth_filename) && use_auth_file){
		std::string* auth_info = get_auth_info();
		nm.email_from_username = auth_info[0];
		nm.email_from_password = auth_info[1];
		delete[] auth_info;
	}
	int ret = notify(nm);
	if(ret == 67)std::cout << "authentication failure" << std::endl;
	if(ret == 6)std::cout << "could not connect to the internet" << std::endl;
	if(ret == 0)std::cout << "email sent succesfully" << std::endl;
	return ret;
	
}
