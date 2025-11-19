#ifndef UTIL_H
#define UTIL_H

#include "types.h"

// 12비트 VA를 9비트 VPN과 3비트 Offset, 그리고 3개의 3비트 인덱스로 분리하는 함수
void split_va(uint16_t va, uint16_t *vpn, uint16_t *offset, 
              uint8_t *pd1_idx, uint8_t *pd2_idx, uint8_t *pt_idx);

#endif