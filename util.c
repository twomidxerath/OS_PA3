#include "util.h"
#include "types.h"
#include <stdio.h>

/*
 * 12-bit VA 구조 (3-level paging):
 * [11-9] PD1 Index (3 bits)
 * [8-6] PD2 Index (3 bits)
 * [5-3] PT Index (3 bits)
 * [2-0] Offset (3 bits)
 * VPN = PD1 | PD2 | PT (9 bits)
 */
void split_va(uint16_t va, uint16_t *vpn, uint16_t *offset, 
              uint8_t *pd1_idx, uint8_t *pd2_idx, uint8_t *pt_idx) {
    
    // 1. Offset (3 bits) 추출: VA의 하위 3비트
    *offset = va & ((1 << OFFSET_BITS) - 1); // 0b111 = 0x7

    // 2. VPN (9 bits) 추출: VA를 3비트 오른쪽으로 시프트
    *vpn = va >> OFFSET_BITS;
    
    // 3. 3단계 인덱스 추출 (VPN을 세 부분으로 분리)
    
    // PT Index (Page Table Index): VPN의 하위 3비트
    *pt_idx = *vpn & ((1 << PT_BITS) - 1); // 0b111 = 0x7
    
    // PD2 Index (Level 2 Page Directory Index): VPN을 3비트 시프트 후 하위 3비트
    *pd2_idx = (*vpn >> PT_BITS) & ((1 << PD2_BITS) - 1); // 0b111 = 0x7
    
    // PD1 Index (Root Page Directory Index): VPN을 6비트 시프트 후 하위 3비트 (12-bit VA의 최상위 3비트)
    *pd1_idx = (*vpn >> (PT_BITS + PD2_BITS)) & ((1 << PD1_BITS) - 1); // 0b111 = 0x7
}

// PTE에서 Present Bit과 PFN을 추출하는 유틸리티 함수도 여기에 추가할 수 있습니다.
uint8_t get_pte_pfn(PTE pte) {
    // 1-bit Present + 7-bit PFN
    return pte.entry_value & 0x7F; // PFN Mask 0x7F (0b01111111)
}

uint8_t get_pte_present(PTE pte) {
    return (pte.entry_value & 0x80) >> 7; // Present Mask 0x80 (0b10000000)
}

// PTE에 PFN을 설정하고 Present Bit을 1로 설정하는 함수
void set_pte_entry(PTE *pte, uint8_t pfn) {
    // PFN이 7비트 범위를 초과하는지 확인하는 로직을 추가하는 것이 안전합니다.
    if (pfn >= NUM_FRAMES) {
        fprintf(stderr, "Error: Invalid PFN %u\n", pfn);
        return;
    }
    // Present Bit (0x80) + PFN (pfn & 0x7F)
    pte->entry_value = 0x80 | (pfn & 0x7F); 
}

// PTE의 Present Bit을 0으로 설정하는 함수
void clear_pte_present(PTE *pte) {
    pte->entry_value &= 0x7F; // Present Bit을 0으로 클리어
}