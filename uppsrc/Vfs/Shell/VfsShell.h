#ifndef _VfsShell_VfsShell_h_
#define _VfsShell_VfsShell_h_

#include <Core/Core.h>
#include <Vfs/Core/VfsBase.h>  // Using Core/VfsBase instead of full Vfs package

#include "ShellSyntax.h"  // Include the new shell syntax parser
#include "NcursesEditor.h"

NAMESPACE_UPP

// VfsShell - A command-line interface for VFS operations
class VfsShellHostBase {
public:
	virtual ~VfsShellHostBase() {}
	virtual bool Command(class VfsShellConsole& shell, const ValueArray& args) = 0;
	virtual String GetOutput() const = 0;
	virtual void ClearOutput() = 0;
	virtual void AddOutput(const String& s) = 0;
	virtual void AddOutputLine(const String& s = String()) = 0;
	
	// New methods to support exit codes for logical operators
	virtual int GetLastExitCode() const { return 0; } // Default to success
	virtual void SetExitCode(int code) {} // Default implementation does nothing
};

class VfsShellConsole {
	VfsShellHostBase& host;
	String            cwd;       // Current working directory (system path by default)
	String            line_header;
	ValueMap          vars;      // Shell variables
	Vector<String>    history;   // Command history
	int               history_index; // Current position in history

public:
	VfsShellConsole(VfsShellHostBase& h);

	void Execute();
	void PrintLineHeader();

	// Path operations
	bool SetCurrentDirectory(const String& path);
	const String& GetCurrentDirectory() const { return cwd; }

	// History operations
	void AddToHistory(const String& command);
	String GetHistoryEntry(int index) const;
	int GetHistorySize() const { return history.GetCount(); }

	// Check if path is in VFS overlay
	static bool IsVfsPath(const String& path);
	// Convert path to internal format (system or VFS)
	static String ConvertToInternalPath(const String& path, const String& cwd);
	// Convert internal path back to external format
	static String ConvertFromInternalPath(const String& path);

	// Command implementations
	void CmdHelp(const ValueArray& args);
	void CmdPwd();
	void CmdCd(const ValueArray& args);
	void CmdLs(const ValueArray& args);
	void CmdTree(const ValueArray& args);
	void CmdMkdir(const ValueArray& args);
	void CmdTouch(const ValueArray& args);
	void CmdRm(const ValueArray& args);
	void CmdMv(const ValueArray& args);
	void CmdLink(const ValueArray& args);
	void CmdExport(const ValueArray& args);
	void CmdCat(const ValueArray& args);
	void CmdGrep(const ValueArray& args);
	void CmdRg(const ValueArray& args);
	void CmdHead(const ValueArray& args);
	void CmdTail(const ValueArray& args);
	void CmdUniq(const ValueArray& args);
	void CmdCount(const ValueArray& args);
	void CmdHistory(const ValueArray& args);
	void CmdRandom(const ValueArray& args);
	void CmdTrueFalse(const ValueArray& args);
	void CmdEcho(const ValueArray& args);
	void CmdEdit(const ValueArray& args);

	// U++/IDE-specific commands
	void CmdIdeBuild(const ValueArray& args);
	void CmdIdeWorkspace(const ValueArray& args);
	void CmdIdePackage(const ValueArray& args);
	void CmdIdeInstall(const ValueArray& args);
	void CmdIdeUninstall(const ValueArray& args);

	// U++ builder support
	void CmdUppBuilderLoad(const ValueArray& args);
	void CmdUppBuilderAdd(const ValueArray& args);
	void CmdUppBuilderList(const ValueArray& args);
	void CmdUppBuilderActiveSet(const ValueArray& args);
	void CmdUppBuilderGet(const ValueArray& args);
	void CmdUppBuilderSet(const ValueArray& args);
	void CmdUppBuilderDump(const ValueArray& args);
	void CmdUppBuilderActiveDump(const ValueArray& args);

	// U++ startup support
	void CmdUppStartupLoad(const ValueArray& args);
	void CmdUppStartupList(const ValueArray& args);
	void CmdUppStartupOpen(const ValueArray& args);

	// U++ assembly support
	void CmdUppAsmLoad(const ValueArray& args);
	void CmdUppAsmCreate(const ValueArray& args);
	void CmdUppAsmList(const ValueArray& args);
	void CmdUppAsmScan(const ValueArray& args);
	void CmdUppAsmLoadHost(const ValueArray& args);
	void CmdUppAsmRefresh(const ValueArray& args);

	// U++ workspace support
	void CmdUppWkspOpen(const ValueArray& args);
	void CmdUppWkspClose(const ValueArray& args);
	void CmdUppWkspPkgList(const ValueArray& args);
	void CmdUppWkspPkgSet(const ValueArray& args);
	void CmdUppWkspFileList(const ValueArray& args);
	void CmdUppWkspFileSet(const ValueArray& args);
	void CmdUppWkspBuild(const ValueArray& args);

	// U++ GUI support
	void CmdUppGui(const ValueArray& args);

	// C++ AST parsing support
	void CmdParseFile(const ValueArray& args);
	void CmdParseDump(const ValueArray& args);
	void CmdParseGenerate(const ValueArray& args);
	
	// Qwen AI assistant support
	void CmdQwen(const ValueArray& args);
	
	// Method to execute commands from a file - Public for main function
	bool ExecuteFile(const String& filepath);
	
	// Public member for command input/output access
	String output;

private:
	void ClearOutput() { host.ClearOutput(); }
	void AddOutput(const String& s) { host.AddOutput(s); }
	void AddOutputLine(const String& s = String()) { host.AddOutputLine(s); }
	
	// Internal methods to execute AST nodes
	String ExecuteCmdNode(CmdNode* node);
};

END_UPP_NAMESPACE

#endif