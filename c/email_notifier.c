#include <stdio.h>
/*#include <stdbool.h>*/
/*#include <stdlib.h>*/
#include <curl/curl.h>
/*#include <fstream>*/
#include <string.h>
#include "email_notifier.h"
//#include <sstream>

/*
 *#ifdef LIBB64
 *#include "base64.h"
 *#endif
 */

//#define LIBB64

char auth_filename[] = "auth.txt";

struct notification_message* init_nm(){
      struct notification_message* nm = malloc(sizeof(struct notification_message));
      nm->email_from_username = calloc(sizeof(char), 100);
      nm->email_from_password = calloc(sizeof(char), 100);

	nm->attachments = calloc(sizeof(char*), MAX_ATTACHMENT_NUM);
	nm->recievers = calloc(sizeof(char*), MAX_RECVRS);
      for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i)nm->attachments[i] = calloc(sizeof(char), 100);
      for(int i = 0; i < MAX_RECVRS; ++i)nm->recievers[i] = calloc(sizeof(char), 100);

	nm->subject = calloc(sizeof(char), 256);
	nm->message = calloc(sizeof(char), 10000);
}


bool file_exists(char* fname){
      FILE* fp = fopen(fname, "r");
      bool ret = fp != NULL;
      if(fp != NULL)fclose(fp);
      return ret;
}

bool is_viable_nm(struct notification_message* nm){
	return !(strcmp(nm->email_from_username, "") == 0 || strcmp(nm->email_from_password, "") == 0 || strcmp(nm->recievers[0],  "") == 0);
}

char** get_auth_info(){
      char** ret = calloc(sizeof(char*), 2);
      FILE* fp = fopen(auth_filename, "r");
      size_t sz = 0;
      getline(&(ret[0]), &sz, fp);
      sz = 0;
      getline(&(ret[1]), &sz, fp);
      fclose(fp);
	return ret;
}

//std::ifstream::pos_type filesize(std::string fn){ //returns filesize
//int filesize(std::string fn){ //returns filesize -- why use ifs::pos_type
int filesize(char* fn){
      FILE* fp = fopen(fn, "r");
      fseek(fp, 0L, SEEK_END);
      size_t sz = ftell(fp);
      fclose(fp);
      return sz/1000000;
	//return inf.tellg()/1000000;
}

char* build_MIME(char* subject, char* message, char** attachments, int n_attachments){
      char* contents = calloc(sizeof(char), 10000);

      sprintf(contents, "Content-Type: multipart/mixed; boundary=adkkibiowiejdkjbazZDJKOIe\nSubject: %s\n--adkkibiowiejdkjbazZDJKOIe\nContent-Type: multipart/alternative; boundary=adkkibiowiejdkjbazZDJKOIe1\n--adkkibiowiejdkjbazZDJKOIe1\n Content-Type: text/plain; charset=UTF-8\n\n%s\n--adkkibiowiejdkjbazZDJKOIe1--\n" , subject, message);
/*
 *      strcat(contents, "Content-Type: multipart/mixed; boundary=adkkibiowiejdkjbazZDJKOIe\n"); //adkkibiowiejdkjbazZDJKOIe is an arbitrary boundary -
 *      contents += "Subject: " + subject + "\n";									    // - something that won't come up in a message or subject
 *      sprintf
 *      contents += "--adkkibiowiejdkjbazZDJKOIe\nContent-Type: multipart/alternative; boundary=adkkibiowiejdkjbazZDJKOIe1\n";
 *
 *      contents += "--adkkibiowiejdkjbazZDJKOIe1\nContent-Type: text/plain; charset=UTF-8\n\n" + message +"\n--adkkibiowiejdkjbazZDJKOIe1--\n";
 */

	//char* cmd;
	char* b64_file;

      char* p_text_file;
      char* tmp;
	//adding attachments encoded in base64 to MIME format
	for(int i = 0; i < n_attachments; ++i){ 
/*
 *            #ifdef LIBB64
 *            //std::ifstream ifs(attachments[i]);
 *            FILE* fp = fopen(attachments[i], "r");
 *            std::stringstream ss;
 *            ss << ifs.rdbuf();
 *            p_text_file = ss.str();
 *
 *            Base64::Encode(p_text_file, &b64_file);
 *            contents += b64_file;
 *            p_text_file = ""; b64_file = "";
 *            //#endif // LIBB64
 *            #else
 */
            /*printf("checking attachments[%i]: %s\n", i, attachments[i]);*/
		/*if(strcmp(attachments[i], "") == 0)break; //TODO find a better place to brk*/
            if(strlen(attachments[i]) == 0 || attachments[i][0] == '\0' || attachments[i][0] == '\n')break;
		//if(attachments[i] == "" || i >= sizeof(attachments)/sizeof(std::string))break; //TODO find a better place to brk
            char tmp_cont[4096];
            sprintf(tmp_cont, "\n--adkkibiowiejdkjbazZDJKOIe\nContent-Type: application/octet-stream; name=\"%s\"\nContent-Transfer-Encoding: Base64\nContent-Disposition: attachment; filename=\"%s\"\n", attachments[i], attachments[i]);
            strcat(contents, tmp_cont);
		//contents += "\n--adkkibiowiejdkjbazZDJKOIe\n";
		//contents += "Content-Type: application/octet-stream; name=\"" + attachments[i] + "\"\nContent-Transfer-Encoding: Base64\nContent-Disposition: attachment; filename=\"" + attachments[i] +"\"\n";
            char cmd[100];
            sprintf(cmd, "cat \"%s\" | base64 | cat > .tmp_b64_file_attachment", attachments[i]);
            //cmd = "cat \"" + attachments[i] + "\"| base64 | cat > .tmp_b64_file_attachment";
            system(cmd);
            //cmd = "";
            //std::ifstream in_s(".tmp_b64_file_attachment");
            FILE* fp = fopen(".tmp_b64_file_attachment", "r");
            size_t sz = 0;
            while(getline(&b64_file, &sz, fp) != EOF){
                  strcat(contents, b64_file);
                  //contents += b64_file;
            }
            //b64_file = "";
            system("rm .tmp_b64_file_attachment");
            //#endif
	}
	//contents += "\n--adkkibiowiejdkjbazZDJKOIe--\n";
      strcat(contents, "\n--adkkibiowiejdkjbazZDJKOIe--\n");
	return contents;
}

