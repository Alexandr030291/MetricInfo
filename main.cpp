#include <iostream>
#include <curl/curl.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <zconf.h>
#include "info.h"

struct URL{
    char *host;
    int port;
};

char *server_name = nullptr;
/*static */Info info;

char * toUrl(const char * root_path, int port) {
    const char _http[] = "http://";
    size_t length = strlen(_http);
    length += strlen(root_path);
    length += 20;
    length +=+strlen(": ");
    char *_url= (char*)malloc(length);
    sprintf(_url,"%s%s:%d",_http,root_path,port);
    return _url;
}

URL settings(int argc, char *argv[]){
    int i = 1;
    URL addr;
    char port[10] = "0";
    addr.host= nullptr;
    if (argc >= 2 && strcmp(argv[i], "-h") == 0) {
        std::cout << "-a  " << "site directory" << std::endl;
        std::cout << "-p  " << "port" << std::endl;
        std::cout << "-r  " << "rps address" << std::endl;
        std::cout << "-n  " << "name server" << std::endl;
        return addr;
    }
    while ((argc - i) > 1) {
        if (strcmp(argv[i], "-a") == 0) {
            addr.host = (char *) malloc(strlen(argv[i + 1]) + 1);
            strcpy(addr.host, argv[i + 1]);
        }else if (strcmp(argv[i], "-p") == 0) {
            strcpy(port, argv[i + 1]);
        }else if (strcmp(argv[i], "-r") == 0) {
            char *rps_path = (char *) malloc(strlen(argv[i + 1]) + 1);
            strcpy(rps_path, argv[i + 1]);
            info.set_rps_location(rps_path);
            free (rps_path);
        }else if (strcmp(argv[i], "-n") == 0) {
            server_name = (char *) malloc(strlen(argv[i + 1]) + 1);
            strcpy(server_name, argv[i + 1]);
        }
        i += 2;
    }
    addr.port=atoi(port);
    return addr;
}

static std::string buffer;
static int writer(char *data, size_t size, size_t nmemb, std::string *buffer) {
    int result = 0;
    if (buffer != NULL) {
        buffer->append(data, size * nmemb);
        result = size * nmemb;
    }
    return result;
}

int sendPost(char * url,char * http_request) {
    CURL *curl;
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers=NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charsets: utf-8");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, http_request);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        buffer.clear();
    }
    return 0;
}

int requestPost(URL * url) {
    const char _info_url[]="db/api/info";
    char *_url=toUrl(url->host,url->port);
    char *_url_path= (char*) malloc(strlen(_url)+strlen(_info_url)+5);
    sprintf(_url_path,"%s/%s",_url,_info_url);
    sendPost(_url_path,info.getInfo());
    free(_url_path);
    free (_url);
    return 0;
}

int namePost(URL url) {
    const char _info_url[]="db/api/name";
    const char _body_json[] ="{\"name\" : \"%s\" }";
    char *_url=toUrl(url.host,url.port);
    char *_url_path= (char*) malloc(strlen(_url)+strlen(_info_url)+5);
    sprintf(_url_path,"%s/%s",_url,_info_url);
    char * body=(char*) malloc(strlen(_body_json)+strlen(server_name)+5);
    sprintf(body,_body_json,server_name);
    sendPost(_url_path,body);
    free(body);
    free(_url_path);
    free (_url);
    return 0;
}

void* run_thread(void * url){
    info.update();
    requestPost((URL*) url);
}

int main(int argc, char *argv[]) {
    URL _url = settings(argc, argv);
    if (_url.host == nullptr){
        return 1;
    }
    if(server_name!= nullptr)
        namePost(_url);
    else
        return 2;

    info.update();
    pthread_t thread;
    while(true){
        sleep(1);
        pthread_create(&thread, NULL, run_thread, &_url);
    }
    return 0;
}

