#include "exception.h"
//#include "utils/string_utils.h"
#include <sstream>
#include "extract_filename.h"
//UTILS_NAMESPACE_BEGIN

CException::CException(const char* errmsg, int errcode, const char* file, int line) throw ()
{
	init(errmsg, errcode, file, line);
}

CException::CException(const std::string& errmsg, int errcode, const std::string& file, int line) throw ()
{
	init(errmsg.c_str(), errcode, file.c_str(), line);
}

const char* CException::what() const throw ()
{
	return _errmsg.c_str();
}

const char* CException::file() const throw ()
{
	return _file.c_str();
}

int CException::line() const throw ()
{
	return _line;
}

int CException::errcode() const throw ()
{
	return _errcode;
}

std::string CException::str() const throw ()
{
	std::stringstream ss;
	ss << prefix() << "[" << _errcode << "]" << _errmsg << "@" << _file << ":" << _line;

	return ss.str();
}

std::string CException::prefix() const throw ()
{
	return "exception://";
}

void CException::init(const char* errmsg, int errcode, const char* file, int line) throw ()
{
	if (errmsg != NULL)
		_errmsg = errmsg;
	_errcode = errcode;

	_line = line;
	if (file != NULL)
	{
		_file = extract_filename(file);
	}
}