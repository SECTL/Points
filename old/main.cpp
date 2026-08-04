/*
    班级积分管理系统。在班级一体机上安装这个软件，用积分和学习小组激励同学们上进。Class Points Management System. Install this software on the computer in the classroom. Then use points and study groups to motivate students. 
    Copyright (C) 2025  ShiMingXuanSimon

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <climits>
#include <sstream>
#include <map>
#include <cstdint>
#include <ctime>
#include <windows.h>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;

// 设置控制台编码为中文GBK
void setConsoleEncoding() {
    SetConsoleOutputCP(936);  // GBK编码
    SetConsoleCP(936);        // 输入也使用GBK
}

struct Student {
    int id;
    string name;
    string gender;
    long long old_score, score;
    int old_rank, rank;
};

struct Rule {
    string desc;
    int delta;
};

struct Version {
    int major, minor, patch;
};

vector<Student> students;
vector<Rule> rules;

void log(const string& message, const string& level = "INFO") {
    static const string log_filename = "./" + to_string(time(nullptr)) + ".log";

    if (level == "DEBUG") {
        # ifndef _DEBUG
        return; // 在非调试模式下不输出DEBUG日志
        # endif
    }
    time_t nowtime;
    time(&nowtime);
    struct tm* timeinfo = localtime(&nowtime);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    ofstream fout(log_filename, ios::app); 
    fout << "[" << level << "] " << buffer << ": " << message << endl;
}

// -------------------- 网络请求模块 --------------------
size_t write_callback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

// 专门给下载用的文件写入回调（普通函数，不搞 Lambda）
size_t write_file_callback(void* contents, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(contents, size, nmemb, stream);
}

string fetch_latest_release_json() {
    log("开始抓取最新版本信息", "INFO");

    CURL* curl = curl_easy_init();
    string response;
    
    if (curl) {
        // 设置 GitHub API 地址
        curl_easy_setopt(curl, CURLOPT_URL, 
            "https://api.github.com/repos/ShiMingXuanSimon/ClassScoreManageSystem/releases/latest");
        
        // 设置回调函数，让 libcurl 把收到的数据往 response 里塞
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // 临时关闭 SSL 证书验证（仅开发阶段，避免你折腾 CA 证书）
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        
        // GitHub API 强制要求 User-Agent，否则返回 403
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ClassManager/1.0");
        
        // 执行网络请求
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            log("网络请求失败: " + string(curl_easy_strerror(res)), "ERROR");
        }
        
        curl_easy_cleanup(curl);
    }
    log("抓取最新版本信息完成", "INFO");
    return response;
}

bool download_release_asset(const string& url, const string& save_path) {
    log("开始下载新版本安装包", "INFO");
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        log("初始化curl失败", "ERROR");
        return false;
    }

    FILE* fp = fopen(save_path.c_str(), "wb");
    if (!fp) {
        log("无法创建本地文件: " + save_path, "ERROR");
        curl_easy_cleanup(curl);
        return false;
    }

    log("配置curl参数", "INFO");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // 关键：跟随重定向
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ClassManager/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);           // 整个请求最多等 30 秒
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);    // 建立连接最多等 10 秒
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);           // 避免信号干扰（Windows 下必加）
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    log("配置curl参数完成", "INFO");

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log("下载失败: " + string(curl_easy_strerror(res)), "ERROR");
        return false;
    }
    log("下载成功: " + save_path, "INFO");
    return true;
}
//-------------------- 网络请求模块 结束 --------------------

Version parse_version(const string& ver_str) {
    Version v = {0, 0, 0};
    string clean = ver_str;
    
    // 去掉开头的 'v' 或 'V'
    if (!clean.empty() && (clean[0] == 'v' || clean[0] == 'V')) {
        clean = clean.substr(1);
    }

    // 按 '.' 分割
    stringstream ss(clean);
    string part;
    vector<int> parts;
    while (getline(ss, part, '.')) {
        parts.push_back(stoi(part));  // stoi 把字符串转数字
    }

    // 填充到结构体，不足的补 0（比如 "v1" 等同于 1.0.0）
    if (parts.size() > 0) v.major = parts[0];
    if (parts.size() > 1) v.minor = parts[1];
    if (parts.size() > 2) v.patch = parts[2];
    return v;
}

// 比较两个版本：返回 true 如果 a > b
bool is_version_greater(const Version& a, const Version& b) {
    if (a.major != b.major) return a.major > b.major;
    if (a.minor != b.minor) return a.minor > b.minor;
    return a.patch > b.patch;
}

string parse_version_number(string json_data){
    log("开始解析最新版本号", "INFO");
    string latest_tag;

    // string json_data = fetch_latest_release_json();
    if (!json_data.empty()) {
        try {
            auto json_obj = nlohmann::json::parse(json_data);
            latest_tag = json_obj["tag_name"];  // 提取 tag_name0 
            cout << "远程最新版本: " << latest_tag << endl;
            log("远程最新版本: " + latest_tag, "INFO");
        } catch (const exception& e) {
            log("解析JSON失败: " + string(e.what()), "ERROR");
        }
    }
    log("最新版本号解析完成", "INFO");
    return latest_tag;
}

string parse_download_url(string json_data){
    log("开始解析下载链接", "INFO");

    string download_url;
    if (!json_data.empty()) {
        try {
            auto json_obj = nlohmann::json::parse(json_data);
            download_url = json_obj["assets"][0]["browser_download_url"];
            log("下载链接: " + download_url, "INFO");
        } catch (const exception& e) {
            log("解析JSON失败: " + string(e.what()), "ERROR");
        }
    }
    log("下载链接解析完成", "INFO");
    return download_url;
}

string parse_asset_name(const string& json_data) {
    log("开始解析资产名称", "INFO");

    string asset_name;
    if (!json_data.empty()) {
        try {
            auto json_obj = nlohmann::json::parse(json_data);
            asset_name = json_obj["assets"][0]["name"];
            log("资产名称: " + asset_name, "INFO");
        } catch (const exception& e) {
            log("解析JSON失败: " + string(e.what()), "ERROR");
        }
    }
    log("资产名称解析完成", "INFO");
    return asset_name;
}

bool check_for_updates(const string& json_data) {
    log("开始检查更新", "INFO");
    
    bool is_not_latest = false;

    string latest_tag = parse_version_number(json_data);

    const string current_version = APP_VERSION;   // APP_VERSION 由 xmake 的 add_defines 传入
    cout << "当前本地版本: " << current_version << endl;
    log("当前本地版本: " + current_version, "INFO");

    // 解析并比较版本      
    Version remote_ver = parse_version(latest_tag);
    Version local_ver = parse_version(current_version);
                
    if (is_version_greater(remote_ver, local_ver)) {        
        cout << "发现新版本！" << endl;
        log("发现新版本" + latest_tag, "INFO");
        is_not_latest = true;

    } else {
        cout << "已是最新版本。" << endl;
    }

    log("检查更新完成", "INFO");
    return is_not_latest;
}

void update(const string& json_data){
    string asset_name = parse_asset_name(json_data);
    string download_url = parse_download_url(json_data);

    cout << "准备下载: " << asset_name << endl;

    // 调用下载函数（存到当前目录，文件名用原始名称）
    string save_path = "./" + asset_name;
    bool ok = download_release_asset(download_url, save_path);

    if (ok) {
        cout << "下载完成，文件保存在: " << save_path << endl;
    }
    else {
       cout << "下载失败，请查看日志。" << endl;
    }
}

// 文件操作函数
void load_students() {
    log("开始加载学生数据", "INFO");
    ifstream fin("students.dat");
    if (!fin) {
        log("未找到学生数据文件，将自动创建该文件", "WARNING");
        cout << "未找到学生数据文件，将自动创建该文件" << endl;
        return;
    }

    string line;
    while (getline(fin, line)) {
        size_t pos1 = line.find('|');
        if (pos1 == string::npos) {
            log("数据格式错误，请检查：" + line + "行", "WARNING");
            cout << "数据格式错误，请检查：" << line << "行" << endl;
            continue;
        }

        int id = stoi(line.substr(0, pos1));
        Student student;
        student.id = id;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == string::npos) {
            log("姓名字段格式错误，请检查：" + line + "行", "WARNING");
            cout << "姓名字段格式错误，请检查：" << line << "行" << endl;
            continue;
        }
        student.name = line.substr(pos1 + 1, pos2 - pos1 - 1);

        size_t pos3 = line.find('|', pos2 + 1);
        if (pos3 == string::npos) {
            log("性别字段格式错误，请检查：" + line + "行", "WARNING");
            cout << "性别字段格式错误，请检查：" << line << "行" << endl;
            continue;
        }
        student.gender = line.substr(pos2 + 1, pos3 - pos2 - 1);

        size_t pos4 = line.find('|', pos3 + 1);
        if (pos4 == string::npos || pos4 + 1 > line.length()) {
            log("旧积分字段格式错误，请检查：" + line + "行", "WARNING");
            cout << "旧积分字段格式错误，请检查：" << line << "行" << endl;
            continue;
        }
        student.old_score = stoll(line.substr(pos3 + 1, pos4 - pos3 - 1));

        size_t pos5 = line.find('|', pos4 + 1);
        if (pos5 == string::npos || pos5 + 1 > line.length()) {
            log("积分字段格式错误，请检查：" + line + "行", "WARNING");
            cout << "积分字段格式错误，请检查：" << line << "行" << endl;
            continue;
        }
        student.score = stoll(line.substr(pos4 + 1, pos5 - pos4 - 1));

        size_t pos6 = line.find('|', pos5 + 1);
        if (pos6 == string::npos || pos6 + 1 > line.length()) {
            log("旧排名字段格式错误，请检查：" + line + "行", "WARNING");
            cout << "旧排名字段格式错误，请检查：" << line << "行" << endl;
            continue;
        }
        student.old_rank = stoi(line.substr(pos5 + 1, pos6 - pos5 - 1));

        student.rank = stoi(line.substr(pos6 + 1));
        students.push_back(student);
    }
    fin.close();
    log("学生数据加载完成，共加载 " + to_string(students.size()) + " 条记录", "INFO");
}

void save_students() {
    log("开始保存学生数据", "INFO");
    ofstream fout("students.dat");
    for (const auto& student : students) {
        fout << student.id << "|"
             << student.name << "|"
             << student.gender << "|"
             << student.old_score << "|"
             << student.score << "|"
             << student.old_rank << "|"
             << student.rank << "\n";
    }
    fout.close();
    log("学生数据保存完成，共保存 " + to_string(students.size()) + " 条记录", "INFO");
}

void load_rules() {
    log("开始加载规则数据", "INFO");
    ifstream fin("rules.dat");
    if (!fin) {
        log("未找到规则文件，将自动创建该文件", "WARNING");
        cout << "未找到规则文件，将自动创建该文件" << endl;
        return;
    }

    string line;
    while (getline(fin, line)) {
        size_t pos = line.find_last_of('|');
        if (pos == string::npos || pos + 1 >= line.length()) {
            log("无效规则格式，请检查：" + line + "行", "WARNING");
            cout << "无效规则格式：" << line << "行" << endl;
            continue;
        }
        Rule r;
        r.desc = line.substr(0, pos);
        try {
            r.delta = stoi(line.substr(pos + 1));
        } catch (...) {
            log("分值转换错误，请检查：" + line + "行", "WARNING");
            cout << "分值转换错误：" << line << "行" << endl;
            continue;
        }
        rules.push_back(r);
    }
    fin.close();
    log("规则数据加载完成，共加载 " + to_string(rules.size()) + " 条记录", "INFO");
}

void save_rules() {
    log("开始保存规则数据", "INFO");
    ofstream fout("rules.dat");
    for (size_t i = 0; i < rules.size(); i++) {
        fout << rules[i].desc << "|" << rules[i].delta << endl;
    }
    fout.close();
    log("规则数据保存完成，共保存 " + to_string(rules.size()) + " 条记录", "INFO");
}

map<string, string> loadConfig(string filename) {
    log("开始加载配置文件" + filename, "INFO");
    map<string, string> config;

    ifstream file(filename);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != string::npos) {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 1);
                config[key] = value;
            }
        }
        file.close();
    }
    return config;
    log("配置文件" + filename + "加载完成", "INFO");
}

uint64_t hash_password(const string& str) {
    uint64_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
    }
    return hash;
}

// 核心功能函数
bool verify_password(uint64_t stored_hash) {
    log("请求输入密码进行验证", "INFO");
    cout << "请输入密码：";
    string input;
    getline(cin, input);
    system("cls");
    if (!hash_password(input) == stored_hash) log("密码验证失败", "INFO");

    log("返回密码验证结果", "INFO");
    return (hash_password(input) == stored_hash);
}

void show_rules() {
    cout << "\n当前规则列表：" << endl;
    for (size_t i = 0; i < rules.size(); i++) {
        cout << i + 1 << ". " << rules[i].desc
             << " (" << (rules[i].delta > 0 ? "+" : "")
             << rules[i].delta << "分)" << endl;
    }
}

void add_rule() {
    log("开始添加新规则", "INFO");
    Rule new_rule;
    cout << "请输入规则描述：";
    getline(cin, new_rule.desc);
    log("获取到规则描述输入", "INFO");

    cout << "请输入规则分值（减分请加负号）：";
    string delta_str;
    getline(cin, delta_str);
    try {
        new_rule.delta = stoi(delta_str);
    } catch (...) {
        log("无效的分值输入！", "WARNING");
        cout << "无效的分值输入！" << endl;
        return;
    }
    log("获取到规则分值输入", "INFO");

    rules.push_back(new_rule);
    cout << "规则添加成功！" << endl;
    log("新规则添加成功", "INFO");
}

void show_ranking() {
    vector<Student*> sorted;
    for (auto& student : students) {
        sorted.push_back(&student);
    }

    sort(sorted.begin(), sorted.end(), [](Student* a, Student* b) {
        if (a->score != b->score) return a->score > b->score;
        return a->id < b->id;
    });

    cout << "积分排行榜" << endl;
    cout << "排名\t学号\t姓名\t性别\t积分\t排名变动\t积分变动" << endl;
    int rank = 1;
    for (size_t i = 0; i < sorted.size(); i++) {
        if (i > 0 && sorted[i]->score != sorted[i - 1]->score) {
            rank = static_cast<int>(i + 1);
        }
        long long score_change = sorted[i]->score - sorted[i]->old_score;
        int rank_change = sorted[i]->old_rank - sorted[i]->rank;

        cout << rank << "\t"
             << sorted[i]->id << "\t"
             << (sorted[i]->name.empty() ? "未登记" : sorted[i]->name) << "\t"
             << (sorted[i]->gender.empty() ? "未知" : sorted[i]->gender) << "\t"
             << sorted[i]->score << "\t"
             << (rank_change > 0 ? "+" : "") << rank_change << "名\t"
             << (score_change >= 0 ? "+" : "") << score_change << "分" << endl;
    }
    save_students();
}

void show_students() {
    vector<Student*> sorted;
    for (auto& student : students) {
        sorted.push_back(&student);
    }

    sort(sorted.begin(), sorted.end(), [](Student* a, Student* b) {
        if (a->score != b->score) return a->score > b->score;
        return a->id < b->id;
    });

    cout << "学号\t姓名\t性别" << endl;
    for (const auto& studentPtr : sorted) {
        cout << studentPtr->id << "\t"
             << (studentPtr->name.empty() ? "未登记" : studentPtr->name) << "\t"
             << (studentPtr->gender.empty() ? "未知" : studentPtr->gender) << "\t" << endl;
    }
    save_students();
}

void togetherapply_rule() {
    log("开始批量修改积分", "INFO");
    if (rules.empty()) {
        cout << "当前没有规则！" << endl;
        return;
    }

    int rule_num;
    cout << "请选择要应用的规则号：";
    cin >> rule_num;
    cin.ignore();

    if (rule_num < 1 || rule_num > static_cast<int>(rules.size())) {
        log("无效的规则号输入：" + to_string(rule_num), "WARNING");
        cout << "无效的规则号！" << endl;
        return;
    }
    log("获取到规则号", "INFO");

    int stu_id;
    while (true) {
        static int count = 0;
        count++;

        cout << "\n[学号]. 修改积分 \n0. 返回\n请选择";
        cout << "请输入学生学号：";
        cin >> stu_id;
        cin.ignore();

        if (stu_id == 0){ 
            log("退出批量修改积分，共修改了 " + to_string(count) + " 个学生，应用规则：" + rules[rule_num].desc, "INFO");
            break;
        }

        auto it = find_if(students.begin(), students.end(), [stu_id](const Student& s) {
            return s.id == stu_id;
        });

        if (it == students.end()) {
            cout << "无效的学号！" << endl;
            continue;
        }

        it->score += rules[rule_num - 1].delta;
        cout << "修改成功，当前积分：" << it->score;

        show_students();
    }

}

void apply_rule() {
    log("开始应用规则", "INFO");
    if (rules.empty()) {
        log("当前没有规则！", "WARNING");
        cout << "当前没有规则！" << endl;
        return;
    }

    int rule_num;
    cout << "请选择要应用的规则号：";
    cin >> rule_num;
    cin.ignore();
    log("获取到规则号", "INFO");

    if (rule_num < 1 || rule_num > static_cast<int>(rules.size())) {
        log("无效的规则号输入：" + to_string(rule_num), "WARNING");
        cout << "无效的规则号！" << endl;
        return;
    }

    int stu_id;
    cout << "请输入学生学号：";
    cin >> stu_id;
    cin.ignore();

    auto it = find_if(students.begin(), students.end(), [stu_id](const Student& s) {
        return s.id == stu_id;
    });
    log("获取到学生学号", "INFO");

    if (it == students.end()) {
        log("无效的学号输入：" + to_string(stu_id), "WARNING");
        cout << "无效的学号！" << endl;
        return;
    }

    it->score += rules[rule_num - 1].delta;
    cout << "修改成功，当前积分：" << it->score;
    log("应用规则成功，规则：" + rules[rule_num - 1].desc, "INFO");
}

void modify_student() {
    log("开始修改学生信息", "INFO");
    int id;
    cout << "请输入学号：";
    cin >> id;
    cin.ignore();

    auto it = find_if(students.begin(), students.end(), [id](const Student& s) {
        return s.id == id;
    });
    log("获取到学生学号", "INFO");

    if (it == students.end()) {
        log("无效的学号输入：" + to_string(id), "WARNING");
        cout << "无效学号！" << endl;
        return;
    }

    cout << "当前信息：\n"
         << "姓名：" << (it->name.empty() ? "未登记" : it->name) << "\n"
         << "性别：" << (it->gender.empty() ? "未知" : it->gender) << "\n"
         << "积分：" << it->score << endl;

    cout << "请输入新姓名（直接回车保留原姓名）：";
    string name;
    getline(cin, name);
    if (!name.empty()) it->name = name;
    log("获取到学生新姓名", "INFO");

    cout << "请输入新性别（直接回车保留原性别）：";
    string gender;
    getline(cin, gender);
    if (!gender.empty()) it->gender = gender;
    log("获取到学生新性别", "INFO");

    cout << "请输入新积分（直接回车保留原积分）：";
    string score_str;
    getline(cin, score_str);
    if (!score_str.empty()) {
        try {
            it->score = stoll(score_str);
        } catch (...) {
            log("无效的积分输入：" + score_str, "WARNING");
            cout << "无效的积分输入！" << endl;
        }
    }
    log("获取到学生新积分", "INFO");

    cout << "信息修改成功！" << endl;
    log("修改学生信息结束", "INFO");
}

void apply_group_scores() {
    log("尿袋开始","INFO");
    ifstream infile("group_scores.txt");
    if (!infile) {
        cout << "未找到小组加分文件，请生成小组加分文件" << endl;
        return;
    }

    string line;
    vector<int> student_ids;
    vector<int> add_scores;

    while (getline(infile, line)) {
        if (line.find("每人加分:") != string::npos) {
            size_t pos = line.find(":");
            int add_score = stoi(line.substr(pos + 1));

            if (getline(infile, line)) {
                if (line.find("成员学号:") != string::npos) {
                    size_t start = line.find(":") + 1;
                    string ids_str = line.substr(start);
                    stringstream ss(ids_str);
                    int id;

                    while (ss >> id) {
                        student_ids.push_back(id);
                        add_scores.push_back(add_score);
                    }
                }
            }
        }
    }
    infile.close();

    for (size_t i = 0; i < student_ids.size(); i++) {
        int id = student_ids[i];
        auto it = find_if(students.begin(), students.end(), [id](const Student& s) {
            return s.id == id;
        });

        if (it != students.end()) {
            it->score += add_scores[i];
            cout << "学号" << id << "(" << it->name
                 << ") 增加积分: " << add_scores[i]
                 << " 新积分: " << it->score << endl;
        }
    }

    save_students();
    cout << "小组加分成功添加！" << endl;
    log("尿袋结束","INFO");
}

void renew_score_and_rank() {
    log("开始更新分数和排名", "INFO");
    if (!students.empty()) {
        cout << "开始更新旧分数..." << endl;
        for (auto& student : students) {
            student.old_score = student.score;
        }
        log("旧分数更新完成", "INFO");

        cout << "开始更新旧排名..." << endl;
        for (auto& student : students) {
            student.old_rank = student.rank;
        }
        cout << "更新完成" << endl;
        log("旧排名更新完成", "INFO");
    }
    log("更新分数和排名完成", "INFO");
}

void auto_renew_score_and_rank() {
    log("开始检测分数和排名更新", "INFO");
    int renewtime = 604800; // 默认7天
    map<string, string> config = loadConfig("config.ini");
    if (config.count("renewtime")) {
        try {
            renewtime = stoi(config["renewtime"]);
        } catch (...) {}
    }
    log("加载自动更新时间配置完成", "INFO");

    log("开始获取当前时间戳", "INFO");
    time_t nowtime;
    time(&nowtime);
    log("当前时间戳：" + to_string(nowtime), "INFO");

    log("开始读取上次更新时间戳", "INFO");
    time_t lasttime = 0;
    ifstream inFile("lastrenew.dat");
    if (inFile) inFile >> lasttime;
    inFile.close();
    log("上次更新时间戳：" + to_string(lasttime), "INFO");
    
    if (nowtime - lasttime >= renewtime) {
        log("检测到需要更新分数和排名", "INFO");
        renew_score_and_rank();

        log("开始保存当前时间戳", "INFO");
        ofstream outFile("lastrenew.dat");
        outFile << nowtime;
        outFile.close();
        log("当前时间戳保存完成", "INFO");
    }


    log("检测分数和排名更新完成", "INFO");
}

// 主程序
int main() {
    setConsoleEncoding(); // 设置控制台编码
    log("程序启动", "INFO");
    log("程序启动", "DEBUG");

    log("开始自动更新流程", "INFO");
    string json_data = fetch_latest_release_json();
    if (check_for_updates(json_data) == true) {
        cout << "发现新版本，是否下载？(y/n): ";
        log("询问用户是否下载新版本", "INFO");
        char choice;
        cin >> choice;
        cin.ignore();
        if (choice == 'y' || choice == 'Y') {
            log("用户选择下载新版本", "INFO");
            update(json_data);
        }
        else {
            log("用户选择不下载新版本", "INFO");
        }
    }
    log("自动更新流程完成", "INFO");

    load_students();
    load_rules();
    
    
    auto_renew_score_and_rank();
    
    map<string, string> config = loadConfig("config.ini");

    log("计算正确密码哈希值", "INFO");
    uint64_t password_hash = 0;
    if (config.count("password")) {
        try {
            password_hash = stoull(config["password"]);
        } catch (...) {
            password_hash = hash_password("12345"); // 默认密码
        }
    } else {
        password_hash = hash_password("12345"); // 默认密码
    }
    log("正确密码哈希值计算完成", "INFO");
    
    cout << "\n====== 班级积分管理系统 ======" << endl;

    while (true) {
        show_ranking();
        show_rules();
        cout << "\n1. 管理规则\n2. 修改积分\n3. 修改学生信息\n4. 手动归零变化值\n5. 小组加分\n6. 设置\n7. 保存退出\n请通过键盘选择操作" << endl;

        int choice = 0;
        string line;
        getline(cin, line);   // 直接读取整行
        if (line.empty()) continue;
        try {
            choice = stoi(line);  // 把字符串转成整数（如果输入非数字会抛异常，但可以捕获）
        } catch (...) {
            cout << "无效输入！" << endl;
            continue;
        }

        switch (choice) {
            case 1: {
                log("进入管理规则分支", "INFO");
                if (!verify_password(password_hash)) {
                    cout << "密码错误！" << endl;
                    break;
                }
                while (true) {
                    cout << "\n1. 添加规则\n2. 删除规则\n3. 返回\n请选择：";
                    int sub_choice;
                    cin >> sub_choice;
                    cin.ignore();
                    if (sub_choice == 1) {
                        log("用户选择添加规则", "INFO");
                        add_rule();
                    } 
                    else if (sub_choice == 2) {
                        log("用户选择删除规则", "INFO");
                        show_rules();
                        cout << "请输入要删除的规则号：";
                        int num;
                        cin >> num;
                        cin.ignore();
                        log("获取到要删除的规则号", "INFO");
                        if (num >= 1 && num <= static_cast<int>(rules.size())) {
                            rules.erase(rules.begin() + num - 1);
                            cout << "删除成功！" << endl;
                            log("规则删除成功", "INFO");
                        } else {
                            cout << "无效的编号！" << endl;
                            log("获取到无效的规则号", "WARNING");
                        }
                    } else break;
                }
                save_rules();
                break;
            }

            case 2: {
                log("进入修改积分分支", "INFO");
                if (!verify_password(password_hash)) {
                    cout << "密码错误！" << endl;
                    break;
                }
                show_students();
                show_rules();
                while (true) {
                    cout << "\n1. 单个修改\n2. 批量修改\n3. 返回\n请选择：";
                    int sub_choice;
                    cin >> sub_choice;
                    cin.ignore();
                    if (sub_choice == 1) {
                        log("用户选择单个修改", "INFO");
                        apply_rule();
                    } else {
                        if (sub_choice == 2) {
                            log("用户选择批量修改", "INFO");
                            togetherapply_rule();
                        }
                        else {
                            log("用户选择返回", "INFO");
                            break;
                        }
                    }
                }
                break;
            }

            case 3: {
                log("进入修改学生信息分支", "INFO");
                if (!verify_password(password_hash)) {
                    cout << "密码错误！" << endl;
                    break;
                }
                modify_student();
                while (true) {
                    cout << "\n1. 继续修改\n2. 返回\n请选择：";
                    int sub_choice;
                    cin >> sub_choice;
                    cin.ignore();
                    if (sub_choice == 1) {
                        log("用户选择继续修改", "INFO");
                        modify_student();
                    }
                    else {
                        log("用户选择返回", "INFO");
                        break;
                    }
                }
                break;
            }

            case 4: {
                log("进入归零变化值分支", "INFO");
                renew_score_and_rank();
                log("返回主菜单", "INFO");
                break;
            }

            case 5: {
                log("进入尿袋分支", "INFO");
                if (!verify_password(password_hash)) {
                    cout << "密码错误！" << endl;
                    break;
                }

                system("calculate_group_score.exe");

                ifstream testfile("group_scores.txt");
                if (testfile) {
                    testfile.close();
                    apply_group_scores();
                    }
                else cout << "未找到计算小组分数后生成的数据文件文件，无法加分" << endl;
                break;
            }
            
            case 6: {
                log("进入设置分支", "INFO");
                if (!verify_password(password_hash)) {
                    cout << "密码错误！" << endl;
                    break;
                }

                while (true) {
                    cout << "\n1. 设置密码\n2. 设置变化值归零频率\n3. 返回\n请选择：";
                    int sub_choice;
                    cin >> sub_choice;
                    cin.ignore();
                    log("获取到设置选项", "INFO");
                    if (sub_choice == 1) {
                        log("用户选择设置密码", "INFO");
                        log("开始获取新密码输入", "INFO");
                        cout << "输入新密码: ";
                        string new_pass;
                        getline(cin, new_pass);
                        log("获取到新密码输入", "INFO");

                        log("计算新密码哈希值", "INFO");
                        password_hash = hash_password(new_pass);
                        log("新密码哈希值计算完成", "INFO");
                        
                        log("开始保存新密码哈希值到配置文件", "INFO");
                        ofstream file("config.ini", ios::app);
                        file << "password=" << password_hash << "\n";
                        file.close();
                        log("新密码哈希值保存完成", "INFO");

                        cout << "密码已更新！" << endl;
                        log("密码设置完成", "INFO");
                    }
                    else if (sub_choice == 2) {
                        log("用户选择设置归零频率", "INFO");

                        log("开始获取新归零频率输入", "INFO");
                        cout << "输入新归零频率（单位：天。请输入纯数字）: ";
                        string new_time_day_str;
                        getline(cin, new_time_day_str);
                        log("获取到新归零频率字符串输入", "INFO");

                        log("开始计算归零频率秒数", "INFO");
                        int new_time_num = stoi(new_time_day_str) * 86400;
                        log("归零频率秒数计算完成", "INFO");

                        log("转换为字符串", "INFO");
                        string new_time = to_string(new_time_num);
                        log("转换完成", "INFO");

                        log("开始保存新归零频率到配置文件", "INFO");
                        ofstream file("config.ini", ios::app);
                        file << "renewtime=" << new_time << "\n"; 
                        file.close();
                        log("新归零频率保存完成", "INFO");

                        cout << "归零频率已更新！" << endl;						 
                        log("归零频率已更新", "INFO");
                    }
                    else {
                        log("用户选择返回", "INFO");
                        break;
                    }
                }
                break;
            }
            
            case 7: {
                log("进入保存数据分支", "INFO");
                save_students();
                save_rules();
                cout << "数据已保存，感谢使用！" << endl;
                log("程序退出", "INFO");
                return 0;
            }

            default: {
                log("用户输入了无效选择", "WARNING");
                cout << "无效的选择！" << endl;
                break;
            }
        }
    }
}