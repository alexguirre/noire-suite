#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		MessageBox(NULL, "Attached", "Attached", MB_OK);
	}

	return TRUE;
}