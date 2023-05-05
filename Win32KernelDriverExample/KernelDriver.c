/*
	-- cmd Commands to test this application --
	--		Run cmd as administrator		 --

	bcdedit /set testsigning on
	*REBOOT MACHINE, BIOS MUST HAVE SAFEMODE DISABLED*

	sc delete <YourDriverName>
	sc create <YourDriverName> type= kernel binpath="<Exact path where <YourDriverName>.sys is located>"
	sc start <YourDriverName>
	*CONSOLE OUTPUT VISIBLE IN DbgView64*
	sc stop <YourDriverName>
*/

#pragma warning (disable : 4100)

#include "KernelDriver.h"

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	pDriverObject->DriverUnload = UnloadDriver;
	DbgPrintEx(0, 0, "Driver loaded.\n");

	return STATUS_SUCCESS;
}

NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	DbgPrintEx(0, 0, "Driver unloaded.\n");
	return STATUS_SUCCESS;
}