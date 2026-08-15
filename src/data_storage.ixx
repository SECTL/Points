// Storage Module - C++20 modules with C++23 features
// Modern OOP storage layer for points system
module;

#include <span>
#include <string>
#include <cstdint>
#include <format>
#include <filesystem>

export module storage.data;

import storage;


export namespace points
{
	// 数据传输对象：业务层实体
	struct StudentData
	{
		std::string   name;
		std::string   gender;
		std::int64_t  old_score = 0;
		std::int64_t  score     = 0;
		std::uint32_t old_rank  = 0;
		std::uint32_t rank      = 0;

		auto operator<=>(const StudentData &) const = default;
	};

	struct RuleData
	{
		std::string  description;
		std::int32_t delta = 0;

		auto operator<=>(const RuleData &) const = default;
	};

	using GiftData = RuleData;

	// Repository 基础设施：封装 FileStore，提供 CRUD
	class StudentRepository
	{
	public:
		explicit StudentRepository(FileStore<Record> &store) : store_(&store)
		{}

		std::span<const Record> all() const
		{
			return store_->all();
		}

		const Record *find(std::uint32_t id) const
		{
			return store_->find(id);
		}

		std::uint32_t create(const StudentData &data);
		bool          update(std::uint32_t id, const StudentData &data);
		bool          remove(std::uint32_t id)
		{
			return store_->remove(id);
		}

	private:
		FileStore<Record> *store_;
	};

	class RuleRepository
	{
	public:
		explicit RuleRepository(FileStore<RuleRecord> &store) : store_(&store)
		{}

		std::span<const RuleRecord> all() const
		{
			return store_->all();
		}

		const RuleRecord *find(std::uint32_t id) const
		{
			return store_->find(id);
		}

		std::uint32_t create(const RuleData &data);
		bool          update(std::uint32_t id, const RuleData &data);
		bool          remove(std::uint32_t id)
		{
			return store_->remove(id);
		}

	private:
		FileStore<RuleRecord> *store_;
	};

	class GiftRepository
	{
	public:
		explicit GiftRepository(FileStore<GiftRecord> &store) : store_(&store)
		{}

		std::span<const GiftRecord> all() const
		{
			return store_->all();
		}

		const GiftRecord *find(std::uint32_t id) const
		{
			return store_->find(id);
		}

		std::uint32_t create(const GiftData &data);
		bool          update(std::uint32_t id, const GiftData &data);
		bool          remove(std::uint32_t id)
		{
			return store_->remove(id);
		}

	private:
		FileStore<GiftRecord> *store_;
	};

	// 班级级数据存储门面：持有三个表的 FileStore + Repository
	class DataStorage
	{
	public:
		explicit DataStorage(std::string class_name = "default")
			: class_name_(std::move(class_name)),
			  students_store_(path_for(class_name_, "students")),
			  rules_store_(path_for(class_name_, "rules")),
			  gifts_store_(path_for(class_name_, "gifts")),
			  students_(students_store_),
			  rules_(rules_store_),
			  gifts_(gifts_store_)
		{}

		void switch_class(std::string class_name)
		{
			class_name_ = std::move(class_name);
			students_store_.switch_to(path_for(class_name_, "students"));
			rules_store_.switch_to(path_for(class_name_, "rules"));
			gifts_store_.switch_to(path_for(class_name_, "gifts"));
		}

		std::string_view class_name() const noexcept
		{
			return class_name_;
		}

		StudentRepository &students() noexcept
		{
			return students_;
		}

		RuleRepository &rules() noexcept
		{
			return rules_;
		}

		GiftRepository &gifts() noexcept
		{
			return gifts_;
		}

		void reload()
		{
			students_store_.load();
			rules_store_.load();
			gifts_store_.load();
		}

		void save()
		{
			students_store_.save();
			rules_store_.save();
			gifts_store_.save();
		}

	private:
		static std::filesystem::path path_for(std::string_view class_name, std::string_view stem)
		{
			if (class_name.empty() || class_name == "default")
				return std::format("{}.dat", stem);
			return std::format("{}_{}.dat", stem, class_name);
		}

		std::string class_name_;

		FileStore<Record>     students_store_;
		FileStore<RuleRecord> rules_store_;
		FileStore<GiftRecord> gifts_store_;

		StudentRepository students_;
		RuleRepository    rules_;
		GiftRepository    gifts_;
	};
} // namespace points
