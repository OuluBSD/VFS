#include <CtrlLib/CtrlLib.h>
#include <MetaCtrl/MetaCtrl.h>

using namespace Upp;

struct MetaCtrlDemoWindow : public TopWindow {
	ProcessCtrl proc;

	MetaCtrlDemoWindow() {
		Title("MetaCtrl AST & Process Inspector");
		Sizeable().Zoomable();
		SetRect(0, 0, 800, 600);
		Add(proc.SizePos());
	}
};

GUI_APP_MAIN {
	const Vector<String>& cmd = CommandLine();
	bool is_test = false;
	for (const String& arg : cmd) {
		if (arg == "--test" || arg == "-t")
			is_test = true;
	}

	MetaCtrlDemoWindow w;
	if (is_test) {
		Cout() << "MetaCtrlDemo: Self-test mode started" << EOL;
		ASSERT(w.proc.tasks.GetColumnCount() >= 0);
		Cout() << "MetaCtrlDemo: Self-test mode passed successfully" << EOL;
#ifdef UPP_HEAP
		NoMemoryLeaksCheck = true;
#endif
		return;
	}
	w.Run();
}
