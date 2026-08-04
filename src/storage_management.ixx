// Storage Management Module for Points
// copyright (c) 2026 SECTL, Licensed under GPL-3.0
module;
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <fstream>
#include <memory>

export module storage;
export namespace points
{
    constexpr char FILE_HEADER_MAGIC[4] = {'P','N','T','S'};
    constexpr uint32_t FILE_VERSION = 1;

    struct FileHeader
    {
        char magic[4];
        uint32_t version;
		uint32_t header_size;
    	uint32_t record_size;
    	uint32_t capacity;
    	uint32_t alive_count;
    	uint32_t free_head;
    	uint32_t crc32;
    };

	struct Record
	{
		uint8_t flags;
		uint32_t id;
		uint32_t next;
		std::byte name[64];
		std::byte gender[64];
		int64_t old_score;
		int64_t score;
		uint32_t old_rank;
		uint32_t rank;
	};

	constexpr uint32_t file_header_size = sizeof(FileHeader);

	// 磁盘记录长度 = 字段连续和（≠ sizeof(Record)，padding 不入文件）
	// flags(1) id(4) next(4) name(64) gender(64) old_score(8) score(8) old_rank(4) rank(4) = 161
	constexpr uint32_t record_size = 161;
	static_assert(1 + 4 + 4 + 64 + 64 + 8 + 8 + 4 + 4 == record_size);

	class FileInstance {
	public:
		static void init(const std::filesystem::path& path);  // main 开头调用一次
		static FileInstance& get();                           // 之后无参访问

		FileInstance(const FileInstance&) = delete;           // 拷贝构造：封死
		FileInstance& operator=(const FileInstance&) = delete; // 拷贝赋值：封死

		void load();                                          // 无参——路径在成员里
		void save();

	private:
		explicit FileInstance(std::filesystem::path path);    // 唯一构造，私有
		std::filesystem::path file_path_;
		static std::unique_ptr<FileInstance> inst_;
	};
}