#include <sstream>
#include <string.h>
//#include "sys/utils.h"
#include "syscall_exception.h"
//SYS_NAMESPACE_BEGIN

CSyscallException::CSyscallException(
	const char* errmsg, int errcode, const char* file, int line, const char* syscall) throw ()
	: CException(errmsg, errcode, file, line)
{
	if (NULL == errmsg)
		_errmsg = strerror(errno);

	if (syscall != NULL)
		_syscall = syscall;
}

CSyscallException::CSyscallException(
	const std::string& errmsg, int errcode, const std::string& file, int line, const std::string& syscall) throw ()
	: CException(errmsg, errcode, file, line)
{
	if (errmsg.empty())
		_errmsg = strerror(errno);

	if (!syscall.empty())
		_syscall = syscall;
}

std::string CSyscallException::str() const throw ()
{
	std::stringstream ss;

	ss << prefix() << _errmsg << "@" << _file << ":" << _line;
	if (!_syscall.empty())
		ss << "#" << _syscall;

	return ss.str();
}

const char* CSyscallException::syscall() const throw ()
{
	return _syscall.c_str();
}

std::string CSyscallException::prefix() const throw ()
{
	return std::string("syscall_exception://");
}