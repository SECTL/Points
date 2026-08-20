module business.logic;

import storage.data;
#include <algorithm>

namespace core {
    // 全局存储对象（整个模块共享）
    points::DataStorage storage{"default"};

    Result init() {
        // 留着写其他东西，比如检查是否重制积分变化值。
        return {.is_success = true, .message = "初始化成功"};
    }
    
    void write_to_disk() {
        storage.save();
    }
    
    namespace students {
        std::vector<points::StudentData> get_all() {
            return storage.students().all();
        }
        
        std::vector<points::StudentData> get_ranking() {
            auto all_students = storage.students().all();
            std::sort(all_students.begin(), all_students.end(),
                [](const points::StudentData& a, const points::StudentData& b) {
                    if (a.score != b.score) return a.score > b.score;
                    return a.id < b.id;
                });
            return all_students;
        }
        
        Result modify(const points::StudentData &new_student) {
            if (storage.students().find(new_student.id) == std::nullopt) {
                return {.is_success = false, .message = "请求修改的成员不存在。"};
            }
            if (storage.students().update(new_student.id, new_student)) {
                return {.is_success = true, .message = "修改成员信息成功。"};
            } else {
                return {.is_success = false, .message = "修改成员信息失败。"};
            }
        }
    }
    
    namespace auth {
        uint64_t hash_password(const std::string& str) {
            uint64_t hash = 5381;
            for (char c : str) {
                hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
            }
            return hash;
        }
        bool is_pw_correct(const std::string& input) {
            // auto password_hash = hash_password(input);
            // return (password_hash = config.password_hash)
            return true;
        }
    }// namespace auth
} // namespace core