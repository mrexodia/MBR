#include "driver.hpp"
#include "log.hpp"

#define PREFIX "[MBR] "
#define PREFIX_LEN (sizeof(PREFIX) - 1)

#ifdef _DEBUG
void Log(_In_z_ _Printf_format_string_ const char* format, ...)
{
	// Format the string
	char msg[1024] = PREFIX;
	va_list vl;
	va_start(vl, format);
	const int n = _vsnprintf(msg + PREFIX_LEN, sizeof(msg) - PREFIX_LEN - 2, format, vl);
	va_end(vl);

	// Append CRLF
	auto index = PREFIX_LEN + n;
	msg[index++] = '\r';
	msg[index++] = '\n';
	msg[index++] = '\0';

	// Log to the debugger
	KdPrint((msg));
}
#else
void Log(_In_z_ _Printf_format_string_ const char* format, ...)
{
	return;
}
#endif
