#pragma once
#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

//#include "string_formatter.h"
#include <string>
#include <exception>

// �׳�CException�쳣
#define THROW_EXCEPTION(errmsg, errcode) \
    throw CException(errmsg, errcode, __FILE__, __LINE__)

// �׳��û������쳣
#define THROW_CUSTOM_EXCEPTION(errmsg, errcode, exception_class) \
    throw exception_class(errmsg, errcode, __FILE__, __LINE__)

//UTILS_NAMESPACE_BEGIN

// �쳣���࣬�̳��Ա�׼���exception
class CException : public std::exception
{
public:
	// errmsg ������Ϣ
	// errcode �������
	// file ���������Դ�����ļ�
	// line ��������Ĵ����к�
	CException(const char* errmsg, int errcode, const char* file, int line) throw ();
	CException(const std::string& errmsg, int errcode, const std::string& file, int line) throw ();
	virtual ~CException() throw () {}

	virtual const char* what() const throw ();
	const char* file() const throw ();
	int line() const throw ();
	int errcode() const throw ();

	// ����һ���ɶ����ַ���
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