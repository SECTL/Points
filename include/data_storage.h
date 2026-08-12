# ifndef DATA_STORAGE_H
# define DATA_STORAGE_H

# include <string>
# include <map>

namespace read{
    void students(const std::string& class_name = "default");
    void rules(const std::string& class_name = "default");    // 这三个函数会根据 class_name 切换班级并读取，即修改 data_storage.cpp的 current_class ，危险。是给 data_processing.h 里的 use_class() 调用的
    void gifts(const std::string& class_name = "default");
    std::map<std::string, std::string> config(const std::string& class_name = "default");

    const std::vector<Record>& return_all_records(const std::string& class_name); // 这个函数不改 data_storage.cpp的 current_class ，供 data_processing.h 里的 repository::student::get_all() 之类的 调用
}

namespace write{
    namespace student{
        bool id();
        bool name(int id, const std::string& new_name);
        bool gender(int id, const std::string& new_gender);
        bool old_score(int id, const long long& new_old_score);
        bool score(int id, const long long& new_score);
        bool old_rank(int id, const int& new_old_rank);
        bool rank(int id, const int& new_rank);
    }

    namespace rule{
        bool desc(int rule_num, const std::string& new_desc);
        bool delta(int rule_num, const int& new_delta);
    }

    namespace gift{
        bool desc(int gift_num, const std::string& new_desc);
        bool delta(int gift_num, const int& new_delta);
    }

    void save_all();

    void log(const std::string& message, const std::string& level = "INFO");
}

# endif // DATA_STORAGE_H