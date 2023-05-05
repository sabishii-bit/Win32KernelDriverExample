#include "FileSearch.h"

NTSTATUS SearchForFile(IN PUNICODE_STRING Directory, IN PUNICODE_STRING Target, OUT PBOOLEAN Found)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IOStatusBlock;
	HANDLE hDirectory;
	NTSTATUS Status;

	// Specifications for the properties of the object directory we want to open.
	InitializeObjectAttributes(
		&ObjectAttributes,
		Directory,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	);

	// Open the directory using ZwCreateFile with appropriate flags for directory listing.
	Status = ZwCreateFile(
		&hDirectory,
		FILE_LIST_DIRECTORY | SYNCHRONIZE,
		&ObjectAttributes,
		&IOStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
		NULL,
		0
	);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Failed to open directory: %wZ\n", Directory));
		return Status;
	}

	// Allocate a buffer to store the directory entries.
	ULONG BufferLength = 1024;
	PVOID Buffer = ExAllocatePoolWithTag(NonPagedPoolNx, BufferLength, 'frch');
	if (!Buffer)
	{
		ZwClose(hDirectory);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	while (TRUE)
	{
		// Query the directory entries using ZwQueryDirectoryFile with the FileDirectoryInformation information class.
		Status = ZwQueryDirectoryFile(
			hDirectory,
			NULL,
			NULL,
			NULL,
			&IOStatusBlock,
			Buffer,
			BufferLength,
			FileDirectoryInformation,
			TRUE,
			Target,
			FALSE
		);

		if (Status == STATUS_NO_MORE_FILES)
		{
			KdPrint(("No more files in directory: %wZ\n", directory));
			break;
		}

		if (!NT_SUCCESS(Status))
		{
			KdPrint(("Failed to query directory: %wZ\n", directory));
			break;
		}

		// Iterate through the directory entries returned in the buffer.
		FILE_DIRECTORY_INFORMATION* fileInfo = (FILE_DIRECTORY_INFORMATION*)Buffer;
		do
		{
			UNICODE_STRING fileName;
			fileName.Buffer = fileInfo->FileName;
			fileName.Length = fileInfo->FileNameLength;
			fileName.MaximumLength = fileInfo->FileNameLength;

			// Compare the current directory entry's file name with the target file name.
			if (RtlCompareUnicodeString(Target, &fileName, TRUE) == 0)
			{
				KdPrint(("Found file: %wZ\\%wZ\n", directory, &fileName));
				*Found = TRUE;
				break;
			}

			// Check if the current directory entry is a subdirectory.
			if (fileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// Recursively search subdirectories.
				UNICODE_STRING subDirectory;
				RtlInitEmptyUnicodeString(&subDirectory, NULL, 0);
				subDirectory.MaximumLength = Directory->Length + sizeof(WCHAR) + fileInfo->FileNameLength;
				subDirectory.Buffer = ExAllocatePoolWithTag(NonPagedPoolNx, subDirectory.MaximumLength, 'dstr');

				if (subDirectory.Buffer)
				{
					// Construct the full path of the subdirectory.
					RtlCopyUnicodeString(&subDirectory, Directory);
					RtlAppendUnicodeToString(&subDirectory, L"\\");
					RtlAppendUnicodeStringToString(&subDirectory, &fileName);

					// Call search_for_file recursively for the subdirectory.
					SearchForFile(&subDirectory, Target, Found);
					ExFreePoolWithTag(subDirectory.Buffer, 'dstr');
				}
				// If there is no NextEntryOffset, it means we reached the last entry in the buffer.
				if (fileInfo->NextEntryOffset == 0)
				{
					break;
				}

				// Move to the next entry in the buffer using NextEntryOffset.
				fileInfo = (FILE_DIRECTORY_INFORMATION*)((PCHAR)fileInfo + fileInfo->NextEntryOffset);
			}
		} while (TRUE);

		// Free the allocated buffer and close the directory handle.
		ExFreePoolWithTag(Buffer, 'frch');
		ZwClose(hDirectory);

		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;

}