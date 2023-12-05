#ifndef __CLIENT_H__
#define __CLIENT_H__

class sock{
private:
    sock(){}    

    bool connected;

public:
    void connect();
    void close();
    void send();
};

class client{
private:
    client();
    sock socket;
public:
    
    void run();
};

#endif