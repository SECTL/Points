module;
#include <vector>
#include <string>   // 需要 string，因为 Result 里有 std::string

export module business.logic;               // 模块声明

import storage.data;                        // 导入存储模块

export struct Result {
    bool is_success;
    std::string message;
};

//----------------------
export namespace core
{
    // 初始化
    Result init();

    // 写入磁盘
    void write_to_disk();

    // 学生相关
    namespace students {
        const std::vector<points::StudentData>& get_all();
        std::vector<points::StudentData> get_ranking();
        Result modify(const points::StudentData& new_student);
    }

    // 规则相关
    namespace rule {
        const std::vector<points::RuleData>& get_all();
        Result modify(const points::RuleData& new_rule);
    }

    // 礼物相关
    namespace gift {
        const std::vector<points::GiftData>& get_all();
        Result modify(const points::GiftData& new_gift);
    }

    // 积分操作
    namespace score {
        Result apply_rule(const std::vector<uint32_t>& student_ids, const uint32_t& rule_id);
    }

    // 认证相关
    namespace auth {
        uint64_t hash_password(const std::string& str);
        bool is_pw_correct(const std::string& input);
    }
}