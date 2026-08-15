// Storage Management Module for Points
// copyright (c) 2026 SECTL, Licensed under GPL-3.0
module;
#include <array>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#if defined(__cpp_impl_reflection)
#include <meta>
#endif

export module storage;
import exceptions;

// 模块私有：CRC-32（IEEE 802.3，poly 0xEDB88320），覆盖整个记录区。
// 定义在接口里（inline）：FileStore 模板在消费 TU 实例化时也要调用它
namespace
{
	constexpr std::array<uint32_t, 256> make_crc32_table()
	{
		std::array<uint32_t, 256> t{};
		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t c = i;
			for (int k = 0; k < 8; ++k)
				c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
			t[i] = c;
		}
		return t;
	}
}

inline uint32_t crc32(const void *data, size_t len)
{
	static constexpr auto crc32_table = make_crc32_table();
	const auto *p = static_cast<const uint8_t *>(data);
	uint32_t    c = 0xFFFFFFFFu;
	for (size_t i = 0; i < len; ++i)
		c = crc32_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return c ^ 0xFFFFFFFFu;
}

export namespace points
{
	constexpr char     FILE_HEADER_MAGIC[4] = {'P', 'N', 'T', 'S'};
	constexpr uint32_t FILE_VERSION         = 1;

	struct FileHeader
	{
		char     magic[4];
		uint32_t version;
		uint32_t header_size;
		uint32_t record_size;
		uint32_t capacity;
		uint32_t alive_count;
		uint32_t free_head;
		uint32_t crc32;
	};

	// 学生记录。磁盘布局见 get_record_field_size；flags bit0 = alive，
	// 空闲槽借 next 挂空闲链表。序列化/清零由各记录类型自行提供。
	struct Record
	{
		uint8_t   flags;
		uint32_t  id;
		uint32_t  next;
		std::byte name[64];
		std::byte gender[64];
		int64_t   old_score;
		int64_t   score;
		uint32_t  old_rank;
		uint32_t  rank;

		static void put(std::string &buf, const Record &r); // 字段连续贴（161B，无 padding）
		static void get(std::string_view s, size_t slot, Record &r);

		static void          clear(Record &r); // 新增时字段清零
		static consteval int disk_size();      // 磁盘记录长度
	};

	constexpr uint32_t file_header_size = sizeof(FileHeader);

	// 磁盘记录长度 = 字段连续和（≠ sizeof(Record)，padding 不入文件）
	// flags(1) id(4) next(4) name(64) gender(64) old_score(8) score(8) old_rank(4) rank(4) = 161
	#if defined(__cpp_impl_reflection)
	// P2996 反射（GCC 16.1+ -freflection）：字段尺寸由 Record 直接推导，数组/断言整体退役
	consteval int get_record_field_size()
	{
		std::size_t    s   = 0;
		constexpr auto ctx = std::meta::access_context::unchecked();
		for (auto m: std::meta::nonstatic_data_members_of(^^Record, ctx))
			s += std::meta::size_of(m);
		return static_cast<int>(s);
	}
	#else
	// 反射不可用（clang-cl/MSVC）时的替身：数组 + 逐字段断言
	constexpr int record_field_sizes[9] = {1, 4, 4, 64, 64, 8, 8, 4, 4};
	// 数组 ↔ 结构体逐字段钉死：改 Record 忘了数组，编译报错而不是运行时错位
	static_assert(sizeof(Record::flags) == record_field_sizes[0]);
	static_assert(sizeof(Record::id) == record_field_sizes[1]);
	static_assert(sizeof(Record::next) == record_field_sizes[2]);
	static_assert(sizeof(Record::name) == record_field_sizes[3]);
	static_assert(sizeof(Record::gender) == record_field_sizes[4]);
	static_assert(sizeof(Record::old_score) == record_field_sizes[5]);
	static_assert(sizeof(Record::score) == record_field_sizes[6]);
	static_assert(sizeof(Record::old_rank) == record_field_sizes[7]);
	static_assert(sizeof(Record::rank) == record_field_sizes[8]);

	consteval int get_record_field_size()
	{
		return std::accumulate(
			std::begin(record_field_sizes),
			std::end(record_field_sizes),
			0);
	}
	#endif
	// 两条路径都必须得出同一个磁盘长度，编译期钉死
	static_assert(get_record_field_size() == 161, "record disk size");

