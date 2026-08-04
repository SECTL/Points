module;
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cstring>
#include <string_view>

module storage;
import exceptions;

namespace points
{
	// ---- Record（学生，161B）----
	void Record::put(std::string& buf, const Record& r) {
		buf.push_back(static_cast<char>(r.flags));
		auto put = [&buf](auto v) {
			buf.append(reinterpret_cast<const char*>(&v), sizeof(v));
		};
		put(r.id);
		put(r.next);
		buf.append(reinterpret_cast<const char*>(r.name), 64);
		buf.append(reinterpret_cast<const char*>(r.gender), 64);
		put(r.old_score);
		put(r.score);
		put(r.old_rank);
		put(r.rank);
	}

	void Record::get(std::string_view s, size_t slot, Record& r) {
		const size_t off = slot * get_record_field_size();
		if (s.size() < off + get_record_field_size())
			throw file_format_error("Record slot out of range");
		// 与 put 严格镜像：同序、同长度，跑指针逐个字段取
		const char* p = s.data() + off;
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

	void Record::clear(Record& r) {
		std::memset(r.name, 0, sizeof(r.name));
		std::memset(r.gender, 0, sizeof(r.gender));
		r.old_score = 0;
		r.score = 0;
		r.old_rank = 0;
		r.rank = 0;
	}

	// ---- RuleRecord（规则/礼物，77B；GiftRecord 继承）----
	void RuleRecord::put(std::string& buf, const RuleRecord& r) {
		buf.push_back(static_cast<char>(r.flags));
		auto put = [&buf](auto v) {
			buf.append(reinterpret_cast<const char*>(&v), sizeof(v));
		};
		put(r.id);
		put(r.next);
		buf.append(reinterpret_cast<const char*>(r.desc), 64);
		put(r.delta);
	}

	void RuleRecord::get(std::string_view s, size_t slot, RuleRecord& r) {
		const size_t off = slot * rule_disk_size();
		if (s.size() < off + rule_disk_size())
			throw file_format_error("Rule slot out of range");
		const char* p = s.data() + off;
		r.flags = static_cast<uint8_t>(*p);
		++p;
		memcpy(&r.id, p, sizeof(r.id));
		p += sizeof(r.id);
		memcpy(&r.next, p, sizeof(r.next));
		p += sizeof(r.next);
		memcpy(r.desc, p, 64);
		p += 64;
		memcpy(&r.delta, p, sizeof(r.delta));
	}

	void RuleRecord::clear(RuleRecord& r) {
		std::memset(r.desc, 0, sizeof(r.desc));
		r.delta = 0;
	}
}
