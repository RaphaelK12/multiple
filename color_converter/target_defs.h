#ifndef TARGET_DEFS_H_INCLUDED
#define TARGET_DEFS_H_INCLUDED

#ifdef LINUX_BUILD
#define TARGET_INLINE inline __attribute__((always_inline))
#define TARGET_MEMALIGN(align, size) memalign( (align), (size))
#define TARGET_FREEALIGN free
#elif defined WINDOWS_BUILD
#define TARGET_INLINE __forceinline
#define TARGET_MEMALIGN(align, size) _aligned_malloc( (size), (align) )
#define TARGET_FREEALIGN _aligned_free
#endif

#endif // TARGET_DEFS_H_INCLUDED
