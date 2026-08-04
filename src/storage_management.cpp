module;
#include <filesystem>
#include <memory>
#include <cstring>
#include <string_view>

module storage;
import exceptions;

namespace points
{
	std::unique_ptr<FileInstance> FileInstance::inst_;

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
		const size_t off = slot * record_size;
		if (s.size() < off + record_size)
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
	}
}
