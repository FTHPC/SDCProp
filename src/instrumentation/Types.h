#ifndef TYPES_H
#define TYPES_H
typedef enum {
    UNKNOWN = -1,
    INT8 = 0,
    INT16,
    INT32,
    INT64,
    FLOAT,
    DOUBLE,
    PTR
} TYPES;

static const char* TYPE_STR[8] = {"UNKNOWN",
    "INT8", 
    "INT16",
    "INT32",
    "INT64",
    "FLOAT",
    "DOUBLE",
    "PTR"};

static const unsigned char BYTES[7] = { sizeof(unsigned char),
    sizeof(unsigned short), sizeof(unsigned int), sizeof(unsigned long),
    sizeof(float), sizeof(double), sizeof(void*)};
//static const unsigned char BYTES[7] = { sizeof(char), sizeof(short), sizeof(unsigned), sizeof(long), sizeof(float), sizeof(double), sizeof(void*)};
#endif
