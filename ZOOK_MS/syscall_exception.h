#pragma once
#pragma warning(disable:4996)
#ifndef _SYSCALL_EXCEPTION_
#define _SYSCALL_EXCEPTION_
/*
ϵͳ������쳣����
*/

//#include "mooon/sys/config.h"
#include "exception.h"

#define THROW_SYSCALL_EXCEPTION(errmsg, errcode, syscall) \
    CSyscallException(errmsg, errcode, __FILE__, __LINE__, syscall)

//YS_NAMESPACE_BEGIN

/** ϵͳ���ó����쳣������ϵͳ���ó���ʱ�����Դ��쳣�ͷ����������� */
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
	std::string _syscall; // �ĸ�ϵͳ����ʧ����
};

//SYS_NAMESPACE_END
#endif