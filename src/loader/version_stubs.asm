; Export stubs for the version.dll proxy: each one jumps straight into the real system version.dll.
; Order must match kExports[] in version_proxy.cpp.
extern g_versionProcs:qword

STUB macro name, index
name proc
    jmp qword ptr [g_versionProcs + index * 8]
name endp
endm

.code
STUB proxy_GetFileVersionInfoA, 0
STUB proxy_GetFileVersionInfoByHandle, 1
STUB proxy_GetFileVersionInfoExA, 2
STUB proxy_GetFileVersionInfoExW, 3
STUB proxy_GetFileVersionInfoSizeA, 4
STUB proxy_GetFileVersionInfoSizeExA, 5
STUB proxy_GetFileVersionInfoSizeExW, 6
STUB proxy_GetFileVersionInfoSizeW, 7
STUB proxy_GetFileVersionInfoW, 8
STUB proxy_VerFindFileA, 9
STUB proxy_VerFindFileW, 10
STUB proxy_VerInstallFileA, 11
STUB proxy_VerInstallFileW, 12
STUB proxy_VerLanguageNameA, 13
STUB proxy_VerLanguageNameW, 14
STUB proxy_VerQueryValueA, 15
STUB proxy_VerQueryValueW, 16
end
