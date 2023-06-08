#include "notify.hpp"
#include "log.hpp"

static PCUNICODE_STRING SafeString(PCUNICODE_STRING str)
{
	static UNICODE_STRING nullStr = RTL_CONSTANT_STRING(L"<nullptr>");
	return str != nullptr ? str : &nullStr;
}

static PUNICODE_STRING GetImageName(HANDLE pid)
{
	PUNICODE_STRING processName = nullptr;
	PEPROCESS process = nullptr;
	if (NT_SUCCESS(PsLookupProcessByProcessId(pid, &process)))
	{
		if (!NT_SUCCESS(SeLocateProcessImageName(process, &processName)))
		{
			processName = nullptr;
		}

		// See: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-pslookupprocessbyprocessid#remarks
		ObDereferenceObject(process);
	}
	return processName;
}

// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nc-ntddk-pcreate_process_notify_routine_ex
_IRQL_requires_max_(PASSIVE_LEVEL)
static void CreateProcessNotifyRoutineEx(PEPROCESS process, HANDLE pid, PPS_CREATE_NOTIFY_INFO createInfo)
{
	UNREFERENCED_PARAMETER(process);
	UNREFERENCED_PARAMETER(pid);

	if (createInfo != nullptr)
	{
		Log("[notify] [process] start %u (%wZ)", HandleToULong(pid), SafeString(createInfo->ImageFileName));
		// TODO: this is technically unsafe
		// See: https://social.msdn.microsoft.com/Forums/windows/en-US/b49cdf12-029c-4272-ac41-a3c9842b1675/how-check-if-a-unicodestring-contains-in-other-unicodestring?forum=wdk
		PCUNICODE_STRING commandLine = createInfo->CommandLine;
		if (commandLine != nullptr && wcsstr(commandLine->Buffer, L"powershell") != nullptr)
		{
			Log("[notify] [process] Access to launch powershell.exe (%u) was denied!", HandleToULong(pid));
			createInfo->CreationStatus = STATUS_ACCESS_DENIED;
		}
	}
	else
	{
		Log("[notify] [process] exit %u", HandleToULong(pid));
	}
}

// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nc-ntddk-pload_image_notify_routine
_IRQL_requires_max_(PASSIVE_LEVEL)
static void LoadImageNotifyRoutine(PUNICODE_STRING imageName, HANDLE pid, PIMAGE_INFO imageInfo)
{
	UNREFERENCED_PARAMETER(imageInfo);

	auto processName = GetImageName(pid);

	Log("[notify] [image] process %u (%wZ) loaded %wZ", HandleToUlong(pid), SafeString(processName), imageName);

	if (processName != nullptr)
	{
		ExFreePool(processName);
	}
}

// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nc-ntddk-pcreate_thread_notify_routine
_IRQL_requires_max_(PASSIVE_LEVEL)
static void CreateThreadNotifyRoutine(HANDLE pid, HANDLE tid, BOOLEAN create)
{
	if (create)
	{
		Log("[notify] [thread] process %u created thread %u", HandleToUlong(pid), HandleToUlong(tid));
	}
	else
	{
		Log("[notify] [thread] process %u killed thread %u", HandleToUlong(pid), HandleToUlong(tid));
	}
}

NTSTATUS NotifyLoad()
{
	// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreateprocessnotifyroutineex
	// Minimum version: Windows Vista SP1
	// NOTE: The PsSetCreateProcessNotifyRoutineEx2 function is available from Windows 10 (1703).
	// Tt seems like the only difference is that it's also invoked for pico processes
	// (WSL1, see: http://blog.deniable.org/posts/windows-callbacks/)
	auto status = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutineEx, FALSE);
	if (!NT_SUCCESS(status))
	{
		Log("[notify] PsSetCreateProcessNotifyRoutineEx failed: %08X", status);
		NotifyUnload();
		return status;
	}

	// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetloadimagenotifyroutine
	// Minimum version: Windows 2000
	// NOTE: PsSetLoadImageNotifyRoutineEx function is available from Windows 10 (1709).
	// TODO: figure out the difference
	status = PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);
	if (!NT_SUCCESS(status))
	{
		Log("[notify] PsSetLoadImageNotifyRoutine failed: %08X", status);
		NotifyUnload();
		return status;
	}

	// Reference: https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreatethreadnotifyroutine
	// Minimum version: Windows XP(?)
	// NOTE: PsSetCreateThreadNotifyRoutineEx function is available from Windows 10
	// The difference is that the Ex version executes the callback on the created thread
	// whereas the regular version executes the callback on the creator thread.
	status = PsSetCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
	if (!NT_SUCCESS(status))
	{
		Log("[notify] PsSetCreateThreadNotifyRoutine failed: %08X", status);
		NotifyUnload();
		return status;
	}

	Log("[notify] Callbacks registered successfully!");

	return status;
}

void NotifyUnload()
{
	PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutineEx, TRUE);
	PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine);
	PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
}
