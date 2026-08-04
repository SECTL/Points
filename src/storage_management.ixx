// Storage Management Module for Points
// copyright (c) 2026 SECTL, Licensed under GPL-3.0
module;
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <string>
#include <fstream>
#include <memory>
#include <numeric>
#if defined(__cpp_impl_reflection)
#include <meta>
#endif

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
#if defined(__cpp_impl_reflection)
	// P2996 反射（GCC 16.1+ -freflection）：字段尺寸由 Record 直接推导，数组/断言整体退役
	consteval int get_record_field_size()
	{
		std::size_t s = 0;
		constexpr auto ctx = std::meta::access_context::unchecked();
		for (auto m : std::meta::nonstatic_data_members_of(^^Record, ctx))
			s += std::meta::size_of(m);
		return static_cast<int>(s);
	}
#else
	// 反射不可用（clang-cl/MSVC）时的替身：数组 + 逐字段断言
	constexpr int record_field_sizes[9] = {1, 4, 4, 64, 64, 8, 8, 4, 4};
	// 数组 ↔ 结构体逐字段钉死：改 Record 忘了数组，编译报错而不是运行时错位
	static_assert(sizeof(Record::flags)     == record_field_sizes[0]);
	static_assert(sizeof(Record::id)        == record_field_sizes[1]);
	static_assert(sizeof(Record::next)      == record_field_sizes[2]);
	static_assert(sizeof(Record::name)      == record_field_sizes[3]);
	static_assert(sizeof(Record::gender)    == record_field_sizes[4]);
	static_assert(sizeof(Record::old_score) == record_field_sizes[5]);
	static_assert(sizeof(Record::score)     == record_field_sizes[6]);
	static_assert(sizeof(Record::old_rank)  == record_field_sizes[7]);
	static_assert(sizeof(Record::rank)      == record_field_sizes[8]);
	consteval int get_record_field_size()
	{
		return std::accumulate(std::begin(record_field_sizes), std::end(record_field_sizes), 0);
	}
#endif
	// 两条路径都必须得出同一个磁盘长度，编译期钉死
	static_assert(get_record_field_size() == 161, "record disk size");

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