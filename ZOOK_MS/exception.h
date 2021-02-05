#pragma once
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

//#include "string_formatter.h"
#include <string>
#include <exception>

// 抛出CException异常
#define THROW_EXCEPTION(errmsg, errcode) \
    throw CException(errmsg, errcode, __FILE__, __LINE__)

// 抛出用户定制异常
#define THROW_CUSTOM_EXCEPTION(errmsg, errcode, exception_class) \
    throw exception_class(errmsg, errcode, __FILE__, __LINE__)

//UTILS_NAMESPACE_BEGIN

// 异常基类，继承自标准库的exception
class CException : public std::exception
{
public:
	// errmsg 错误信息
	// errcode 错误代码
	// file 发生错误的源代码文件
	// line 发生错误的代码行号
	CException(const char* errmsg, int errcode, const char* file, int line) throw ();
	CException(const std::string& errmsg, int errcode, const std::string& file, int line) throw ();
	virtual ~CException() throw () {}

	virtual const char* what() const throw ();
	const char* file() const throw ();
	int line() const throw ();
	int errcode() const throw ();

	// 返回一个可读的字符串
	virtual std::string str() const throw ();

private:
	virtual std::string prefix() const throw ();

private:
	void init(const char* errmsg, int errcode, const char* file, int line) throw ();

protected:
	std::string _errmsg;
	int _errcode;
	std::string _file;
	int _line;
};

//UTILS_NAMESPACE_END

#endif