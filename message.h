#ifndef __MESSAGE__
#define __MESSAGE__

#define MAXLEN  4096 
typedef struct  Message message;

struct Message{
    char type;
    char info[MAXLEN];
};

#endif