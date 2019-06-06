#include <slmath/runtime_checks.h>

#ifdef SLMATH_SSE2_MSVC
#ifdef SLMATH_MSVC_HAS_INTRIN_H
#include <intrin.h>
#else // SLMATH_MSVC_HAS_INTRIN_H

extern "C" void __cpuid(int cpuinfo[4], int mode)
{
    /* edi and ebx will be saved and restored automatically */
    __asm
    {
        /* Use edi to index into cpuinfo */
        mov     edi, dword ptr [cpuinfo];

        /* Invoke cpuid with mode in eax */
        mov     eax, mode;
        cpuid;

        /* Store the results in cpuinfo */
        mov     dword ptr [edi +  0], eax;
        mov     dword ptr [edi +  4], ebx;
        mov     dword ptr [edi +  8], ecx;
        mov     dword ptr [edi + 12], edx;
    }
}

#endif // SLMATH_MSVC_HAS_INTRIN_H
#endif // SLMATH_SSE2_MSVC

SLMATH_BEGIN()

bool isSSE2CPU()
{
#ifdef SLMATH_SSE2_MSVC
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);
	bool sse2 = (cpuinfo[3] & (1<<26)) != 0;
	return sse2;
#else
	return false;
#endif
}

SLMATH_END()
