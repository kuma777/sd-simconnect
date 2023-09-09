#pragma once

#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#define LOG_EXPORT_BEGIN(path) Logger::Instance().BeginExport(path)
#define LOG_EXPORT_END Logger::Instance().EndExport

#define LOG_ERROR(...) Logger::Instance().Error(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::Instance().Debug(__VA_ARGS__)

class Logger
{
private:
	bool export_{ false };

	std::mutex mutex_;

	std::fstream stream_;

	std::deque<std::pair<int, std::string>> history_;

public:
	Logger()
	{
	}

	~Logger()
	{
	}

	void
	BeginExport(const char* path)
	{
		try
		{
			stream_ = std::fstream(path, std::fstream::out);
			export_ = true;
		}
		catch (std::exception e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	void
	EndExport()
	{
		try
		{
			stream_.close();
		}
		catch (std::exception e)
		{
			std::cerr << e.what() << std::endl;
		}
	}

	template <typename... Args>
	void
	Error(const char* format, Args... args)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		char buffer[256];
		std::snprintf(buffer, sizeof(buffer), format, args...);

		history_.emplace_front(3, buffer);

		while (history_.size() > 30)
		{
			history_.pop_back();
		}

		if (export_)
		{
			stream_ << "[ERROR] " << buffer << "\n";
		}

		std::cout << "[ERROR] " << buffer << "\n";
	}

	template <typename... Args>
	void
	Debug(const char* format, Args... args)
	{
#ifdef NDEBUG
		return;
#endif

		std::lock_guard<std::mutex> lock(mutex_);

		char buffer[1024];
		std::snprintf(buffer, sizeof(buffer), format, args...);

		history_.emplace_front(1, buffer);

		while (history_.size() > 30)
		{
			history_.pop_back();
		}

		if (export_)
		{
			stream_ << "[DEBUG] " << buffer << "\n";
		}

		std::cout << "[ERROR] " << buffer << "\n";
	}

	const std::deque<std::pair<int, std::string>>& getHistory() const
	{
		return history_;
	}

	static Logger&
	Instance()
	{
		static Logger self;
		return self;
	}
};