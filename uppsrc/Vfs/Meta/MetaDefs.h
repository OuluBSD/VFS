#ifndef _Vfs_Meta_MetaDefs_h_
#define _Vfs_Meta_MetaDefs_h_

#define DEBUG_METANODE_DTOR 0
#define USE_META_BIN 1

#if USE_META_BIN
	#define META_FILENAME "Meta.bin"
	#define META_EXISTS_FN FileExists
#else
	#define META_FILENAME "Meta.d"
	#define META_EXISTS_FN DirectoryExists
#endif

#endif
