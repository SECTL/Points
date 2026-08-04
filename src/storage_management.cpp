module;
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cstring>
#include <string_view>
#include <utility>

module storage;
import exceptions;

namespace points
{
	std::unique_ptr<FileInstance> FileInstance::inst_;

	FileInstance::FileInstance(std::filesystem::path path)
		: file_path_(std::move(path))
	{
	}

	void FileInstance::init(const std::filesystem::path &path)
	{
		if (inst_)
			throw std::logic_error("FileInstance already initialized");
		inst_ = std::unique_ptr<FileInstance>(new FileInstance(path));
	}

	FileInstance &FileInstance::get()
	{
		if (!inst_)
			throw std::logic_error("FileInstance not initialized");
		return *inst_;
	}

	void put_record(std::string &buf, const Record &record)
	{
		buf.push_back(record.flags);
		auto put = [&buf](auto v)
		{
			buf.append(reinterpret_cast<const char *>(&v), sizeof(v));
		};
		put(record.id);
		put(record.next);
		buf.append(reinterpret_cast<const char *>(record.name), 64);
		buf.append(reinterpret_cast<const char *>(record.gender), 64);
		put(record.old_score);
		put(record.score);
		put(record.old_rank);
		put(record.rank);
	}

	void get_record(std::string_view s, size_t slot, Record &r)
	{
		const size_t off = slot * get_record_field_size();
		if (s.size() < off + get_record_field_size())
			throw file_format_error("Record slot out of range");
		// 与 put_record 严格镜像：同序、同长度，跑指针逐个字段取
		const char *p = s.data() + off;
		r.flags = static_cast<uint8_t>(*p);
		++p;
		memcpy(&r.id, p, sizeof(r.id));
		p += sizeof(r.id);
		memcpy(&r.next, p, sizeof(r.next));
		p += sizeof(r.next);
		memcpy(r.name, p, 64);
		p += 64;
		memcpy(r.gender, p, 64);
		p += 64;
		memcpy(&r.old_score, p, sizeof(r.old_score));
		p += sizeof(r.old_score);
		memcpy(&r.score, p, sizeof(r.score));
		p += sizeof(r.score);
		memcpy(&r.old_rank, p, sizeof(r.old_rank));
		p += sizeof(r.old_rank);
		memcpy(&r.rank, p, sizeof(r.rank));
		// 等到C++26 反射来了之后这段代码就可以变成一个循环，这就是前面 record_field_sizes 数组的作用
	}

	// ---- 文件层：CRC-32（IEEE 802.3，poly 0xEDB88320），覆盖整个记录区 ----
	namespace {
		constexpr std::array<uint32_t, 256> make_crc32_table() {
			std::array<uint32_t, 256> t{};
			for (uint32_t i = 0; i < 256; ++i) {
				uint32_t c = i;
				for (int k = 0; k < 8; ++k)
					c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
				t[i] = c;
			}
			return t;
		}
		constexpr auto crc32_table = make_crc32_table();
	}
	uint32_t crc32(const void* data, size_t len) {
		const auto* p = static_cast<const uint8_t*>(data);
		uint32_t c = 0xFFFFFFFFu;
		for (size_t i = 0; i < len; ++i)
			c = crc32_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
		return c ^ 0xFFFFFFFFu;
	}

	const std::filesystem::path& FileInstance::path() const noexcept { return file_path_; }

