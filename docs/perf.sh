#!/bin/bash

# 设置输出目录
OUTPUT_DIR="perf_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p $OUTPUT_DIR

# 颜色函数
print_color() {
    echo -e "\033[1;34m[性能分析] $1\033[0m"
}

# 记录性能数据 (如果需要重新收集)
record_perf_data() {
    print_color "正在记录性能数据..."
    perf record -F 9999 \
        -e cpu-cycles,instructions \
        -e cache-references,cache-misses \
        -e branch-instructions,branch-misses \
        -g -- build/fusellm -m /tmp/llm -c .settings.toml
    print_color "性能数据已保存到 perf.data"
}

# 生成火焰图
generate_flamegraph() {
    print_color "生成火焰图..."
    perf script -i perf.data | stackcollapse-perf.pl | flamegraph.pl > "$OUTPUT_DIR/flamegraph.svg"
    print_color "火焰图已保存到 $OUTPUT_DIR/flamegraph.svg"
}

# 分析各个线程的性能情况
analyze_threads() {
    print_color "分析线程性能..."
    perf report -i perf.data --sort pid,comm,dso,symbol > "$OUTPUT_DIR/threads.txt"
    print_color "线程性能分析已保存到 $OUTPUT_DIR/threads.txt"
}


# 主函数
main() {
    print_color "开始性能分析..."
    
    # 如果没有 perf.data 文件，则先记录性能数据
    if [ ! -f "perf.data" ]; then
        record_perf_data
    else
        print_color "使用现有的 perf.data 文件进行分析"
    fi
    
    # 生成各种分析报告
    generate_flamegraph
    analyze_threads
    
    print_color "性能分析完成！所有结果已保存到 $OUTPUT_DIR 目录"
}

# 执行主函数
main
