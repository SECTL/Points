module business.logic;
#include <algorithm>
import storage.data;


// 注：未完工。下一步计划：使用模板简化代码。
namespace core {
    // points::DataStorage storage{"default"};
    points::DataStorage& get_storage() {
        static points::DataStorage storage{"default"};
        return storage;
    }

    Result init() {
        get_storage();
        // 留着写其他东西，比如检查是否重制积分变化值。
        return {.is_success = true, .message = "初始化成功"};
    }
    
    void write_to_disk() {
        get_storage().save();
    }
    
    namespace student {
        std::vector<points::StudentData> get_all() {
            return get_storage().students().all();
        }
        
        std::vector<points::StudentData> get_ranking() {
            auto all_students = get_storage().students().all();
            std::sort(all_students.begin(), all_students.end(),
                [](const points::StudentData& a, const points::StudentData& b) {
                    if (a.score != b.score) return a.score > b.score;
                    return a.id < b.id;
                });
            return all_students;
        }
        
        Result modify(const points::StudentData &new_student) {
            if (get_storage().students().find(new_student.id) == std::nullopt) {
                return {.is_success = false, .message = "请求修改的成员不存在。"};
            }
            if (get_storage().students().update(new_student.id, new_student)) {
                return {.is_success = true, .message = "修改成员信息成功。"};
            } else {
                return {.is_success = false, .message = "修改成员信息失败。"};
            }
        }
    }
    
    namespace rule{
        std::vector<points::RuleData>& get_all() {
            return get_storage().rules().all();
        }
        
        Result modify(const points::RuleData &new_rule) {
            if (get_storage().rules().find(new_rule.id) == std::nullopt) {
                return {.is_success = false, .message = "请求修改的规则不存在。"};
            }
            if (get_storage().rules().update(new_rule.id, new_rule)) {
                return {.is_success = true, .message = "修改规则信息成功。"};
            } else {
                return {.is_success = false, .message = "修改规则信息失败。"};
            }
        }
    }
    
    namespace gift {
        std::vector<points::GiftData>& get_all() {
            return get_storage().gifts().all();
        }
        
        Result modify(const points::GiftData &new_gift) {
            if (get_storage().gifts().find(new_gift.id) == std::nullopt) {
                return {.is_success = false, .message = "请求修改的礼物不存在。"};
            }
            if (get_storage().gifts().update(new_gift.id, new_gift)) {
                return {.is_success = true, .message = "修改礼物信息成功。"};
            } else {
                return {.is_success = false, .message = "修改礼物信息失败。"};
            }
        }
    }
    
    namespace score{
        Result apply_rule(const std::vector<uint32_t>& student_ids, const uint32_t& rule_id) { 
            auto rule_opt = get_storage().rules().find(rule_id);
            if(!rule_opt) return{false, "请求应用的规则不存在。"};
            const auto& rule_data = *rule_opt;
            const auto& delta = rule_data.delta;
            std::string message_supplement;
            for(const auto& id : student_ids){
                auto student_opt = get_storage().students().find(id);
                if([[unlikely]] !student_opt) {
                    message_supplement += std::format("；成员{}不存在，已跳过", id);
                    continue;
                }
                auto student_data = *student_opt;
                student_data.score += delta;
                get_storage().students().update(id, student_data);
            }
            return {true, std::format("应用规则成功{}。", message_supplement)};
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
            // 请在完成设置功能后取消下面代码的注释
            // auto password_hash = hash_password(input);
            // return (password_hash = config.password_hash)
            return true;
        }
    }// namespace auth
} // namespace core