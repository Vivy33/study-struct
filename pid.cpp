/* 解析/proc/pid/maps
输入一个pid，构建一个数据结构


支持通过pid和地址查找他属于哪一行
      start        end       flags  offset                                       filename
// 5614d857b000-5614d8580000 r-xp 00002000 08:20 1663                       /usr/bin/cat
struct vma {
	size_t start;
	size_t end;
	int flags;
	size_t offset;
	std::string filename;
}

输入pid，构建processes-vma-maps
map1<pid, vma-maps>
map2<addr, vma>

传入pid和addr, 查找vma信息
允许你使用stl
你可以使用std::map*/


//用于解析和处理 Linux 系统中某个进程的虚拟内存区域 (VMA)。它读取 /proc/[pid]/maps 文件，提取每个 VMA 的信息，并允许你查询某个特定地址所属的 VMA。


#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <iomanip>
#include <unistd.h>
#include <utility> // for std::move

// 定义 VMA 结构体
struct VMA {
    size_t start;
    size_t end;
    int flags;
    size_t offset;
    std::string filename;
};

// 定义常量
const int READ = 4;
const int WRITE = 2;
const int EXECUTE = 1;

// 解析标记
unsigned int parse_flags(const std::string& flags) {
    unsigned int result = 0;
    if (flags.size() != 4) return result;

    if (flags[0] == 'r') result |= READ; 
    if (flags[1] == 'w') result |= WRITE; 
    if (flags[2] == 'x') result |= EXECUTE; 

    return result;
}

// 解析单行
VMA parse_line(const std::string& line) {
    std::istringstream iss(line);
    std::string start_end, flags, offset, dev, inode, pathname;

    // 使用 getline 分割字段
    std::getline(iss, start_end, ' ');
    std::getline(iss, flags, ' ');
    std::getline(iss, offset, ' ');
    std::getline(iss, dev, ' ');
    std::getline(iss, inode, ' ');
    std::getline(iss, pathname);

    size_t dash_pos = start_end.find('-');
    if (dash_pos == std::string::npos) {
        throw std::runtime_error("Invalid format: missing '-' in start-end range.");
    }
    
    size_t start = std::stoull(start_end.substr(0, dash_pos), nullptr, 16);
    size_t end = std::stoull(start_end.substr(dash_pos + 1), nullptr, 16);
    int parsed_flags = parse_flags(flags);
    size_t parsed_offset = std::stoull(offset, nullptr, 16);

    return { start, end, parsed_flags, parsed_offset, pathname };
}

// 解析 /proc/[pid]/maps 文件
void parse_maps_file(int pid, std::vector<VMA>& vmas, std::map<size_t, const VMA*>& addr_vma_map) {
    printf("解析 PID 为 %d 的 maps 文件\n", pid);
    std::ifstream maps_file("/proc/" + std::to_string(pid) + "/maps");
    std::string line;

    while (std::getline(maps_file, line)) {
        printf("读取行: %s\n", line.c_str());
        VMA vma = parse_line(line);
        printf("解析到的 VMA: start=0x%lx, end=0x%lx, flags=%d, offset=0x%lx, filename=%s\n", 
               vma.start, vma.end, vma.flags, vma.offset, vma.filename.c_str());

        // 使用std::move来避免拷贝
        vmas.push_back(std::move(vma));

        // 使用指向 vma 的指针来避免数据重复
        addr_vma_map[vmas.back().start] = &vmas.back();
    }
}

// 查找 VMA
bool find_vma(size_t addr, const std::map<size_t, const VMA*>& addr_vma_map, const VMA** vma) {
    printf("搜索地址 0x%lx\n", addr);
    auto it = addr_vma_map.upper_bound(addr);
    if (it != addr_vma_map.begin()) {
        --it;
        if (it->second->start <= addr && addr < it->second->end) {
            printf("找到地址 0x%lx 的 VMA\n", addr);
            *vma = it->second;
            return true; // 成功找到
        }
    }
    return false; // 找不到
}

// 打印 VMA 信息
void print_vma_info(const VMA& vma) {
    printf("VMA 信息:\n");
    printf("起始地址: 0x%lx\n", vma.start);
    printf("结束地址: 0x%lx\n", vma.end);
    printf("标记: %d\n", vma.flags);
    printf("偏移: 0x%lx\n", vma.offset);
    printf("文件名: %s\n", vma.filename.c_str());
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("用法: %s <pid> <地址>\n", argv[0]);
        return 1;
    }

    int pid = std::stoi(argv[1]); // std::stoi 字符串转换为整数
    size_t addr = std::stoull(argv[2], nullptr, 16);

    std::vector<VMA> vmas;
    std::map<size_t, const VMA*> addr_vma_map;

    parse_maps_file(pid, vmas, addr_vma_map);

    const VMA* vma;
    if (find_vma(addr, addr_vma_map, &vma)) {
        print_vma_info(*vma);
    } else {
        printf("错误: 找不到给定地址的 VMA。\n");
        return 1; // 返回错误状态码
    }

    return 0;
}
