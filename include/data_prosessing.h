# ifndef DATA_PROCESSING_H
# define DATA_PROCESSING_H

# include <string>

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

namespace modify{
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

void log(const std::string& message, const std::string& level = "INFO");

#endif // DATA_PROCESSING_H