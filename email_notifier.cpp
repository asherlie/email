#include <stdio.h>
#include <cstdlib>
#include <curl/curl.h>
#include <fstream>
#include <string>

#define MAX_RECVRS 20
#define MAX_ATTACHMENT_SIZE 25
#define MAX_ATTACHMENT_NUM 50

/*
 * To use a gmail account as a sender, 'less secure apps' must be enabled 
 * This can be done here: https://www.google.com/settings/security/lesssecureapps
 */

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

std::string* get_auth_info(){
	std::string* ret = new std::string[2];
	std::ifstream ifs(auth_filename);	
	std::getline(ifs, ret[0]);
	std::getline(ifs, ret[1]);
	return ret;
}

std::ifstream::pos_type filesize(std::string fn){ //returns filesize
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
	//adding attachments encoded in base64 to MIME format
	for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){ 
		if(attachments[i] == "")break;
		contents += "\n--adkkibiowiejdkjbazZDJKOIe\n";
		contents += "Content-Type: application/octet-stream; name=\"" + attachments[i] + "\"\nContent-Transfer-Encoding: Base64\nContent-Disposition: attachment; filename=\"" + attachments[i] +"\"\n";
		cmd = "cat " + attachments[i] + "| base64 | cat > .tmp_b64_file_attachment";
		system(cmd.c_str());
		cmd = "";
		std::ifstream in_s(".tmp_b64_file_attachment");
		while(getline(in_s, b64_file)){
			contents += b64_file;
		}
		b64_file = "";
		system("rm .tmp_b64_file_attachment");
	}
	contents += "\n--adkkibiowiejdkjbazZDJKOIe--\n";
	return contents;
}

int notify(notification_message& nm){
	if(file_exists(auth_filename)){
		std::string* auth_i = get_auth_info();
		nm.email_from_username = auth_i[0];
		nm.email_from_password = auth_i[1];
		delete[] auth_i;
	}
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
		res = curl_easy_perform(curl);
		//curl_easy_cleanup(curl);
		remove(tmp_file.c_str());
		//return 0;
		return int(res);
	}
	return -1;
}
