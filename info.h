
#ifndef UNTITLED_INFO_H
#define UNTITLED_INFO_H
class Info{
private:
    long _cpu_busy ;
    long _cpu_work ;
    long _cpu_busy_old ;
    long _cpu_work_old ;

    long _mem_free ;
    long _mem_total ;

    long _net_receive;
    long _net_transmit;
    long _net_receive_old;
    long _net_transmit_old;

    long _rps;
    long _rps_old;

    long _sys_time;

    char * _rps_location;

    char * _info;
    int _flag;

    bool _server_active;

    void cpuUpdate();
    void memUpdate();
    void netUpdate();
    void rpsUpdate();
    void timeUpdate();
    void infoUpdate();
public:
    Info();
    virtual ~Info();
    void update();
    void set_rps_location(const char *_rps_location);

    void set_flag(int _flag);

    char * getInfo();
};


#endif //UNTITLED_INFO_H
