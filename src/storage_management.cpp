module;
#include <filesystem>
#include <memory>

module storage;

namespace points
{
	std::unique_ptr<FileInstance> FileInstance::inst_;

	void FileInstance::init(const std::filesystem::path &path)
	{
		if (inst_)
			throw std::logic_error("FileInstance already initialized");
		inst_ = std::make_unique<FileInstance>(path);
	}

	FileInstance &FileInstance::get()
	{
		if (!inst_)
			throw std::logic_error("FileInstance not initialized");
		return *inst_;
	}
}
