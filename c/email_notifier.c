#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include "email_notifier.h"

// TODO: fix ridiculous memory leaks

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

int filesize(char* fn){
      FILE* fp = fopen(fn, "r");
      fseek(fp, 0L, SEEK_END);
      size_t sz = ftell(fp);
      fclose(fp);
      return sz/1000000;
}

char* build_MIME(char* subject, char* message, char** attachments, int n_attachments){
      char* contents = calloc(sizeof(char), 10000);
      sprintf(contents, "Content-Type: multipart/mixed; boundary=adkkibiowiejdkjbazZDJKOIe\nSubject: %s\n--adkkibiowiejdkjbazZDJKOIe\nContent-Type: multipart/alternative; boundary=adkkibiowiejdkjbazZDJKOIe1\n--adkkibiowiejdkjbazZDJKOIe1\n Content-Type: text/plain; charset=UTF-8\n\n%s\n--adkkibiowiejdkjbazZDJKOIe1--\n" , subject, message);
	char* b64_file;
      char* p_text_file;
      char* tmp;
	//adding attachments encoded in base64 to MIME format
	for(int i = 0; i < n_attachments; ++i){ 
            // TODO: implement base64 library method of MIME encoding
            // TODO: use ifdef to detect LIB_B64 macro
            char tmp_cont[4096];
            sprintf(tmp_cont, "\n--adkkibiowiejdkjbazZDJKOIe\nContent-Type: application/octet-stream; name=\"%s\"\nContent-Transfer-Encoding: Base64\nContent-Disposition: attachment; filename=\"%s\"\n", attachments[i], attachments[i]);
            strcat(contents, tmp_cont);
            char cmd[100];
            sprintf(cmd, "cat \"%s\" | base64 | cat > .tmp_b64_file_attachment", attachments[i]);
            system(cmd);
            FILE* fp = fopen(".tmp_b64_file_attachment", "r");
            size_t sz = 0;
            while(getline(&b64_file, &sz, fp) != EOF){
                  strcat(contents, b64_file);
            }
            system("rm .tmp_b64_file_attachment");
	}
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
			}
			else {
                        // TODO: add more meaningful output, print max size and size of file for example
                        strcat(nm->message, "FILE TOO LARGE\n");
			}
		}
		else {
                  // TODO: add more meaningful output
                  strcat(nm->message, "FILE DOES NOT EXIST\n");
		}
		tmp_size = 0;
	}
      char tmp_file[] = ".tmp_mail_contents";
      FILE* fp = fopen(tmp_file, "w");
      fputs(build_MIME(nm->subject, nm->message, good_attachments, c), fp);

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
		if(!is_viable_nm(nm))return -2;
		res = curl_easy_perform(curl);
		remove(tmp_file);
		return (int)res;
	}
	return -1;
}
