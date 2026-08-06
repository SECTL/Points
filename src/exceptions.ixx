//
// Created by jimmy on 2026/8/4.
//
module;
#include <stdexcept>

export module exceptions;

export namespace points
{
	struct file_format_error : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
}