	consteval int Record::disk_size()
	{
		return get_record_field_size();
	}

	// 规则/礼物记录：desc 描述 + delta 分值，77B = 1+4+4+64+4
	struct RuleRecord
	{
		uint8_t   flags;
		uint32_t  id;
		uint32_t  next;
		std::byte desc[64];
		int32_t   delta;

		static void put(std::string &buf, const RuleRecord &r);

		static void get(std::string_view s, size_t slot, RuleRecord &r);

		static void clear(RuleRecord &r);

		static consteval int disk_size();
	};

	#if defined(__cpp_impl_reflection)
	consteval int rule_disk_size()
	{
		std::size_t    s   = 0;
		constexpr auto ctx = std::meta::access_context::unchecked();
		for (auto m: std::meta::nonstatic_data_members_of(^^RuleRecord, ctx))
			s += std::meta::size_of(m);
		return static_cast<int>(s);
	}
	#else
	constexpr int rule_field_sizes[5] = {1, 4, 4, 64, 4};
	static_assert(sizeof(RuleRecord::flags) == rule_field_sizes[0]);
	static_assert(sizeof(RuleRecord::id) == rule_field_sizes[1]);
	static_assert(sizeof(RuleRecord::next) == rule_field_sizes[2]);
	static_assert(sizeof(RuleRecord::desc) == rule_field_sizes[3]);
	static_assert(sizeof(RuleRecord::delta) == rule_field_sizes[4]);

	consteval int rule_disk_size()
	{
		return std::accumulate(
			std::begin(rule_field_sizes),
			std::end(rule_field_sizes),
			0);
	}
	#endif
	static_assert(rule_disk_size() == 77, "rule disk size");

	consteval int RuleRecord::disk_size()
	{
		return rule_disk_size();
	}

	// 礼物 = 规则同构（desc + delta），独立类型以让 FileStore 各自持有单例
	struct GiftRecord : RuleRecord
	{
		static consteval int disk_size()
		{
			return rule_disk_size();
		}
	};

	// 空闲链表哨兵：0xFFFFFFFF = 无空闲槽
	inline constexpr uint32_t FREE_NIL = 0xFFFFFFFFu;

	// 通用定长记录文件引擎：header(32B) + 记录数组，空闲链表复用，CRC 校验，
	// tmp+rename 原子写。T 必须提供：put/get/clear/disk_size。
	template<typename T>
	class FileStore
	{
	public:
		explicit FileStore(std::filesystem::path path) : file_path_(std::move(path))
		{
			load();
		}

		FileStore(const FileStore &) = delete;

		FileStore &operator=(const FileStore &) = delete;

		const std::filesystem::path &path() const noexcept
		{
			return file_path_;
		}

		void load()
		{
			if (!std::filesystem::exists(file_path_))
			{ // 全新表：空库
				pool_.clear();
				capacity_  = 0;
				alive_     = 0;
				free_head_ = FREE_NIL;
				loaded_    = true;
				dirty_     = false;
				return;
			}
			std::ifstream fin(file_path_, std::ios::binary);
			fin.exceptions(std::ios::failbit | std::ios::badbit);
			FileHeader h;
			fin.read(reinterpret_cast<char *>(&h), sizeof(h));
			if (std::memcmp(h.magic, FILE_HEADER_MAGIC, 4) != 0)
				throw file_format_error("文件头魔数不符");
			if (h.version != FILE_VERSION)
				throw file_format_error("文件版本不符");
			if (h.header_size != file_header_size || h.record_size != static_cast<
				uint32_t>(T::disk_size()))
				throw file_format_error("文件头尺寸字段不符");

			std::string buf(static_cast<size_t>(h.record_size) * h.capacity, '\0');
			fin.read(buf.data(), static_cast<std::streamsize>(buf.size()));
			if (crc32(buf.data(), buf.size()) != h.crc32)
				throw file_format_error("记录区 CRC 校验失败");

			pool_.resize(h.capacity);
			uint32_t chain = FREE_NIL;
			uint32_t tail  = FREE_NIL;
			uint32_t alive = 0;
			for (uint32_t i = 0; i < h.capacity; ++i)
			{
				T::get(buf, i, pool_[i]);
				if (pool_[i].flags & 0x01u)
				{
					++alive;
					pool_[i].next = 0;
				}
				else
				{
					pool_[i].next = FREE_NIL;
					if (tail == FREE_NIL) chain = i;
					else pool_[tail].next       = i;
					tail = i;
				}
			}
			if (alive != h.alive_count)
				throw file_format_error("存活计数与文件头不符");
			uint32_t n = 0;
			for (uint32_t s = chain; s != FREE_NIL; s = pool_[s].next)
				if (++n > h.capacity)
					throw file_format_error("空闲链表成环或超长");
			if (n != h.capacity - h.alive_count)
				throw file_format_error("空闲链表长度不符");

			capacity_  = h.capacity;
			alive_     = alive;
			free_head_ = chain;
			loaded_    = true;
			dirty_     = false;
		}

