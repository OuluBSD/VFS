#ifndef _Vfs_Shell_Utils_h_
#define _Vfs_Shell_Utils_h_

#include <Core/Core.h>
#include <Vfs/Core/VfsBase.h>

NAMESPACE_UPP

// Utility function to join a string path with a VfsPath
inline VfsPath JoinPath(const String& basePath, const VfsPath& relativePath) {
    VfsPath base;
    base.Set(basePath);
    return base / relativePath;
}

END_UPP_NAMESPACE

#endif
