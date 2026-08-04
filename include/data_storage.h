# ifndef DATA_STORAGE_H
# define DATA_STORAGE_H

# include <string>
# include <map>

namespace read{
    void students(const std::string& class_name = "default");
    void rules(const std::string& class_name = "default");
    void gifts(const std::string& class_name = "default");
    std::map<std::string, std::string> config(const std::string& class_name = "default");
}

namespace write{
    namespace student{
        bool id();
        bool name(int id, const std::string& new_name);
        bool gender(int id, const std::string& new_gender);
        bool old_point(int id, const long long& new_old_point);
        bool point(int id, const long long& new_point);
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
}

# endif // DATA_STORAGE_H