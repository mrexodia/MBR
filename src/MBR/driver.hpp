#ifdef __cplusplus
extern "C"
{
#endif

#include <ntifs.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntstrsafe.h>
#include <ntimage.h>
#include <wdm.h>
#ifdef __cplusplus
}
#endif

#include <TraceLoggingProvider.h>
#include <evntrace.h>

// Seperate ETW providers. Driver information || Event information 
_tlg_EXTERN_C_CONST TraceLoggingHProvider g_LoggingProviderDriver;
_tlg_EXTERN_C_CONST TraceLoggingHProvider g_LoggingProviderEvents;
