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
#include <stdexcept>

struct vma {
    uint64_t start;
    uint64_t end;
    uint32_t offset;
    uint32_t flags; // rwx
    std::string name;
    bool valid = true; // 是否为有效的vma
};

struct process {
    int pid;
    std::map<uint64_t, vma> vmas;
    bool valid = true; // 是否为有效的进程
};

struct system_info {
    std::map<int, process> procs; // pid为key, process为value的map
};

const int READ = 4;
const int WRITE = 2;
const int EXECUTE = 1;

unsigned int parse_flags(const std::string& flags) {
    unsigned int result = 0;
    if (flags.size() != 4) return result;

    if (flags[0] == 'r') result |= READ;
    if (flags[1] == 'w') result |= WRITE;
    if (flags[2] == 'x') result |= EXECUTE;

    return result;
}

vma parse_line(const std::string& line) {
    std::istringstream iss(line);
    std::string start_end, flags, offset, dev, inode, pathname;

    // 使用 getline 分割字段
    std::getline(iss, start_end, ' ');
    std::getline(iss, flags, ' ');
    std::getline(iss, offset, ' ');
    std::getline(iss, dev, ' ');
    std::getline(iss, inode, ' ');
    std::getline(iss, pathname);

    if (!pathname.empty() && pathname[0] == ' ') {
        pathname.erase(0, 1); // 去除前导空格
    }

    size_t dash_pos = start_end.find('-');
    if (dash_pos == std::string::npos) {
        throw std::runtime_error("Invalid format: missing '-' in start-end range.");
    }

    uint64_t start = std::stoull(start_end.substr(0, dash_pos), nullptr, 16);
    uint64_t end = std::stoull(start_end.substr(dash_pos + 1), nullptr, 16);
    uint32_t parsed_flags = parse_flags(flags);
    uint32_t parsed_offset = std::stoull(offset, nullptr, 16);

    return { start, end, parsed_offset, parsed_flags, pathname, true };
}

void load_process_vmas(int pid, std::map<uint64_t, vma>& vmas) {
    std::ifstream maps_file("/proc/" + std::to_string(pid) + "/maps");
    if (!maps_file.is_open()) {
        throw std::runtime_error("无法打开文件: /proc/" + std::to_string(pid) + "/maps");
    }

    std::string line;
    while (std::getline(maps_file, line)) {
        vma vma_info = parse_line(line);
        if (vma_info.valid) {
            vmas[vma_info.start] = std::move(vma_info);
        }
    }
}

process* create_process(system_info& system_info, int pid) {
    if (system_info.procs.find(pid) != system_info.procs.end()) {
        return &system_info.procs[pid];
    }

    process proc;
    proc.pid = pid;
    try {
        load_process_vmas(pid, proc.vmas);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        proc.valid = false;
    }
    system_info.procs[pid] = std::move(proc);
    return &system_info.procs[pid];
}

process* find_process(system_info& system_info, int pid) {
    auto it = system_info.procs.find(pid);
    if (it != system_info.procs.end()) {
        return &it->second;
    }
    return nullptr;
}

vma* find_vma(process& proc, uint64_t addr) {
    auto it = proc.vmas.upper_bound(addr);
    if (it != proc.vmas.begin()) {
        --it;
        if (it->second.start <= addr && addr < it->second.end) {
            return &it->second;
        }
    }
    return nullptr;
}

void print_vma_info(const vma& vma_info) {
    printf("VMA 信息:\n");
    printf("起始地址: 0x%lx\n", vma_info.start);
    printf("结束地址: 0x%lx\n", vma_info.end);
    printf("标记: %d\n", vma_info.flags);
    printf("偏移: 0x%lx\n", vma_info.offset);
    printf("文件名: %s\n", vma_info.name.c_str());
}

// API接口实现
bool get_process_vma_info(system_info& system_info, int pid, uint64_t addr, vma& out_vma) {
    process* proc = create_process(system_info, pid);

    if (!proc || !proc->valid) {
        return false;
    }

    vma* vma_info = find_vma(*proc, addr);
    if (vma_info && vma_info->valid) {
        out_vma = *vma_info;
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("用法: %s <pid> <地址>\n", argv[0]);
        return 1;
    }

    int pid = std::stoi(argv[1]);
    uint64_t addr = std::stoull(argv[2], nullptr, 16);

    system_info system_info;
    vma vma_info;
    if (get_process_vma_info(system_info, pid, addr, vma_info)) {
        print_vma_info(vma_info);
    } else {
        printf("错误: 找不到给定地址的 VMA。\n");
        return 1;
    }

    return 0;
}
