// Domain-facing storage API. Backends implement IStorageProvider; callers do not
// depend on the binary record layout used by the file backend.
module;

#include <cstdint>
#include <stdexcept>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module storage.data;

import storage;

export namespace points
{
	struct StudentData
	{
		std::uint32_t id = 0;
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
		std::uint32_t id = 0;
		std::string  description;
		std::int32_t delta = 0;

		auto operator<=>(const RuleData &) const = default;
	};

	using GiftData = RuleData;

	enum class StorageBackend
	{
		file
	};

	class IStorageProvider
	{
	public:
		virtual ~IStorageProvider() = default;

		virtual void switch_class(std::string class_name) = 0;
		virtual std::string_view class_name() const noexcept = 0;

		virtual std::vector<StudentData> students() const = 0;
		virtual std::optional<StudentData> find_student(std::uint32_t id) const = 0;
		virtual std::uint32_t create_student(const StudentData &) = 0;
		virtual bool update_student(std::uint32_t id, const StudentData &) = 0;
		virtual bool remove_student(std::uint32_t id) = 0;

		virtual std::vector<RuleData> rules() const = 0;
		virtual std::optional<RuleData> find_rule(std::uint32_t id) const = 0;
		virtual std::uint32_t create_rule(const RuleData &) = 0;
		virtual bool update_rule(std::uint32_t id, const RuleData &) = 0;
		virtual bool remove_rule(std::uint32_t id) = 0;

		virtual std::vector<GiftData> gifts() const = 0;
		virtual std::optional<GiftData> find_gift(std::uint32_t id) const = 0;
		virtual std::uint32_t create_gift(const GiftData &) = 0;
		virtual bool update_gift(std::uint32_t id, const GiftData &) = 0;
		virtual bool remove_gift(std::uint32_t id) = 0;

		virtual void reload() = 0;
		virtual void save() = 0;
	};

	std::unique_ptr<IStorageProvider> make_storage_provider(
		StorageBackend backend,
		std::string class_name = "default");

	class StudentRepository
	{
	public:
		explicit StudentRepository(IStorageProvider &provider) : provider_(&provider) {}
		std::vector<StudentData> all() const { return provider_->students(); }
		std::optional<StudentData> find(std::uint32_t id) const { return provider_->find_student(id); }
		std::uint32_t create(const StudentData &data) { return provider_->create_student(data); }
		bool update(std::uint32_t id, const StudentData &data) { return provider_->update_student(id, data); }
		bool remove(std::uint32_t id) { return provider_->remove_student(id); }

	private:
		IStorageProvider *provider_;
	};

	class RuleRepository
	{
	public:
		explicit RuleRepository(IStorageProvider &provider) : provider_(&provider) {}
		std::vector<RuleData> all() const { return provider_->rules(); }
		std::optional<RuleData> find(std::uint32_t id) const { return provider_->find_rule(id); }
		std::uint32_t create(const RuleData &data) { return provider_->create_rule(data); }
		bool update(std::uint32_t id, const RuleData &data) { return provider_->update_rule(id, data); }
		bool remove(std::uint32_t id) { return provider_->remove_rule(id); }

	private:
		IStorageProvider *provider_;
	};

	class GiftRepository
	{
	public:
		explicit GiftRepository(IStorageProvider &provider) : provider_(&provider) {}
		std::vector<GiftData> all() const { return provider_->gifts(); }
		std::optional<GiftData> find(std::uint32_t id) const { return provider_->find_gift(id); }
		std::uint32_t create(const GiftData &data) { return provider_->create_gift(data); }
		bool update(std::uint32_t id, const GiftData &data) { return provider_->update_gift(id, data); }
		bool remove(std::uint32_t id) { return provider_->remove_gift(id); }

	private:
		IStorageProvider *provider_;
	};

	class DataStorage
	{
	public:
		explicit DataStorage(std::unique_ptr<IStorageProvider> provider)
			: provider_(std::move(provider))
			, students_(require_provider(provider_))
			, rules_(require_provider(provider_))
			, gifts_(require_provider(provider_))
		{}

		explicit DataStorage(
			std::string class_name = "default",
			StorageBackend backend = StorageBackend::file)
			: DataStorage(make_storage_provider(backend, std::move(class_name)))
		{}

		void switch_class(std::string class_name) { provider_->switch_class(std::move(class_name)); }
		[[nodiscard]] std::string_view class_name() const noexcept { return provider_->class_name(); }
		StudentRepository &students() noexcept { return students_; }
		RuleRepository &rules() noexcept { return rules_; }
		GiftRepository &gifts() noexcept { return gifts_; }
		void reload() { provider_->reload(); }
		void save() { provider_->save(); }

	private:
		static IStorageProvider &require_provider(const std::unique_ptr<IStorageProvider> &provider)
		{
			if (!provider) throw std::invalid_argument("存储后端不能为空");
			return *provider;
		}
		std::unique_ptr<IStorageProvider> provider_;
		StudentRepository students_;
		RuleRepository rules_;
		GiftRepository gifts_;
	};
}
