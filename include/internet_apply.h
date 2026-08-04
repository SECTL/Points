# ifndef INTERNET_APPLY_H
# define INTERNET_APPLY_H

# include <string>

namespace update{
    std::string fetch_git_release_json();
    std::string fetch_sectl_release_json();
    bool fetch_git_release_asset(const std::string& download_url, const std::string& save_path);
    bool fetch_sectl_release_asset(const std::string& download_url, const std::string& save_path);
}

# endif // INTERNET_APPLY_H