// 测试入口点：验证新 OOP 存储实现
#include <format>
#include <iostream>
#include <exception>
#include <stdexcept>
import storage.data;

int main()
{
	try
	{
		// 创建存储实例
		points::DataStorage storage{"test_class"};

		std::cout << std::format("当前班级: {}\n", storage.class_name());

		// 测试学生仓库
		points::StudentData student{
			.name      = "测试学生",
			.gender    = "男",
			.old_score = 90,
			.score     = 100,
			.old_rank  = 2,
			.rank      = 1
		};

		auto id = storage.students().create(student);
		std::cout << std::format("创建学生 ID: {}\n", id);

		// 函数查询：返回第一个满足条件的学生
		if (auto found = storage.students().find(
			[id](const points::StudentData &candidate)
			{
				return candidate.id == id && candidate.score == 100;
			}); found)
		{
			std::cout << std::format("找到学生，分数: {}\n", found->score);
		}
		else
		{
			throw std::runtime_error("函数查询未找到刚创建的学生");
		}

		// 函数删除：删除所有满足条件的学生，并返回删除数量
		const auto removed = storage.students().remove(
			[id](const points::StudentData &candidate)
			{
				return candidate.id == id;
			});
		if (removed != 1) throw std::runtime_error("函数删除数量不正确");
		if (storage.students().find(id))
			throw std::runtime_error("函数删除后学生仍然存在");

		// 保存到磁盘
		storage.save();

		std::cout << "✓ OOP 存储层测试通过！\n";
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << std::format("✗ 错误: {}\n", e.what());
		return 1;
	}
}
