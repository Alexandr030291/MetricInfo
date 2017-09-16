#include <fstream>
#include <iostream>
#include <cstring>
#include <curl/curl.h>
#include "info.h"

//подключаем стандартное пространство имен
using namespace std;
//объявляем буфер, для хранения возможной ошибки, размер определяется в самой библиотеке
static char errorBuffer[CURL_ERROR_SIZE];
//объялвяем буфер принимаемых данных
static string buffer;
//функция обратного вызова
static int writer(char *data, size_t size, size_t nmemb, string *buffer) {
    //переменная - результат, по умолчанию нулевая
    int result = 0;
    //проверяем буфер
    if (buffer != NULL) {
        //добавляем к буферу строки из data, в количестве nmemb
        buffer->append(data, size * nmemb);
        //вычисляем объем принятых данных
        result = size * nmemb;
    }
    //вовзращаем результат
    return result;
}


void Info::netUpdate(){
    _net_receive_old = _net_receive;
    _net_transmit_old = _net_transmit;
    std::ifstream stat_file("/proc/net/dev");
    if (!stat_file.is_open()){
        std::cout << "Unable to open /proc/net/dev" << std::endl;
        return;
    }
    char *_tmp = (char*) malloc(50);
    stat_file >> _tmp;
    for(int j=0;j<21;j++){
        stat_file >> _tmp;
    }
    _net_receive = atol(_tmp);
    for(int j=0;j<8;j++){
        stat_file >> _tmp;
    }
    _net_transmit = atol(_tmp);
    free(_tmp);
    stat_file.close();
}

void Info::timeUpdate(){
    const time_t timer = time(NULL);
    _sys_time = timer;
}

void Info::set_rps_location(const char *_rps_location) {
    if(!Info::_rps_location)
        free(Info::_rps_location);
    Info::_rps_location = (char *)malloc(strlen(_rps_location));
    strcpy(Info::_rps_location,_rps_location);
}

void Info::cpuUpdate() {
    _cpu_work_old=_cpu_work;
    _cpu_busy_old=_cpu_busy;
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()){
        std::cout << "Unable to open /proc/stat" << std::endl;
        return;
    }
    char *_tmp= (char*)malloc(50);
    stat_file >> _tmp;
    long  val=0;
    _cpu_work=0;
    for (int i = 0 ; i < 4 ; i++) {
        stat_file >> _tmp;
        val= atol(_tmp);
        _cpu_work +=val;
    }
    _cpu_busy=_cpu_work-val;
    stat_file.close();
    free(_tmp);
}

void Info::rpsUpdate() {
    _rps_old = _rps;
    if (!_rps_location)
        return;
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) { //проверяем
        curl_easy_setopt(curl, CURLOPT_URL, _rps_location);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
        //указываем куда записывать принимаемые данные
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        //запускаем выполнение задачи
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            if (!_server_active) {
                timeUpdate();
                cout << "server start: " << ctime(&_sys_time) << endl;
                _server_active = true;
            }
            if (_flag) {
                size_t len = 128;
                char *tmp = (char *) malloc(len);
                int start = 0;
                int stop = 0;
                int spase = 12;
                const char * _buffer = buffer.data();
                while (spase > 0) {
                    start = stop;
                    char c;
                   do{
                        c = buffer.data()[stop];
                        stop++;
                    } while (c != '\0' && c != ' ' && c != '\n');
                    if (c == '\0') {
                        break;
                    }
                   // memset(tmp,'\0',len);
                   // strncpy(tmp, buffer.data() + start, (size_t) (stop - start));
                    spase--;
                }
                memset(tmp,'\0',len);
                strncpy(tmp, buffer.data() + start, (size_t) (stop - start));
                _rps = atol(tmp)-1;
                free(tmp);
            }else {
                _rps = atol(buffer.data());
            }
        } else if (_server_active) {
            timeUpdate();
            cout << "server stop: " << ctime(&_sys_time) << endl;
            _rps = 0;
            _rps_old = 0;
            _server_active = false;
        }
        curl_easy_cleanup(curl);
        buffer.clear();
      //  _rps = this->_net_receive;
    }
}


void Info::memUpdate() {
    std::ifstream stat_file("/proc/meminfo");
    if (!stat_file.is_open()){
        std::cout << "Unable to open /proc/meminfo" << std::endl;
        return;
    }
    char _tmp[50];
    stat_file >> _tmp;
    stat_file >> _tmp;
    _mem_total =  atol(_tmp);
    _mem_free = 0;
    for (int i = 0 ; i < 3 ; i++) {
        stat_file >> _tmp; stat_file>> _tmp; stat_file >> _tmp;
        _mem_free += atol(_tmp);
    }
    stat_file.close();
}

void Info::infoUpdate() {
    const size_t len_buffer=1024;
    const char json[] = "{%s, %s, %s, %s, %s }";
    char cpu_json[len_buffer] = {'\0'};
    char mem_json[len_buffer] = {'\0'};
    char net_json[len_buffer] = {'\0'};
    char rps_json[len_buffer] = {'\0'};
    char time_json[len_buffer] = {'\0'};
    sprintf(cpu_json,"\"cpu\":{\"busy\":%ld,\"work\":%ld}",_cpu_busy-_cpu_busy_old,_cpu_work-_cpu_work_old);
    sprintf(rps_json,"\"rps\": %lu",_rps-_rps_old);
    sprintf(mem_json,"\"mem\":{\"free\":%ld,\"total\":%ld}",_mem_free,_mem_total);
    sprintf(net_json,"\"net\":{\"receive\":%ld, \"transmit\":%ld}",_net_receive_old-_net_receive,-_net_transmit_old+_net_transmit);
    sprintf(time_json,"\"time\":%ld" ,_sys_time);
    if (!_info)
        free (_info);
    _info = (char*) malloc(strlen(cpu_json)+strlen(json)+strlen(mem_json)+strlen(net_json)+strlen(rps_json)+strlen(time_json)+1);
    sprintf(_info,json,cpu_json,mem_json,net_json,rps_json,time_json);
}

Info::Info() {
    _cpu_busy = 0;
    _cpu_work = 0;
    _mem_free = 0;
    _mem_total = 0;
    _net_receive = 0;
    _net_transmit = 0;
    _rps = 0;
    _sys_time = 0;
    _rps_location = nullptr;
    _info = nullptr;
    _server_active = false;
    _flag=0;
}

Info::~Info() {
    if(!_rps_location)
        free(_rps_location);
    if (!_info)
        free (_info);
    _rps_location= nullptr;
    _info = nullptr;
}

char* Info::getInfo() {
    return _info;
}

void Info::update() {
    cpuUpdate();
    rpsUpdate();
    memUpdate();
    netUpdate();
    timeUpdate();
    infoUpdate();
}

void Info::set_flag(int _flag) {
    Info::_flag = _flag;
}
