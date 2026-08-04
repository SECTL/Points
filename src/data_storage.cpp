// 业务存储层：把 FileInstance 引擎接到 data_storage.h 的字段级 API
// 班级语义：read::xxx(class) 激活班级（学生走 FileInstance 单例，rules/gifts 走独立表）；
//           write::xxx 作用于当前激活班级（无班级参数的签名按当前班级处理）
// 注意：include 必须放在 import 之前——MSVC 下消费 TU 在 import 后再 include 模块全局片段已含
// 的 STL 头（<string>/<fstream> 等）会触发 C2572 默认模板参数重复定义
#include "data_storage.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

import storage;

namespace {

std::string path_for(const std::string& class_name, const char* stem) {
    if (class_name.empty() || class_name == "default")
        return std::string(stem) + ".dat";
    return std::string(stem) + "_" + class_name + ".dat";
}

// 学生引擎：FileInstance 单例，按班级映射文件路径
points::FileInstance& ensure_students(const std::string& class_name) {
    auto path = path_for(class_name, "students");
    try {
        auto& fi = points::FileInstance::get();
        if (fi.path() != path)
            fi.switch_to(path);          // 切班级：脏则自动落盘
        return fi;
    } catch (const std::logic_error&) {  // 未初始化
        points::FileInstance::init(path);
        return points::FileInstance::get();
    }
}

// UTF-8 安全截断：不超过 max_bytes，且不在多字节序列中间切断
void copy_utf8_truncated(std::byte* dest, size_t max_bytes, const std::string& src) {
    size_t n = std::min(src.size(), max_bytes);
    if (n < src.size()) {                // 需要截断：回退到完整字符边界
        while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0u) == 0x80u)
            --n;
    }
    std::memcpy(dest, src.data(), n);
    std::memset(dest + n, 0, max_bytes - n);
}

// —— rules / gifts：独立表（管道文本，仿旧 rules.dat；迁二进制引擎后统一走 FileInstance）——
struct Entry { std::string desc; int delta; };
using Table = std::vector<Entry>;

struct Tables {
    std::map<std::string, Table> rules;
    std::map<std::string, Table> gifts;
};
Tables& tables() { static Tables t; return t; }

Table load_table(const std::string& path) {
    Table t;
    std::ifstream fin(path);
    std::string line;
    while (std::getline(fin, line)) {
        auto pos = line.find_last_of('|');
        if (pos == std::string::npos || pos + 1 >= line.size())
            continue;                    // 坏行跳过（宽松读旧数据）
        Entry e;
        e.desc = line.substr(0, pos);
        e.delta = std::stoi(line.substr(pos + 1));
        t.push_back(std::move(e));
    }
    return t;
}

void save_table(const std::string& path, const Table& t) {
    std::ofstream fout(path);            // 文本表暂不原子写，迁引擎后统一
    for (const auto& e : t)
        fout << e.desc << "|" << e.delta << "\n";
}

std::string current_class = "default";   // read::xxx 激活，write::xxx 消费

} // namespace

namespace read {

void students(const std::string& class_name) {
    current_class = class_name;
    ensure_students(class_name);         // 加载即激活
}

void rules(const std::string& class_name) {
    current_class = class_name;
    tables().rules[class_name] = load_table(path_for(class_name, "rules"));
}

void gifts(const std::string& class_name) {
    current_class = class_name;
    tables().gifts[class_name] = load_table(path_for(class_name, "gifts"));
}

std::map<std::string, std::string> config(const std::string& /*class_name*/) {
    std::map<std::string, std::string> kv;
    std::ifstream fin("config.ini");
    std::string line;
    while (std::getline(fin, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        kv[line.substr(0, pos)] = line.substr(pos + 1);
    }
    return kv;
}

} // namespace read

namespace write {
namespace student {

// 签名歧义：header 只声明了 bool id()，按"新建一个空学生并返回是否成功"实现
bool id() {
    ensure_students(current_class).add();
    return true;
}

bool name(int id, const std::string& new_name) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    copy_utf8_truncated(r->name, sizeof(r->name), new_name);
    fi.mark_dirty();
    return true;
}

bool gender(int id, const std::string& new_gender) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    copy_utf8_truncated(r->gender, sizeof(r->gender), new_gender);
    fi.mark_dirty();
    return true;
}

bool old_score(int id, const long long& new_old_score) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    r->old_score = new_old_score;
    fi.mark_dirty();
    return true;
}

bool score(int id, const long long& new_score) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    r->score = new_score;
    fi.mark_dirty();
    return true;
}

bool old_rank(int id, const int& new_old_rank) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    r->old_rank = new_old_rank;
    fi.mark_dirty();
    return true;
}

bool rank(int id, const int& new_rank) {
    auto& fi = ensure_students(current_class);
    auto* r = fi.find(id);
    if (!r) return false;
    r->rank = new_rank;
    fi.mark_dirty();
    return true;
}

} // namespace student

namespace rule {

bool desc(int rule_num, const std::string& new_desc) {
    auto& t = tables().rules[current_class];
    if (rule_num < 1 || rule_num > static_cast<int>(t.size())) return false;
    t[rule_num - 1].desc = new_desc;
    save_table(path_for(current_class, "rules"), t);
    return true;
}

bool delta(int rule_num, const int& new_delta) {
    auto& t = tables().rules[current_class];
    if (rule_num < 1 || rule_num > static_cast<int>(t.size())) return false;
    t[rule_num - 1].delta = new_delta;
    save_table(path_for(current_class, "rules"), t);
    return true;
}

} // namespace rule

namespace gift {

bool desc(int gift_num, const std::string& new_desc) {
    auto& t = tables().gifts[current_class];
    if (gift_num < 1 || gift_num > static_cast<int>(t.size())) return false;
    t[gift_num - 1].desc = new_desc;
    save_table(path_for(current_class, "gifts"), t);
    return true;
}

bool delta(int gift_num, const int& new_delta) {
    auto& t = tables().gifts[current_class];
    if (gift_num < 1 || gift_num > static_cast<int>(t.size())) return false;
    t[gift_num - 1].delta = new_delta;
    save_table(path_for(current_class, "gifts"), t);
    return true;
}

} // namespace gift
} // namespace write
