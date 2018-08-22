#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include "email_notifier.h"

// TODO: fix ridiculous memory leaks

char auth_filename[] = "auth.txt";

struct notification_message* init_nm(struct notification_message* nm){
      if(!nm)nm = malloc(sizeof(struct notification_message));
      nm->email_from_username = nm->email_from_password = NULL;
      nm->attachments = nm->recievers = NULL;
      nm->atch_cap = nm->recv_cap = 5; 
      nm->n_attachments = nm->n_recievers = 0;
      nm->attachments = calloc(sizeof(char*), nm->atch_cap);
      nm->recievers = calloc(sizeof(char*), nm->recv_cap);
      nm->subject = nm->message = NULL;
      return nm;
}

int add_el_to_cpp(char** cpp, char* el, int* cap, int* sz){
      if(*sz == *cap){
            *cap *= 2;
            char** tmp = calloc(sizeof(char*), *cap);
            for(int i = 0; i < *sz; ++i)tmp[i] = cpp[i];
            free(cpp);
            cpp = tmp;
      }
      cpp[(*sz)++] = el;
      return *sz;
}

int add_attachment(struct notification_message* nm, char* atch_str){
      return add_el_to_cpp(nm->attachments, atch_str, &nm->atch_cap, &nm->n_attachments);
}

int add_reciever(struct notification_message* nm, char* recv_str){
      return add_el_to_cpp(nm->recievers, recv_str, &nm->recv_cap, &nm->n_recievers);
}

void free_nm(struct notification_message* nm){
      if(nm->email_from_username)free(nm->email_from_username);
      if(nm->email_from_password)free(nm->email_from_password);
      for(int i = 0; i < nm->n_attachments; ++i)free(nm->attachments[i]);
      if(nm->n_attachments)free(nm->attachments);
      for(int i = 0; i < nm->n_recievers; ++i)free(nm->recievers[i]);
      if(nm->n_recievers)free(nm->recievers);
      if(nm->subject)free(nm->subject);
      if(nm->message)free(nm->message);
}

bool file_exists(char* fname){
      FILE* fp = fopen(fname, "r");
      bool ret = fp != NULL;
      if(fp != NULL)fclose(fp);
      return ret;
}

bool is_viable_nm(struct notification_message* nm){
	return !(strcmp(nm->email_from_username, "") == 0 || 
             strcmp(nm->email_from_password, "") == 0 || 
             strcmp(nm->recievers[0],  "") == 0);
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
      for(int i = 0; i < nm->n_attachments; ++i){
            if(file_exists(nm->attachments[i])){
                  tmp_size = filesize(nm->attachments[i]);
			if(tmp_size + size_so_far <= MAX_ATTACHMENT_SIZE){ //if i can spare some MB's 
				size_so_far += tmp_size; 
                        good_attachments[c++] = nm->attachments[i];
			}
			else
                        // TODO: add more meaningful output, print max size and size of file for example
                        strcat(nm->message, "FILE TOO LARGE\n");
            }
		else strcat(nm->message, "FILE DOES NOT EXIST\n");
		tmp_size = 0;
      }
      char tmp_file[] = ".tmp_mail_contents";
      FILE* fp = fopen(tmp_file, "w");
      char* MIME = build_MIME(nm->subject, nm->message, good_attachments, c);
      fputs(MIME, fp);
      free(MIME);
      free(good_attachments);
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
            for(int i = 0; i < nm->n_recievers; ++i)recipients = curl_slist_append(recipients, nm->recievers[i]);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
		curl_easy_setopt(curl, CURLOPT_READDATA, FP);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		if(!is_viable_nm(nm))return -2;
		res = curl_easy_perform(curl);
		remove(tmp_file);
            curl_easy_cleanup(curl);
		return (int)res;
	}
      curl_easy_cleanup(curl);
	return -1;
}

bool nm_warn(struct notification_message* nm, bool warn){
      bool ret = true;
      if(!nm->email_from_username || !nm->email_from_password){
            if(warn)puts("username and password are required");
            ret = false;
      }
      if(!nm->n_recievers){
            if(warn)puts("at least one reciever must be specified");
            ret = false;
      }
      return ret;
}
