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
    
    // 1. Offset (3 bits) 추출
    *offset = va & ((1 << OFFSET_BITS) - 1); // 0x7

    // 2. VPN (9 bits) 추출
    *vpn = va >> OFFSET_BITS;
    
    // 3. 3단계 인덱스 추출
    
    // PT Index: VPN의 하위 3비트
    *pt_idx = *vpn & ((1 << PT_BITS) - 1); // 0x7
    
    // PD2 Index: VPN을 3비트 시프트 후 하위 3비트
    *pd2_idx = (*vpn >> PT_BITS) & ((1 << PD2_BITS) - 1); // 0x7
    
    // PD1 Index: VPN을 6비트 시프트 후 하위 3비트
    *pd1_idx = (*vpn >> (PT_BITS + PD2_BITS)) & ((1 << PD1_BITS) - 1); // 0x7
}

// PTE에서 PFN 추출 (7 bits)
uint8_t get_pte_pfn(PTE pte) {
    return pte.entry_value & 0x7F; // PFN Mask 0x7F
}

// PTE에서 Present Bit 추출 (1 bit)
uint8_t get_pte_present(PTE pte) {
    return (pte.entry_value & 0x80) >> 7; // Present Mask 0x80
}

// PTE에 PFN을 설정하고 Present Bit을 1로 설정
void set_pte_entry(PTE *pte, uint8_t pfn) {
    if (pfn >= NUM_FRAMES) {
        fprintf(stderr, "Error: Invalid PFN %u\n", pfn);
        return;
    }
    // Present Bit (0x80) + PFN
    pte->entry_value = 0x80 | (pfn & 0x7F); 
}

// PTE의 Present Bit을 0으로 설정
void clear_pte_present(PTE *pte) {
    pte->entry_value &= 0x7F; // Present Bit을 0으로 클리어
}