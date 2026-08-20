module;
#include <vector>

export module business.logic;               // 模块声明

import storage.data;                        // 导入存储模块

export struct Result {
    bool is_success;
    std::string message;
};

//----------------------
export namespace core
{
    Result init();
    void write_to_disk();

    namespace students {
        std::vector<points::StudentData> get_all();
        std::vector<points::StudentData> get_ranking();
        Result modify(const points::StudentData &new_student);
    }

    namespace auth {
        uint64_t hash_password(const std::string& str);
        bool is_pw_correct(const std::string& input);
    }
}