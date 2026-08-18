// Backend-neutral storage contract and domain transfer types.
module;

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module storage.provider;

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
}
