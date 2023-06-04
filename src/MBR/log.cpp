#include "driver.hpp"
#include "log.hpp"

#define PREFIX "[MBR] "

void Log(const char* format, ...)
{
    char msg[1024] = PREFIX;
    va_list vl;
    va_start(vl, format);
    const int n = _vsnprintf(msg + sizeof(PREFIX) - 1, sizeof(msg) - sizeof(PREFIX) + 1, format, vl);
    msg[sizeof(PREFIX) - 1 + n] = '\0';
    va_end(vl);
#ifdef _DEBUG
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, msg);
#endif
    va_end(format);
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES objAttr;
    // TODO: use some symbolic link to do this better
    RtlInitUnicodeString(&FileName, L"\\DosDevices\\C:\\MBR.log");
    InitializeObjectAttributes(&objAttr, &FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    if(KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
#ifdef _DEBUG
        DbgPrint(PREFIX "KeGetCurrentIrql != PASSIVE_LEVEL!\n");
#endif
        return;
    }
    HANDLE handle;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS ntstatus = ZwCreateFile(&handle,
                                     FILE_APPEND_DATA,
                                     &objAttr, &ioStatusBlock, NULL,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_WRITE | FILE_SHARE_READ,
                                     FILE_OPEN_IF,
                                     FILE_SYNCHRONOUS_IO_NONALERT,
                                     NULL, 0);
    if(NT_SUCCESS(ntstatus))
    {
        size_t cb;
        ntstatus = RtlStringCbLengthA(msg, sizeof(msg), &cb);
        if(NT_SUCCESS(ntstatus))
            ZwWriteFile(handle, NULL, NULL, NULL, &ioStatusBlock, msg, (ULONG)cb, NULL, NULL);
        ZwClose(handle);
    }
}