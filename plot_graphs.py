import matplotlib.pyplot as plt
import sys
import re

# 설정: 실험 케이스 이름
CASES = ["uniform", "zipf_05", "zipf_10", "zipf_15"]
LABELS = ["Uniform", "Zipf (s=0.5)", "Zipf (s=1.0)", "Zipf (s=1.5)"]
TOTAL_ACCESSES = 10000  # 실험 당 총 접근 횟수

def parse_input_frequency(filename):
    """입력 파일(.in)을 읽어 페이지(VPN)별 접근 빈도를 계산합니다."""
    vpn_counts = {}
    try:
        with open(filename, 'r') as f:
            lines = f.readlines()
            # 첫 줄(총 개수) 제외하고 읽기
            for line in lines[1:]:
                line = line.strip()
                if not line: continue
                va = int(line, 16)
                vpn = va >> 3  # Offset 3비트 제외 (12bit VA -> 9bit VPN)
                vpn_counts[vpn] = vpn_counts.get(vpn, 0) + 1
    except FileNotFoundError:
        print(f"Error: File {filename} not found.")
        return None
        
    # VPN 0~511까지 리스트로 변환 (없는 페이지는 0)
    counts = [vpn_counts.get(i, 0) for i in range(512)]
    return counts

def parse_log_metrics(filename):
    """로그 파일(.log)을 읽어 TLB Miss와 Page Fault 횟수를 셉니다."""
    tlb_miss_count = 0
    page_fault_count = 0
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                if "TLB Miss" in line:
                    tlb_miss_count += 1
                if "Page Table Miss" in line:
                    page_fault_count += 1
    except FileNotFoundError:
        print(f"Error: File {filename} not found.")
        return 0, 0

    return tlb_miss_count, page_fault_count

def plot_access_frequency():
    """1. 페이지 접근 빈도 그래프 그리기 (4개 서브플롯)"""
    fig, axs = plt.subplots(2, 2, figsize=(12, 8))
    fig.suptitle('Page Access Frequency Analysis', fontsize=16)
    
    plots = axs.flatten()
    
    for i, case in enumerate(CASES):
        filename = f"{case}.in"
        counts = parse_input_frequency(filename)
        
        if counts:
            # X축: VPN Index (0~511), Y축: Frequency
            plots[i].bar(range(512), counts, color='skyblue', width=1.0)
            plots[i].set_title(LABELS[i])
            plots[i].set_xlabel('Page Number (VPN)')
            plots[i].set_ylabel('Frequency')
            plots[i].set_xlim(0, 512)
            # Zipfian 분포 특성이 잘 보이도록 Y축 범위 조정 가능
            
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig('page_frequency_graphs.png')
    print("[Success] Created 'page_frequency_graphs.png'")

def plot_performance_comparison():
    """2. 성능 비교 그래프 그리기 (TLB Miss Rate, Page Fault Rate)"""
    tlb_miss_rates = []
    page_fault_rates = []
    
    for case in CASES:
        filename = f"{case}.log"
        tm, pf = parse_log_metrics(filename)
        
        # Rate 계산: (Count / Total Accesses) * 100 (%)
        # 과제 요건: "re-accessed addresses should be excluded"
        # -> 분모를 시뮬레이터 내부 접근 횟수가 아닌, 원본 요청 횟수(10,000)로 고정하면 해결됨.
        tlb_miss_rates.append((tm / TOTAL_ACCESSES) * 100)
        page_fault_rates.append((pf / TOTAL_ACCESSES) * 100)
    
    # 그래프 그리기
    x = range(len(CASES))
    width = 0.35
    
    fig, ax = plt.subplots(figsize=(10, 6))
    bar1 = ax.bar([i - width/2 for i in x], tlb_miss_rates, width, label='TLB Miss Rate')
    bar2 = ax.bar([i + width/2 for i in x], page_fault_rates, width, label='Page Fault Rate')
    
    ax.set_xlabel('Input Distribution')
    ax.set_ylabel('Rate (%)')
    ax.set_title('LRU Simulator Performance Comparison')
    ax.set_xticks(x)
    ax.set_xticklabels(LABELS)
    ax.legend()
    
    # 값 표시
    for bars in [bar1, bar2]:
        for bar in bars:
            height = bar.get_height()
            ax.annotate(f'{height:.1f}%',
                        xy=(bar.get_x() + bar.get_width() / 2, height),
                        xytext=(0, 3),
                        textcoords="offset points",
                        ha='center', va='bottom')

    plt.tight_layout()
    plt.savefig('performance_comparison.png')
    print("[Success] Created 'performance_comparison.png'")

if __name__ == "__main__":
    plot_access_frequency()
    plot_performance_comparison()