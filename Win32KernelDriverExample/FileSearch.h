#pragma once
#include <ntifs.h>

NTSTATUS SearchForFile(IN PUNICODE_STRING Directory, IN PUNICODE_STRING Target, OUT PBOOLEAN Found);