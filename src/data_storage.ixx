// Repository facade over a dynamically selected storage provider.
module;

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module storage.data;
export import storage.provider;

export namespace points
{
	class StudentRepository
	{
	public:
		explicit StudentRepository(IStorageProvider &provider) : provider_(&provider) {}
		std::vector<StudentData> all() const { return provider_->students(); }
		std::optional<StudentData> find(std::uint32_t id) const { return provider_->find_student(id); }
		std::uint32_t create(const StudentData &data) { return provider_->create_student(data); }
		bool update(std::uint32_t id, const StudentData &data) { return provider_->update_student(id, data); }
		bool remove(std::uint32_t id) { return provider_->remove_student(id); }

		template <typename Predicate>
			requires std::predicate<Predicate &, const StudentData &>
		std::vector<StudentData> all(Predicate &&predicate) const
		{
			auto result = all();
			std::erase_if(result, [&](const auto &item) { return !std::invoke(predicate, item); });
			return result;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const StudentData &>
		std::optional<StudentData> find(Predicate &&predicate) const
		{
			for (auto &item : all())
				if (std::invoke(predicate, item)) return item;
			return std::nullopt;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const StudentData &>
		std::size_t remove(Predicate &&predicate)
		{
			std::size_t removed = 0;
			for (const auto &item : all())
				if (std::invoke(predicate, item) && provider_->remove_student(item.id)) ++removed;
			return removed;
		}

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

		template <typename Predicate>
			requires std::predicate<Predicate &, const RuleData &>
		std::vector<RuleData> all(Predicate &&predicate) const
		{
			auto result = all();
			std::erase_if(result, [&](const auto &item) { return !std::invoke(predicate, item); });
			return result;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const RuleData &>
		std::optional<RuleData> find(Predicate &&predicate) const
		{
			for (auto &item : all())
				if (std::invoke(predicate, item)) return item;
			return std::nullopt;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const RuleData &>
		std::size_t remove(Predicate &&predicate)
		{
			std::size_t removed = 0;
			for (const auto &item : all())
				if (std::invoke(predicate, item) && provider_->remove_rule(item.id)) ++removed;
			return removed;
		}

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

		template <typename Predicate>
			requires std::predicate<Predicate &, const GiftData &>
		std::vector<GiftData> all(Predicate &&predicate) const
		{
			auto result = all();
			std::erase_if(result, [&](const auto &item) { return !std::invoke(predicate, item); });
			return result;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const GiftData &>
		std::optional<GiftData> find(Predicate &&predicate) const
		{
			for (auto &item : all())
				if (std::invoke(predicate, item)) return item;
			return std::nullopt;
		}

		template <typename Predicate>
			requires std::predicate<Predicate &, const GiftData &>
		std::size_t remove(Predicate &&predicate)
		{
			std::size_t removed = 0;
			for (const auto &item : all())
				if (std::invoke(predicate, item) && provider_->remove_gift(item.id)) ++removed;
			return removed;
		}

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
