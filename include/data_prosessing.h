# ifndef DATA_PROCESSING_H
# define DATA_PROCESSING_H

# include <string>
# include <vector>
# include <optional>
# include "include/entity.h"

namespace update{
    bool download_git_release_asset(const std::string& domain_name, const std::string& save_path);
    bool download_sectl_release_asset(const std::string& save_path);
}

namespace json_parser{
    std::string extract_tag_name_from_git_json(const std::string& json_str);
    std::string extract_tag_name_from_sectl_json(const std::string& json_str);
    std::string extract_download_url_from_git_json(const std::string& json_str);
    std::string extract_download_url_from_sectl_json(const std::string& json_str);

    std::string extract_config_from_json(const std::string& key, const std::string& json_str);
}

namespace repository{

    void use_class(const std::string& class_name);
    namespace student{
        std::vector<Student> get_all();   // 注意：写实现的时候要调用 data_storage.cpp 的 return_all_records() ！不能调用 read::students() ，否则会静默切换班级！！！
        std::optional<Student> get_by_id(int id);
        void save_one(const Student& s);
        void add_one(const Student& s);
        void remove_one(int id);
    }

    namespace rule{
        std::vector<Rule> get_all();
        std::optional<Rule> get_by_rule_num(int rule_num);
        void save_one(const Rule& r);
        void add_one(const Rule& r);
        void remove_one(int rule_num);
    }

    namespace gift{
        std::vector<Gift> get_all();
        std::optional<Gift> get_by_gift_num(int gift_num);
        void save_one(const Gift& g);
        void add_one(const Gift& g);
        void remove_one(int gift_num);
    }

    // 所有改动落盘（业务逻辑做完一连串操作后，最后存一下）
    void write_to_disk();
}

void log(const std::string& message, const std::string& level = "INFO");

#endif // DATA_PROCESSING_H