	void FileInstance::load() {
		if (!std::filesystem::exists(file_path_)) {   // 全新班级：空库
			pool_.clear();
			capacity_ = 0;
			alive_ = 0;
			free_head_ = FREE_NIL;
			loaded_ = true;
			dirty_ = false;
			return;
		}
		std::ifstream fin(file_path_, std::ios::binary);
		fin.exceptions(std::ios::failbit | std::ios::badbit);
		FileHeader h;
		fin.read(reinterpret_cast<char*>(&h), sizeof(h));
		if (std::memcmp(h.magic, FILE_HEADER_MAGIC, 4) != 0)
			throw file_format_error("文件头魔数不符");
		if (h.version != FILE_VERSION)
			throw file_format_error("文件版本不符");
		if (h.header_size != file_header_size || h.record_size != static_cast<uint32_t>(get_record_field_size()))
			throw file_format_error("文件头尺寸字段不符");

		std::string buf(static_cast<size_t>(h.record_size) * h.capacity, '\0');
		fin.read(buf.data(), static_cast<std::streamsize>(buf.size()));
		if (crc32(buf.data(), buf.size()) != h.crc32)
			throw file_format_error("记录区 CRC 校验失败");

		pool_.resize(h.capacity);
		uint32_t chain = FREE_NIL;
		uint32_t tail = FREE_NIL;
		uint32_t alive = 0;
		for (uint32_t i = 0; i < h.capacity; ++i) {
			get_record(buf, i, pool_[i]);
			if (pool_[i].flags & 0x01u) {
				++alive;
				pool_[i].next = 0;
			} else {
				pool_[i].next = FREE_NIL;
				if (tail == FREE_NIL) chain = i;
				else pool_[tail].next = i;
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

		capacity_ = h.capacity;
		alive_ = alive;
		free_head_ = chain;
		loaded_ = true;
		dirty_ = false;
	}

	void FileInstance::save() {
		if (!dirty_) return;   // 懒写：无变更不落盘
		FileHeader h;
		std::memcpy(h.magic, FILE_HEADER_MAGIC, 4);
		h.version = FILE_VERSION;
		h.header_size = file_header_size;
		h.record_size = static_cast<uint32_t>(get_record_field_size());
		h.capacity = capacity_;
		h.alive_count = alive_;
		h.free_head = free_head_;
		h.crc32 = 0;

		std::string buf;
		buf.reserve(file_header_size + static_cast<size_t>(capacity_) * get_record_field_size());
		buf.append(reinterpret_cast<const char*>(&h), sizeof(h));
		for (uint32_t i = 0; i < capacity_; ++i)
			put_record(buf, pool_[i]);
		h.crc32 = crc32(buf.data() + sizeof(h), buf.size() - sizeof(h));
		std::memcpy(buf.data(), &h, sizeof(h));

		auto tmp = file_path_;
		tmp += ".tmp";
		{
			std::ofstream fout(tmp, std::ios::binary);
			fout.exceptions(std::ios::failbit | std::ios::badbit);
			fout.write(buf.data(), static_cast<std::streamsize>(buf.size()));
		}
		std::filesystem::rename(tmp, file_path_);   // 原子替换
		dirty_ = false;
		loaded_ = true;
	}

	void FileInstance::switch_to(const std::filesystem::path& path) {
		if (path == file_path_) return;
		save();                        // 脏则先落盘当前班级
		file_path_ = path;
		loaded_ = false;
		load();
	}

	Record* FileInstance::find(uint32_t id) noexcept {
		if (id >= capacity_) return nullptr;
		auto& r = pool_[id];
		return (r.flags & 0x01u) ? &r : nullptr;
	}
	const Record* FileInstance::find(uint32_t id) const noexcept {
		if (id >= capacity_) return nullptr;
		const auto& r = pool_[id];
		return (r.flags & 0x01u) ? &r : nullptr;
	}

	uint32_t FileInstance::add() {
		uint32_t slot;
		if (free_head_ != FREE_NIL) {          // 复用空洞（LIFO）
			slot = free_head_;
			free_head_ = pool_[slot].next;
		} else {                               // 追加新槽
			slot = capacity_++;
			pool_.emplace_back();
		}
		auto& r = pool_[slot];
		r.flags = 0x01u;
		r.id = slot;
		r.next = 0;
		std::memset(r.name, 0, sizeof(r.name));
		std::memset(r.gender, 0, sizeof(r.gender));
		r.old_score = 0;
		r.score = 0;
		r.old_rank = 0;
		r.rank = 0;
		++alive_;
		dirty_ = true;
		return slot;
	}

	bool FileInstance::remove(uint32_t id) {
		auto* r = find(id);
		if (!r) return false;
		r->flags = 0;                  // 借 next 挂空闲链
		r->next = free_head_;
		free_head_ = id;
		--alive_;
		dirty_ = true;
		return true;
	}

	void FileInstance::mark_dirty() noexcept { dirty_ = true; }
}
