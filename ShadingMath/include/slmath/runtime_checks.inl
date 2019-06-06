inline bool isValidCPU()
{
#ifdef SLMATH_SSE2_MSVC
	if ( !isSSE2CPU() ) return false;
#endif
	return true;
}
