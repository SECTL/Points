module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <format>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module storage.provider;
import storage;

namespace points
{
	namespace
	{
		void copy_utf8_truncated(std::byte *dest, std::size_t max_bytes, std::string_view src)
		{
			std::size_t n = std::min(src.size(), max_bytes);
			if (n < src.size())
				while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0u) == 0x80u)
					--n;
			std::memcpy(dest, src.data(), n);
			std::memset(dest + n, 0, max_bytes - n);
		}

		std::string extract_string(const std::byte *data, std::size_t max_len)
		{
			std::size_t n = 0;
			while (n < max_len && data[n] != std::byte{0}) ++n;
			return {reinterpret_cast<const char *>(data), n};
		}

		std::filesystem::path path_for(std::string_view class_name, std::string_view stem)
		{
			if (class_name.empty() || class_name == "default")
				return std::format("{}.dat", stem);
			return std::format("{}_{}.dat", stem, class_name);
		}
	}

	class FileStorageProvider final : public IStorageProvider
	{
	public:
		explicit FileStorageProvider(std::string class_name)
			: class_name_(std::move(class_name))
			, students_store_(path_for(class_name_, "students"))
			, rules_store_(path_for(class_name_, "rules"))
			, gifts_store_(path_for(class_name_, "gifts"))
		{}

		void switch_class(std::string class_name) override
		{
			class_name_ = std::move(class_name);
			students_store_.switch_to(path_for(class_name_, "students"));
			rules_store_.switch_to(path_for(class_name_, "rules"));
			gifts_store_.switch_to(path_for(class_name_, "gifts"));
		}

		std::string_view class_name() const noexcept override { return class_name_; }

		std::vector<StudentData> students() const override
		{
			std::vector<StudentData> result;
			for (const auto &r : students_store_.all())
				if (r.flags & 0x01u) result.push_back(from_record(r));
			return result;
		}

		std::optional<StudentData> find_student(std::uint32_t id) const override
		{
			const auto *r = students_store_.find(id);
			if (!r) return std::nullopt;
			return from_record(*r);
		}

		std::uint32_t create_student(const StudentData &data) override
		{
			auto id = students_store_.add();
			auto *r = students_store_.find(id);
			if (!r) throw std::logic_error("刚新建的学生槽位找不到");
			to_record(*r, data);
			students_store_.mark_dirty();
			return id;
		}

		bool update_student(std::uint32_t id, const StudentData &data) override
		{
			auto *r = students_store_.find(id);
			if (!r) return false;
			to_record(*r, data);
			students_store_.mark_dirty();
			return true;
		}

		bool remove_student(std::uint32_t id) override { return students_store_.remove(id); }

		std::vector<RuleData> rules() const override { return rules_from(rules_store_); }
		std::optional<RuleData> find_rule(std::uint32_t id) const override { return find_rule_in(rules_store_, id); }
		std::uint32_t create_rule(const RuleData &data) override { return create_rule_in(rules_store_, data); }
		bool update_rule(std::uint32_t id, const RuleData &data) override { return update_rule_in(rules_store_, id, data); }
		bool remove_rule(std::uint32_t id) override { return rules_store_.remove(id); }

		std::vector<GiftData> gifts() const override { return rules_from(gifts_store_); }
		std::optional<GiftData> find_gift(std::uint32_t id) const override { return find_rule_in(gifts_store_, id); }
		std::uint32_t create_gift(const GiftData &data) override { return create_rule_in(gifts_store_, data); }
		bool update_gift(std::uint32_t id, const GiftData &data) override { return update_rule_in(gifts_store_, id, data); }
		bool remove_gift(std::uint32_t id) override { return gifts_store_.remove(id); }

		void reload() override { students_store_.load(); rules_store_.load(); gifts_store_.load(); }
		void save() override { students_store_.save(); rules_store_.save(); gifts_store_.save(); }

	private:
		static StudentData from_record(const Record &r)
		{
			return {.id = r.id, .name = extract_string(r.name, sizeof(r.name)), .gender = extract_string(r.gender, sizeof(r.gender)),
				.old_score = r.old_score, .score = r.score, .old_rank = r.old_rank, .rank = r.rank};
		}

		static void to_record(Record &r, const StudentData &data)
		{
			copy_utf8_truncated(r.name, sizeof(r.name), data.name);
			copy_utf8_truncated(r.gender, sizeof(r.gender), data.gender);
			r.old_score = data.old_score; r.score = data.score;
			r.old_rank = data.old_rank; r.rank = data.rank;
		}

		template <typename T>
		static RuleData from_rule(const T &r)
		{
			return {.id = r.id, .description = extract_string(r.desc, sizeof(r.desc)), .delta = r.delta};
		}

		template <typename T>
		static std::vector<RuleData> rules_from(const FileStore<T> &store)
		{
			std::vector<RuleData> result;
			for (const auto &r : store.all()) if (r.flags & 0x01u) result.push_back(from_rule(r));
			return result;
		}

		template <typename T>
		static std::optional<RuleData> find_rule_in(const FileStore<T> &store, std::uint32_t id)
		{
			const auto *r = store.find(id);
			return r ? std::optional<RuleData>{from_rule(*r)} : std::nullopt;
		}

		template <typename T>
		static std::uint32_t create_rule_in(FileStore<T> &store, const RuleData &data)
		{
			auto id = store.add(); auto *r = store.find(id);
			if (!r) throw std::logic_error("刚新建的规则槽位找不到");
			copy_utf8_truncated(r->desc, sizeof(r->desc), data.description); r->delta = data.delta;
			store.mark_dirty(); return id;
		}

		template <typename T>
		static bool update_rule_in(FileStore<T> &store, std::uint32_t id, const RuleData &data)
		{
			auto *r = store.find(id); if (!r) return false;
			copy_utf8_truncated(r->desc, sizeof(r->desc), data.description); r->delta = data.delta;
			store.mark_dirty(); return true;
		}

		std::string class_name_;
		FileStore<Record> students_store_;
		FileStore<RuleRecord> rules_store_;
		FileStore<GiftRecord> gifts_store_;
	};

	std::unique_ptr<IStorageProvider> make_storage_provider(StorageBackend backend, std::string class_name)
	{
		switch (backend)
		{
		case StorageBackend::file: return std::make_unique<FileStorageProvider>(std::move(class_name));
		}
		throw std::invalid_argument("未知存储后端");
	}
}
