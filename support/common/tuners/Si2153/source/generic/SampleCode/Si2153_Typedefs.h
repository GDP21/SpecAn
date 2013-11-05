#ifndef TYPEDEFS_H
#define TYPEDEFS_H
/* STATUS structure definition */
  typedef struct { /* Si2153_COMMON_REPLY_struct */
    unsigned char   tunint;
    unsigned char   atvint;
    unsigned char   dtvint;
    unsigned char   err;
    unsigned char   cts;
 }  Si2153_COMMON_REPLY_struct;

typedef struct L0_Context {
    unsigned char   address;
    int             indexSize;
} L0_Context;

typedef struct {
    L0_Context *i2c;
} L1_Si2153_Context;





#endif
