#include "driver.hpp"
#include "log.hpp"
#include "notify.hpp"

// {61C92C4B-A8E5-4D75-B670-DB621E19ABB8}
TRACELOGGING_DEFINE_PROVIDER(g_LoggingProviderDriver, "MBR", (0x61c92c4b, 0xa8e5, 0x4d75, 0xb6, 0x70, 0xdb, 0x62, 0x1e, 0x19, 0xab, 0xb8));
// {3F410754-3869-4E68-AE10-01064407844D}
TRACELOGGING_DEFINE_PROVIDER(g_LoggingProviderEvents, "MBR", (0x3f410754, 0x3869, 0x4e68, 0xae, 0x10, 0x1, 0x6, 0x44, 0x7, 0x84, 0x4d));


_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
static void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    Log(__FUNCTION__);

    UNICODE_STRING Win32Device = RTL_CONSTANT_STRING(L"\\DosDevices\\MBR");
    (void)IoDeleteSymbolicLink(&Win32Device);

    IoDeleteDevice(DriverObject->DeviceObject);

    // Unregister notify routines
    NotifyUnload();

    TraceLoggingWrite(g_LoggingProviderEvents, "DriverUnload",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION));

    TraceLoggingUnregister(g_LoggingProviderDriver);
    TraceLoggingUnregister(g_LoggingProviderEvents);
}

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
static NTSTATUS DriverCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Log(__FUNCTION__);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
static NTSTATUS DriverDefaultHandler(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    Log(__FUNCTION__);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    TraceLoggingRegister(g_LoggingProviderDriver);
    TraceLoggingRegister(g_LoggingProviderEvents);

    // Set the major function entry points
    DriverObject->DriverUnload = DriverUnload;
    for (unsigned int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = DriverDefaultHandler;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;

    // Create a device
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MBR");
    UNICODE_STRING Win32Device = RTL_CONSTANT_STRING(L"\\DosDevices\\MBR");

    PDEVICE_OBJECT DeviceObject = nullptr;
    auto status = IoCreateDevice(DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject);
    if (!NT_SUCCESS(status))
    {
        Log("IoCreateDevice Error...");
        goto cleanup;
    }

    if (!DeviceObject)
    {
        Log("Unexpected I/O Error...");
        status = STATUS_UNEXPECTED_IO_ERROR;
        goto cleanup;
    }

    status = IoCreateSymbolicLink(&Win32Device, &DeviceName);
    if (!NT_SUCCESS(status))
    {
        Log("Failed to create symbolic link...");
        goto cleanup;
    }

    status = NotifyLoad();
    if (!NT_SUCCESS(status))
    {
        Log("Failed to register notify routines");
        goto cleanup;
    }

    Log(__FUNCTION__);

    TraceLoggingWrite(g_LoggingProviderEvents, "DriverLoad",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION));


cleanup:
    if (!NT_SUCCESS(status))
    {
        (void)IoDeleteSymbolicLink(&Win32Device);

        if (DriverObject->DeviceObject) 
            IoDeleteDevice(DriverObject->DeviceObject);

        TraceLoggingWrite(g_LoggingProviderEvents, "DriverError",
            TraceLoggingLevel(TRACE_LEVEL_ERROR),
            TraceLoggingValue("Loading", "Message"),
            TraceLoggingValue(status, "Error"));

        TraceLoggingUnregister(g_LoggingProviderDriver);
        TraceLoggingUnregister(g_LoggingProviderEvents);
    }

    return status;
}