int notify(struct notification_message* nm){
	char** good_attachments = calloc(sizeof(char*), MAX_ATTACHMENT_NUM+1);
      int c = 0; int size_so_far = 0; int tmp_size = 0;
	for(int i = 0; i < MAX_ATTACHMENT_NUM; ++i){
		if(strcmp(nm->attachments[i], "") == 0)break;
		if(file_exists(nm->attachments[i])){
			tmp_size = filesize(nm->attachments[i]);
			if(tmp_size + size_so_far <= MAX_ATTACHMENT_SIZE){ //if i can spare some MB's 
				size_so_far += tmp_size; 
                        good_attachments[c++] = nm->attachments[i];
				/*good_attachments[c++] = nm->attachments[i];*/
			}
			else {
                        strcat(nm->message, "FILE TOO BIG\n");
				//nm->message += ("\nA file you attempted to attach is too large. It is " + std::to_string(tmp_size) + "MB's. It can be found here: \"" + nm->attachments[i] + "\" on the server\n");
			}
		}
		else {
			//nm->message += ("\nThe file: '" + nm->attachments[i] + "' that you tried to attach does not exist\n");
                  strcat(nm->message, "FILE DOES NOT EXIST\n");
		}
		tmp_size = 0;
	}
      good_attachments[c] = malloc(sizeof(char));
      good_attachments[c] = '\0';
	

      char tmp_file[] = ".tmp_mail_contents";
	//std::ofstream ostr;
      FILE* fp = fopen(tmp_file, "w");
	//ostr.open(tmp_file);
      fputs(build_MIME(nm->subject, nm->message, good_attachments, c), fp);

	//ostr.close();
      fclose(fp);
	CURL* curl;
	FILE* FP;
	struct curl_slist* recipients = NULL;
	CURLcode res = CURLE_OK;

	curl = curl_easy_init();
	if(curl){
		FP = fopen(tmp_file, "r");
		curl_easy_setopt(curl, CURLOPT_USERNAME, nm->email_from_username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, nm->email_from_password);

		curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, nm->email_from_username);


		for(int i = 0; i< MAX_RECVRS; ++i){ //adds all recipients from std::string recievers[]
			if(strcmp(nm->recievers[i], "") == 0)break;
			recipients = curl_slist_append(recipients, nm->recievers[i]);
		}

		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		curl_easy_setopt(curl, CURLOPT_READDATA, FP);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		//curl_easy_perform(curl); //the actual email sending is done here
		if(!is_viable_nm(nm))return -2;
		res = curl_easy_perform(curl);
		//curl_easy_cleanup(curl);
		remove(tmp_file);
		//return 0;
		return (int)res;
	}
	return -1;
}
