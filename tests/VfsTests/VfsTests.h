#ifndef _VfsTests_VfsTests_h_
#define _VfsTests_VfsTests_h_

#include <Vfs/Core/Core.h>

NAMESPACE_UPP

struct SampleDocExt : VfsValueExt {
	CLASSTYPE(SampleDocExt)

	String title;
	String content;

	SampleDocExt(VfsValue& n) : VfsValueExt(n) {}

	void Visit(Vis& vis) override {
		vis
			("title", title)
			("content", content);
	}
};

END_UPP_NAMESPACE

#endif
