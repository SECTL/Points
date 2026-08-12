// 测试入口点：验证新 OOP 存储实现
import storage.data;

#include <format>
#include <iostream>
#include <exception>

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

		// 查找学生
		if (auto *found = storage.students().find(id); found)
		{
			std::cout << std::format("找到学生，分数: {}\n", found->score);
		}

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
