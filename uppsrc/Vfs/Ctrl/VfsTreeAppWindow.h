#ifndef _Vfs_Ctrl_VfsTreeAppWindow_h_
#define _Vfs_Ctrl_VfsTreeAppWindow_h_

#include "VfsTreeApp.h"

NAMESPACE_UPP

class VfsTreeAppWindow : public TopWindow {
public:
	typedef VfsTreeAppWindow CLASSNAME;

	MenuBar    menu;
	StatusBar  status;
	VfsTreeApp app;

	VfsTreeAppWindow();
	virtual ~VfsTreeAppWindow();

	virtual void MainMenu(Bar& bar);
	virtual void FileMenu(Bar& bar);
	virtual void EditMenu(Bar& bar);
	virtual void ViewMenu(Bar& bar);
	virtual void HelpMenu(Bar& bar);

	void         NewFile();
	void         OpenFile();
	void         SaveFile();
	void         SaveFileAs();
	void         About();

	void         UpdateTitle();
};

END_UPP_NAMESPACE

#endif
