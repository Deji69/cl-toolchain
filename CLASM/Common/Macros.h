#pragma once
#if defined(_WIN32)
	#define CLASM_SYSTEM_WINDOWS
	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif
#elif defined(__APPLE__) && defined(__MACH__)
#elif defined(__unix__)
	#if defined(__ANDROID__)
		#define CLASM_SYSTEM_ANDROID
	#elif defined(__linux__)
		#define CLASM_SYSTEM_LINUX
	#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
		#define CLASM_SYSTEM_FREEBSD
	#elif defined(__OpenBSD__)
		#define CLASM_SYSTEM_OPENBSD
	#else
		#error Unknown UNIX OS
	#endif
#else
	#error Unknown OS
#endif

#if defined(_DEBUG)
#define CLASM_DEBUG
#endif
