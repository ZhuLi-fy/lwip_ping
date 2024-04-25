#ifndef __PING_H
#define __PING_H

#ifdef __cplusplus
extern "C" {
#endif
#include "lwip/ip_addr.h"

const int ping_version = 1;
typedef void (*ping_result)(int state,uint8_t ping_seq_num,uint32_t time);

void ping_init(ping_result re,uint8_t *stopCondition,ip_addr_t ip);

#ifdef __cplusplus
}
#endif
#endif