#pragma once
#pragma warning(disable:4996)
#ifndef _SYSCALL_EXCEPTION_
#define _SYSCALL_EXCEPTION_
/*
系统报错的异常处理
*/

//#include "mooon/sys/config.h"
#include "exception.h"

#define THROW_SYSCALL_EXCEPTION(errmsg, errcode, syscall) \
    CSyscallException(errmsg, errcode, __FILE__, __LINE__, syscall)

//YS_NAMESPACE_BEGIN

/** 系统调用出错异常，多数系统调用出错时，均以此异常型反馈给调用者 */
class CSyscallException : public CException
{
public:
	CSyscallException(const char* errmsg, int errcode, const char* file, int line, const char* syscall) throw ();
	CSyscallException(const std::string& errmsg, int errcode, const std::string& file, int line, const std::string& syscall) throw ();
	virtual ~CSyscallException() throw () {}

	virtual std::string str() const throw ();
	const char* syscall() const throw ();

private:
	virtual std::string prefix() const throw ();

private:
	std::string _syscall; // 哪个系统调用失败了
};

//SYS_NAMESPACE_END
#endif