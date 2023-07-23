#include "driver.hpp"
#include "log.hpp"
#include "notify.hpp"

// {3F410754-3869-4E68-AE10-01064407844D}
TRACELOGGING_DEFINE_PROVIDER(g_LoggingProviderEvents, "MBR", (0x3f410754, 0x3869, 0x4e68, 0xae, 0x10, 0x1, 0x6, 0x44, 0x7, 0x84, 0x4d));

_Function_class_(DRIVER_UNLOAD)
_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
static void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
static NTSTATUS DriverCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
static NTSTATUS DriverDefaultHandler(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    TraceLoggingRegister(g_LoggingProviderEvents);

    DriverObject->DriverUnload = DriverUnload;
    for (unsigned int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = DriverDefaultHandler;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverCreateClose;

    PDEVICE_OBJECT DeviceObject = nullptr;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MBR");
    UNICODE_STRING Win32Device = RTL_CONSTANT_STRING(L"\\DosDevices\\MBR");
    bool SymLinkCreated = FALSE;

    auto status = NotifyLoad();
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    status = IoCreateDevice(DriverObject,
        0,
        &DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    if (!DeviceObject)
    {
        status = STATUS_UNEXPECTED_IO_ERROR;
        goto cleanup;
    }

    status = IoCreateSymbolicLink(&Win32Device, &DeviceName);
    if (!NT_SUCCESS(status))
    {
        goto cleanup;
    }
    SymLinkCreated = true;

    TraceLoggingWrite(g_LoggingProviderEvents, "DriverLoad",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION));

cleanup:
    if (!NT_SUCCESS(status))
    {
        if (SymLinkCreated)
            (void)IoDeleteSymbolicLink(&Win32Device);

        if (DriverObject->DeviceObject) 
            IoDeleteDevice(DriverObject->DeviceObject);

        TraceLoggingWrite(g_LoggingProviderEvents, "DriverError",
            TraceLoggingLevel(TRACE_LEVEL_ERROR),
            TraceLoggingValue("Loading", "Message"),
            TraceLoggingValue(status, "Error"));

        TraceLoggingUnregister(g_LoggingProviderEvents);
    }

    return status;
}

static void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING Win32Device = RTL_CONSTANT_STRING(L"\\DosDevices\\MBR");
    (void)IoDeleteSymbolicLink(&Win32Device);

    IoDeleteDevice(DriverObject->DeviceObject);

    // Unregister notify routines
    NotifyUnload();

    TraceLoggingWrite(g_LoggingProviderEvents, "DriverUnload",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION));

    TraceLoggingUnregister(g_LoggingProviderEvents);
}

static NTSTATUS DriverCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

static NTSTATUS DriverDefaultHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

