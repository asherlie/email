#include <stdio.h>
#include <cstdlib>
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <sstream>

#ifdef LIBB64
#include "base64.h"
#endif

//#define LIBB64
#define MAX_RECVRS 20
#define MAX_ATTACHMENT_SIZE 25
#define MAX_ATTACHMENT_NUM 50

std::string auth_filename = "auth.txt";
struct notification_message{
	notification_message() : subject(""), message(""){} 
	std::string email_from_username;
	std::string email_from_password;

	std::string attachments[MAX_ATTACHMENT_NUM];
	std::string recievers[MAX_RECVRS];

	std::string subject;
	std::string message;
};

bool file_exists(std::string& fname){
	std::ifstream ifs(fname);
	return ifs.good();
}

bool is_viable_nm(notification_message& nm){
	return !(nm.email_from_username == "" || nm.email_from_password == "" || nm.recievers[0] == "");
}

std::string* get_auth_info(){
	std::string* ret = new std::string[2];
	std::ifstream ifs(auth_filename);	
	std::getline(ifs, ret[0]);
	std::getline(ifs, ret[1]);
	return ret;
}

//std::ifstream::pos_type filesize(std::string fn){ //returns filesize
int filesize(std::string fn){ //returns filesize -- why use ifs::pos_type
	std::ifstream inf(fn.c_str(), std::ifstream::ate | std::ifstream::binary);
	return inf.tellg()/1000000;
}

std::string build_MIME(std::string subject, std::string message, std::string attachments[]){
	std::string contents = "Content-Type: multipart/mixed; boundary=adkkibiowiejdkjbazZDJKOIe\n"; //adkkibiowiejdkjbazZDJKOIe is an arbitrary boundary -
	contents += "Subject: " + subject + "\n";									    // - something that won't come up in a message or subject
	contents += "--adkkibiowiejdkjbazZDJKOIe\nContent-Type: multipart/alternative; boundary=adkkibiowiejdkjbazZDJKOIe1\n";
	contents += "--adkkibiowiejdkjbazZDJKOIe1\nContent-Type: text/plain; charset=UTF-8\n\n" + message +"\n--adkkibiowiejdkjbazZDJKOIe1--\n";

	std::string cmd;
	std::string b64_file;

      std::string p_text_file;
      std::string tmp;
	//adding attachments encoded in base64 to MIME format
	for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){ 
            #ifdef LIBB64
            std::ifstream ifs(attachments[i]);
            std::stringstream ss;
            ss << ifs.rdbuf();
            p_text_file = ss.str();

            Base64::Encode(p_text_file, &b64_file);
            contents += b64_file;
            p_text_file = ""; b64_file = "";
            //#endif // LIBB64
            #else
		if(attachments[i] == "")break; //TODO find a better place to brk
		//if(attachments[i] == "" || i >= sizeof(attachments)/sizeof(std::string))break; //TODO find a better place to brk
		contents += "\n--adkkibiowiejdkjbazZDJKOIe\n";
		contents += "Content-Type: application/octet-stream; name=\"" + attachments[i] + "\"\nContent-Transfer-Encoding: Base64\nContent-Disposition: attachment; filename=\"" + attachments[i] +"\"\n";
            cmd = "cat \"" + attachments[i] + "\"| base64 | cat > .tmp_b64_file_attachment";
            system(cmd.c_str());
            cmd = "";
            std::ifstream in_s(".tmp_b64_file_attachment");
            while(getline(in_s, b64_file)){
                  contents += b64_file;
            }
            b64_file = "";
            system("rm .tmp_b64_file_attachment");
            #endif
	}
	contents += "\n--adkkibiowiejdkjbazZDJKOIe--\n";
	return contents;
}

int notify(notification_message& nm){
	std::string good_attachments[MAX_ATTACHMENT_NUM]; int c = 0; int size_so_far = 0; int tmp_size = 0;
	for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){
		if(nm.attachments[i] == "")break;
		if(file_exists(nm.attachments[i])){
			tmp_size = filesize(nm.attachments[i]);
			if(tmp_size + size_so_far <= MAX_ATTACHMENT_SIZE){ //if i can spare some MB's 
				size_so_far += tmp_size; 
				good_attachments[c++] = nm.attachments[i];
			}
			else {
				nm.message += ("\nA file you attempted to attach is too large. It is " + std::to_string(tmp_size) + "MB's. It can be found here: \"" + nm.attachments[i] + "\" on the server\n");
			}
		}
		else {
			nm.message += ("\nThe file: '" + nm.attachments[i] + "' that you tried to attach does not exist\n");
		}
		tmp_size = 0;
	}
	

	std::string tmp_file = ".tmp_mail_contents";
	std::ofstream ostr;
	ostr.open(tmp_file.c_str());

	/*if(c!= 0)*/ostr << build_MIME(nm.subject, nm.message, good_attachments); //adds MIME to email file

	ostr.close();
	CURL *curl;
	FILE *FP;
	struct curl_slist *recipients = NULL;
	CURLcode res = CURLE_OK;

	curl = curl_easy_init();
	if(curl){
		FP = fopen(tmp_file.c_str(), "r");
		curl_easy_setopt(curl, CURLOPT_USERNAME, nm.email_from_username.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, nm.email_from_password.c_str());

		curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, nm.email_from_username.c_str());


		for(int i = 0; i< MAX_RECVRS; ++i){ //adds all recipients from std::string recievers[]
			if(nm.recievers[i] == "")break;
			recipients = curl_slist_append(recipients, nm.recievers[i].c_str());
		}

		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		curl_easy_setopt(curl, CURLOPT_READDATA, FP);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		//curl_easy_perform(curl); //the actual email sending is done here
		if(!is_viable_nm(nm))return -2;
		res = curl_easy_perform(curl);
		//curl_easy_cleanup(curl);
		remove(tmp_file.c_str());
		//return 0;
		return int(res);
	}
	return -1;
}