		void save()
		{
			if (!dirty_) return; // 懒写：无变更不落盘
			FileHeader h;
			std::memcpy(h.magic, FILE_HEADER_MAGIC, 4);
			h.version     = FILE_VERSION;
			h.header_size = file_header_size;
			h.record_size = static_cast<uint32_t>(T::disk_size());
			h.capacity    = capacity_;
			h.alive_count = alive_;
			h.free_head   = free_head_;
			h.crc32       = 0;

			std::string buf;
			buf.reserve(
				file_header_size + static_cast<size_t>(capacity_) * T::disk_size());
			buf.append(reinterpret_cast<const char *>(&h), sizeof(h));
			for (uint32_t i = 0; i < capacity_; ++i)
				T::put(buf, pool_[i]);
			h.crc32 = crc32(buf.data() + sizeof(h), buf.size() - sizeof(h));
			std::memcpy(buf.data(), &h, sizeof(h));

			auto tmp = file_path_;
			tmp      += ".tmp";
			{
				std::ofstream fout(tmp, std::ios::binary);
				fout.exceptions(std::ios::failbit | std::ios::badbit);
				fout.write(buf.data(), static_cast<std::streamsize>(buf.size()));
			}
			std::filesystem::rename(tmp, file_path_); // 原子替换
			dirty_  = false;
			loaded_ = true;
		}

		void switch_to(const std::filesystem::path &path)
		{
			if (path == file_path_) return;
			save();
			file_path_ = path;
			load();
		}

		const std::vector<T> &all() const noexcept
		{
			return pool_;
		}

		std::vector<T> &all() noexcept
		{
			return pool_;
		}


		T *find(uint32_t id) noexcept
		{
			if (id >= capacity_) return nullptr;
			auto &r = pool_[id];
			return (r.flags & 0x01u) ? &r : nullptr;
		}

		const T *find(uint32_t id) const noexcept
		{
			if (id >= capacity_) return nullptr;
			const auto &r = pool_[id];
			return (r.flags & 0x01u) ? &r : nullptr;
		}

		uint32_t add()
		{
			uint32_t slot;
			if (free_head_ != FREE_NIL)
			{ // 复用空洞（LIFO）
				slot       = free_head_;
				free_head_ = pool_[slot].next;
			}
			else
			{ // 追加新槽
				slot = capacity_++;
				pool_.emplace_back();
			}
			auto &r = pool_[slot];
			r.flags = 0x01u;
			r.id    = slot;
			r.next  = 0;
			T::clear(r);
			++alive_;
			dirty_ = true;
			return slot;
		}

		bool remove(uint32_t id)
		{
			auto *r = find(id);
			if (!r) return false;
			r->flags   = 0; // 借 next 挂空闲链
			r->next    = free_head_;
			free_head_ = id;
			--alive_;
			dirty_ = true;
			return true;
		}

		void mark_dirty() noexcept
		{
			dirty_ = true;
		}

	private:
		std::filesystem::path             file_path_;
		std::vector<T>                    pool_;
		uint32_t                          capacity_  = 0;
		uint32_t                          alive_     = 0;
		uint32_t                          free_head_ = FREE_NIL;
		bool                              loaded_    = false;
		bool                              dirty_     = false;
	};


	// 学生表门面（保持既有 API/单例语义）
	using FileInstance = FileStore<Record>;
}
