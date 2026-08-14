module;
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

// Storage implementation with C++23 features
module storage.data;
import storage;

namespace points
{
	namespace
	{
		// UTF-8 安全截断：不超过 max_bytes，且不在多字节序列中间切断
		void copy_utf8_truncated(std::byte *dest, size_t max_bytes, std::string_view src)
		{
			size_t n = std::min(src.size(), max_bytes);
			if (n < src.size())
			{
				// 回退到完整字符边界
				while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0u) == 0x80u)
					--n;
			}
			std::memcpy(dest, src.data(), n);
			std::memset(dest + n, 0, max_bytes - n);
		}

		// 从 std::byte[] 提取字符串
		std::string extract_string(const std::byte *data, size_t max_len)
		{
			std::string result;
			for (size_t i = 0; i < max_len; ++i)
			{
				char c = static_cast<char>(data[i]);
				if (c == '\0') break;
				result.push_back(c);
			}
			return result;
		}
	} // namespace

	// ---- StudentRepository ----
	std::uint32_t StudentRepository::create(const StudentData &data)
	{
		auto slot = store_->add();
		auto *r   = store_->find(slot);
		if (!r) [[unlikely]]
			throw std::logic_error("刚新建的槽位找不到");

		copy_utf8_truncated(r->name, sizeof(r->name), data.name);
		copy_utf8_truncated(r->gender, sizeof(r->gender), data.gender);
		r->old_score = data.old_score;
		r->score     = data.score;
		r->old_rank  = data.old_rank;
		r->rank      = data.rank;
		store_->mark_dirty();
		return slot;
	}

	bool StudentRepository::update(std::uint32_t id, const StudentData &data)
	{
		auto *r = store_->find(id);
		if (!r) return false;

		copy_utf8_truncated(r->name, sizeof(r->name), data.name);
		copy_utf8_truncated(r->gender, sizeof(r->gender), data.gender);
		r->old_score = data.old_score;
		r->score     = data.score;
		r->old_rank  = data.old_rank;
		r->rank      = data.rank;
		store_->mark_dirty();
		return true;
	}

	// ---- RuleRepository ----
	std::uint32_t RuleRepository::create(const RuleData &data)
	{
		auto slot = store_->add();
		auto *r   = store_->find(slot);
		if (!r) [[unlikely]]
			throw std::logic_error("刚新建的槽位找不到");

		copy_utf8_truncated(r->desc, sizeof(r->desc), data.description);
		r->delta = data.delta;
		store_->mark_dirty();
		return slot;
	}

	bool RuleRepository::update(std::uint32_t id, const RuleData &data)
	{
		auto *r = store_->find(id);
		if (!r) return false;

		copy_utf8_truncated(r->desc, sizeof(r->desc), data.description);
		r->delta = data.delta;
		store_->mark_dirty();
		return true;
	}

	// ---- GiftRepository ----
	std::uint32_t GiftRepository::create(const GiftData &data)
	{
		auto slot = store_->add();
		auto *r   = store_->find(slot);
		if (!r) [[unlikely]]
			throw std::logic_error("刚新建的槽位找不到");

		copy_utf8_truncated(r->desc, sizeof(r->desc), data.description);
		r->delta = data.delta;
		store_->mark_dirty();
		return slot;
	}

	bool GiftRepository::update(std::uint32_t id, const GiftData &data)
	{
		auto *r = store_->find(id);
		if (!r) return false;

		copy_utf8_truncated(r->desc, sizeof(r->desc), data.description);
		r->delta = data.delta;
		store_->mark_dirty();
		return true;
	}
} // namespace points
