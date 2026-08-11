// 业务存储层：把 FileStore 引擎接到 data_storage.h 的字段级 API
// 班级语义：read::xxx(class) 激活班级（学生/规则/礼物各走独立表单例）；
//           write::xxx 作用于当前激活班级（无班级参数的签名按当前班级处理）
// 持久化语义：write 只改内存 + 置脏标记，由调用方在动作边界显式 save()。
// 注意：include 必须放在 import 之前——MSVC 下消费 TU 在 import 后再 include 模块全局片段已含
// 的 STL 头（<string>/<fstream> 等）会触发 C2572 默认模板参数重复定义
#include "data_storage.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

import storage;

namespace
{
	std::string path_for(const std::string &class_name, const char *stem)
	{
		if (class_name.empty() || class_name == "default")
			return std::string(stem) + ".dat";
		return std::string(stem) + "_" + class_name + ".dat";
	}

	// 三张表各自的单例门面：学生 = FileInstance，规则/礼物 = 同构 Record 的不同类型
	using RuleStore = points::FileStore<points::RuleRecord>;
	using GiftStore = points::FileStore<points::GiftRecord>;

	template<typename Store>
	Store &ensure_store(Store * /*tag*/, const std::string &class_name, const char *stem)
	{
		auto path = path_for(class_name, stem);
		try
		{
			auto &s = Store::get();
			if (s.path() != path)
				s.switch_to(path); // 切班级：脏则自动落盘
			return s;
		} catch (const std::logic_error &)
		{ // 未初始化
			Store::init(path);
			return Store::get();
		}
	}

	points::FileInstance &ensure_students(const std::string &class_name)
	{
		return ensure_store(
			static_cast<points::FileInstance *>(nullptr),
			class_name,
			"students");
	}

	RuleStore &ensure_rules(const std::string &class_name)
	{
		return ensure_store(static_cast<RuleStore *>(nullptr), class_name, "rules");
	}

	GiftStore &ensure_gifts(const std::string &class_name)
	{
		return ensure_store(static_cast<GiftStore *>(nullptr), class_name, "gifts");
	}

	// UTF-8 安全截断：不超过 max_bytes，且不在多字节序列中间切断
	void copy_utf8_truncated(std::byte *dest, size_t max_bytes, const std::string &src)
	{
		size_t n = std::min(src.size(), max_bytes);
		if (n < src.size())
		{ // 需要截断：回退到完整字符边界
			while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0u) == 0x80u)
				--n;
		}
		std::memcpy(dest, src.data(), n);
		std::memset(dest + n, 0, max_bytes - n);
	}

	std::string current_class = "default"; // read::xxx 激活，write::xxx 消费
}                                          // namespace

namespace read
{
	void students(const std::string &class_name)
	{
		current_class = class_name;
		ensure_students(class_name); // 加载即激活
	}

	void rules(const std::string &class_name)
	{
		current_class = class_name;
		ensure_rules(class_name).load(); // 强制从盘重读
	}

	void gifts(const std::string &class_name)
	{
		current_class = class_name;
		ensure_gifts(class_name).load();
	}

	std::map<std::string, std::string> config(const std::string & /*class_name*/)
	{
		std::map<std::string, std::string> kv;
		std::ifstream                      fin("config.ini");
		std::string                        line;
		while (std::getline(fin, line))
		{
			auto pos = line.find('=');
			if (pos == std::string::npos)
				continue;
			kv[line.substr(0, pos)] = line.substr(pos + 1);
		}
		return kv;
	}
} // namespace read

namespace write
{
	namespace student
	{
		// 签名歧义：header 只声明了 bool id()，按"新建一个空学生并返回是否成功"实现
		bool id()
		{
			ensure_students(current_class).add();
			return true;
		}

		bool name(int id, const std::string &new_name)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			copy_utf8_truncated(r->name, sizeof(r->name), new_name);
			fi.mark_dirty();
			return true;
		}

		bool gender(int id, const std::string &new_gender)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			copy_utf8_truncated(r->gender, sizeof(r->gender), new_gender);
			fi.mark_dirty();
			return true;
		}

		bool old_score(int id, const long long &new_old_score)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			r->old_score = new_old_score;
			fi.mark_dirty();
			return true;
		}

		bool score(int id, const long long &new_score)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			r->score = new_score;
			fi.mark_dirty();
			return true;
		}

		bool old_rank(int id, const int &new_old_rank)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			r->old_rank = new_old_rank;
			fi.mark_dirty();
			return true;
		}

		bool rank(int id, const int &new_rank)
		{
			auto &fi = ensure_students(current_class);
			auto *r  = fi.find(id);
			if (!r) return false;
			r->rank = new_rank;
			fi.mark_dirty();
			return true;
		}
	} // namespace student

	namespace rule
	{
		// rule_num 1..N 映射到槽位 rule_num-1（引擎 add() 分配，槽位即编号）
		bool desc(int rule_num, const std::string &new_desc)
		{
			auto &s = ensure_rules(current_class);
			auto *r = s.find(static_cast<uint32_t>(rule_num - 1));
			if (!r) return false;
			copy_utf8_truncated(r->desc, sizeof(r->desc), new_desc);
			s.mark_dirty();
			return true;
		}

		bool delta(int rule_num, const int &new_delta)
		{
			auto &s = ensure_rules(current_class);
			auto *r = s.find(static_cast<uint32_t>(rule_num - 1));
			if (!r) return false;
			r->delta = new_delta;
			s.mark_dirty();
			return true;
		}
	} // namespace rule

	namespace gift
	{
		bool desc(int gift_num, const std::string &new_desc)
		{
			auto &s = ensure_gifts(current_class);
			auto *r = s.find(static_cast<uint32_t>(gift_num - 1));
			if (!r) return false;
			copy_utf8_truncated(r->desc, sizeof(r->desc), new_desc);
			s.mark_dirty();
			return true;
		}

		bool delta(int gift_num, const int &new_delta)
		{
			auto &s = ensure_gifts(current_class);
			auto *r = s.find(static_cast<uint32_t>(gift_num - 1));
			if (!r) return false;
			r->delta = new_delta;
			s.mark_dirty();
			return true;
		}
	} // namespace gift

	void save_all() {
    	// 把当前班级的三张表内存数据全部原子写入磁盘
    	ensure_students(current_class).save();
    	ensure_rules(current_class).save();
    	ensure_gifts(current_class).save();
	}
}     // namespace write
