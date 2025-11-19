#ifndef UTIL_H
#define UTIL_H

#include "types.h"

// 12비트 VA를 9비트 VPN과 3비트 Offset, 그리고 3개의 3비트 인덱스로 분리하는 함수
void split_va(uint16_t va, uint16_t *vpn, uint16_t *offset, 
              uint8_t *pd1_idx, uint8_t *pd2_idx, uint8_t *pt_idx);

// PTE 비트 조작 및 정보 추출 함수 (새로 추가됨)
uint8_t get_pte_pfn(PTE pte);
uint8_t get_pte_present(PTE pte);
void set_pte_entry(PTE *pte, uint8_t pfn);
void clear_pte_present(PTE *pte);

#endif