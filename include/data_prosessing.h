# ifndef DATA_PROCESSING_H
# define DATA_PROCESSING_H

# include <optional>
# include <string>
# include <vector>
# include <nlohmann/json.hpp>
# include "entity.h"

namespace points
{
	enum class UpdateChannel { SECTL, GITHUB };

	class Updater
	{
	public:
		Updater(
			const std::string &domain_name,
			const std::string &save_path);

		nlohmann::json fetch_updates();

		void switch_channel(UpdateChannel channel);

		void download_updates(std::filesystem::path download_path);

		void install_update(); //TODO: 实现
	};

	void log(const std::string &message, const std::string &level = "INFO");
}



void log(const std::string &message, const std::string &level = "INFO");

#endif // DATA_PROCESSING_H
