#include "VfsShell.h"
#include <Vfs/Core/VfsBase.h>  // For basic VFS operations

#ifdef flagQWEN
#include <Qwen/CmdQwen.h>  // Include for Qwen command
#endif

NAMESPACE_UPP

// Forward declaration for ExecuteCmdNode - we'll implement this after other methods
String ExecuteCmdNodeImpl(VfsShellConsole& console, VfsShellHostBase& host, CmdNode* node);

// Check if path is in VFS overlay
bool VfsShellConsole::IsVfsPath(const String& path) {
	return path.StartsWith("/vfs/");
}

// Convert path to internal format (system or VFS)
String VfsShellConsole::ConvertToInternalPath(const String& path, const String& cwd) {
	if (path.IsEmpty()) return cwd;
	
	// If it's already in the VFS overlay, return as is
	if (IsVfsPath(path)) {
		return path;
	}
	
	// If it's an absolute system path, return as is
	if (path.StartsWith("/")) {
		return path;
	}
	
	// If it's the VFS root
	if (path == "/vfs") {
		return "/vfs/";
	}
	
	// Handle relative paths by resolving against current working directory
	String newPath;
	if (cwd == "/vfs") {
		newPath = "/vfs/";
	} else {
		newPath = cwd;
	}
	
	if (!newPath.EndsWith("/")) newPath += "/";
	newPath += path;
	
	// Normalize the path (handle "..", ".", etc.)
	// We'll use U++'s path utilities where possible
	return NormalizePath(newPath);
}

// Convert internal path back to external format
String VfsShellConsole::ConvertFromInternalPath(const String& path) {
	return path; // For now, just return as is
}

VfsShellConsole::VfsShellConsole(VfsShellHostBase& h) : host(h)
{
	cwd = "/"; // Start in the root directory where system filesystem is mounted
	history_index = -1; // No history position selected initially
}

void VfsShellConsole::CmdHelp(const ValueArray& args) {
	String helpText = 
		"Available commands:\n"
		"  help          - Show this help message\n"
		"  pwd           - Print current working directory\n"
		"  cd [path]     - Change current directory\n"
		"  ls [path]     - List directory contents\n"
		"  tree [path]   - Show directory tree structure\n"
		"  mkdir <path>  - Create a new directory\n"
		"  touch <path>  - Create or update a file\n"
		"  rm <path>     - Remove a file or directory\n"
		"  mv <src> <dst> - Move/rename a file or directory\n"
		"  link <src> <dst> - Create a link\n"
		"  export <vfs> <host> - Export VFS content to host filesystem\n"
		"  cat [paths...] - Concatenate and display file contents\n"
		"  grep [-i] <pattern> [path] - Search for pattern in files\n"
		"  rg [-i] <pattern> [path] - Ripgrep-like search\n"
		"  head [-n N] [path] - Show first N lines of file\n"
		"  tail [-n N] [path] - Show last N lines of file\n"
		"  uniq [path]   - Show unique lines in file\n"
		"  count [path]  - Count lines in file\n"
		"  history [-a | -n N] - Show command history\n"
		"  random [min [max]] - Generate random number\n"
		"  true/false    - Exit with success/failure status\n"
		"  echo <path> <data...> - Display data\n"
		"  edit [path]   - Open full-screen ncurses text editor\n"
		"  qwen [options] - Interactive AI assistant powered by qwen-code\n"
		"  quit/exit     - Exit the shell\n"
		"  \n"
		"IDE/U++ specific commands:\n"
		"  theide        - Execute TheIDE command (e.g. 'theide build MyApp')\n"
		"  idebuild      - Build U++ package (e.g. 'idebuild MyApp')\n"
		"  ideworkspace  - Workspace operations (e.g. 'ideworkspace list')\n"
		"  idepkg        - Package operations (e.g. 'idepkg list', 'idepkg create MyPkg')\n"
		"  ideinstall    - Install U++ package\n"
		"  ideuninstall  - Uninstall U++ package\n"
		"  \n"
		"U++ builder support:\n"
		"  upp.builder.load <directory-path> [-H]      (load all .bm files from directory, -H treats path as OS filesystem path)\n"
		"  upp.builder.add <bm-file-path> [-H]         (load a single .bm build method file)\n"
		"  upp.builder.list                            (list loaded build methods)\n"
		"  upp.builder.active.set <builder-name>       (set the active build method)\n"
		"  upp.builder.get <key>                       (show a key from the active build method)\n"
		"  upp.builder.set <key> <value>               (update a key in the active build method)\n"
		"  upp.builder.dump <builder-name>             (dump all keys for a build method)\n"
		"  upp.builder.active.dump                     (dump keys for the active build method)\n"
		"  \n"
		"U++ startup support:\n"
		"  upp.startup.load <directory-path> [-H] [-v] (load all .var files from directory, -H treats path as OS filesystem path, -v verbose output)\n"
		"  upp.startup.list                            (list all loaded startup assemblies)\n"
		"  upp.startup.open <name> [-v]                (load a named startup assembly, -v for verbose)\n"
		"  \n"
		"U++ assembly support:\n"
		"  upp.asm.load <var-file-path> [-H]           (load U++ assembly file, -H treats path as OS filesystem path)\n"
		"  upp.asm.create <name> <output-path>         (create new U++ assembly)\n"
		"  upp.asm.list [-v]                           (list packages in current assembly, -v for all directories)\n"
		"  upp.asm.scan <directory-path>               (scan directory for U++ packages with .upp files)\n"
		"  upp.asm.load.host <host-var-file>           (mount host dir and load .var file from OS filesystem)\n"
		"  upp.asm.refresh [-v]                        (refresh all packages in active assembly, -v for verbose)\n"
		"  \n"
		"U++ workspace support:\n"
		"  upp.wksp.open <pkg-name> [-v]               (open a package from the list as workspace)\n"
		"  upp.wksp.open -p <path> [-v]                (open a U++ package as workspace from path, -v for verbose)\n"
		"  upp.wksp.close                              (close current workspace)\n"
		"  upp.wksp.pkg.list                           (list packages in current workspace)\n"
		"  upp.wksp.pkg.set <package-name>             (set active package in workspace)\n"
		"  upp.wksp.file.list                          (list files in active package)\n"
		"  upp.wksp.file.set <file-path>               (set active file in editor)\n"
		"  upp.wksp.build [options]                    (build active workspace package and dependencies)\n"
		"  \n"
		"U++ GUI support:\n"
		"  upp.gui                                     (launch U++ assembly IDE GUI)\n"
		"  \n"
		"libclang C++ AST parsing:\n"
		"  parse.file <filepath> [vfs-target-path]     (parse C++ file with libclang)\n"
		"  parse.dump [vfs-path]                       (dump parsed AST tree)\n"
		"  parse.generate <ast-path> <output-path]     (generate C++ code from AST)\n";
	AddOutputLine(helpText);
}

void VfsShellConsole::Execute()
{
	// For console implementation, we'll use the AST parser to handle csh/tcsh syntax
	// In a real console application, we'd read from stdin, but for this integration
	// with the existing framework we'll work with the stored output
	// Process the last command stored in output
	String s = output;
	if (!line_header.IsEmpty() && s.StartsWith(line_header))
		s = s.Mid(line_header.GetCount());  // Remove prompt from line
	s = TrimBoth(s);

	if (s.IsEmpty()) return;

	// Clear the host buffer to capture only new output from this execution
	host.ClearOutput();
	
	// Try to parse and execute command using AST parser
	try {
		// Parse the command string into an AST
		CmdNode* ast = ShellSyntaxParser::ParseString(s);
		if (ast != nullptr) {
			// Execute the AST
			ExecuteCmdNode(ast);
			// Clean up the AST
			delete ast;
		} else {
			// If parsing failed, try the old method as fallback
			Vector<String> parts = Split(s, " ", true);  // Split command line into parts
			if (parts.GetCount() > 0) {
				ValueArray args;
				for (const String& part : parts) {
					args.Add(part);
				}

				if (!host.Command(*this, args)) {  // Let host handle command
					AddOutputLine("Unknown command: " + s);
				}
			}
		}
	}
	catch (Exc e) {
		AddOutputLine("ERROR: " + e);
	}
}

// Implementation to execute an AST node
String VfsShellConsole::ExecuteCmdNode(CmdNode* node) {
	if (node == nullptr) return String();
	
	switch (node->GetType()) {
		case CmdNodeType::COMMAND: {
			CommandNode* cmd = static_cast<CommandNode*>(node);
			if (cmd->args.GetCount() == 0) return String();
			
			ValueArray args;
			for (const String& arg : cmd->args) {
				args.Add(arg);
			}
			
			if (host.Command(*this, args)) {
				return host.GetOutput();
			} else {
				AddOutputLine("Unknown command: " + cmd->args[0]);
				return String();
			}
		}
		
		case CmdNodeType::PIPELINE: {
			PipelineNode* pipeline = static_cast<PipelineNode*>(node);
			if (pipeline->commands.GetCount() == 0) return String();

			// For now, a simple pipeline implementation without actual pipes
			// A real implementation would need to implement actual pipe mechanisms
			String result;
			
			// Execute the first command and capture its output and exit code
			String first_output = ExecuteCmdNode(pipeline->commands[0]);
			
			for (int i = 1; i < pipeline->commands.GetCount(); i++) {
				// In a real implementation, we would pass the output of the previous command 
				// as input to the current command using a pipe mechanism
				// For now, we'll just execute the command ignoring the input
				String current_output = ExecuteCmdNode(pipeline->commands[i]);
				// The current output becomes the result for the last command in the pipeline
				result = current_output;
			}
			
			// The exit code of the pipeline should be the exit code of the last command
			// For now, we'll keep the host's exit code as is after processing the last command
			
			// Return the output of the last command (or first if only one command)
			return pipeline->commands.GetCount() > 1 ? result : first_output;
		}
		
		case CmdNodeType::AND_IF: {
			AndIfNode* andNode = static_cast<AndIfNode*>(node);
			
			// Execute the left side
			String left_result = ExecuteCmdNode(andNode->left);
			
			// Check the exit code from the host
			int left_exit_code = host.GetLastExitCode();
			
			String right_result;
			if (left_exit_code == 0) {  // Command succeeded
				// Execute the right side only if left succeeded
				right_result = ExecuteCmdNode(andNode->right);
			} else {
				// If left failed, right side is not executed (standard && behavior)
				// Set exit code to the left side's exit code
				host.SetExitCode(left_exit_code);
			}
			
			return left_result + right_result;
		}
		
		case CmdNodeType::OR_IF: {
			OrIfNode* orNode = static_cast<OrIfNode*>(node);
			
			// Execute the left side
			String left_result = ExecuteCmdNode(orNode->left);
			
			// Check the exit code from the host
			int left_exit_code = host.GetLastExitCode();
			
			String right_result;
			if (left_exit_code != 0) {  // Command failed
				// Execute the right side only if left failed
				right_result = ExecuteCmdNode(orNode->right);
			} else {
				// If left succeeded, right side is not executed (standard || behavior)
				// Set exit code to the left side's exit code
				host.SetExitCode(left_exit_code);
			}
			
			return left_result + right_result;
		}
		
		case CmdNodeType::SEQUENCE: {
			SequenceNode* seq = static_cast<SequenceNode*>(node);
			String result;
			for (CmdNode* cmd : seq->commands) {
				result += ExecuteCmdNode(cmd);
			}
			return result;
		}
		
		default:
			AddOutputLine("Unsupported command node type");
			return String();
	}
}

// Execute commands from a file
bool VfsShellConsole::ExecuteFile(const String& filepath) {
	FileIn in(filepath);
	if (!in.IsOpen()) {
		AddOutputLine("Cannot open file: " + filepath);
		return false;
	}
	
	String content;
	int c;
	while((c = in.Get()) >= 0) {
		content.Cat((char)c);
	}
	
	Vector<String> lines = Split(content, "\n", false); // Don't omit empty lines
	
	for (const String& line : lines) {
		String trimmed_line = TrimBoth(line);
		if (trimmed_line.IsEmpty() || trimmed_line.StartsWith("#")) {
			continue; // Skip empty lines and comments
		}
		
		// Store the command in output temporarily for Execute() to process
		output = trimmed_line;
		Execute(); // Execute processes the command
	}
	
	return true;
}

void VfsShellConsole::PrintLineHeader() {
	String user = GetUserName();  // Get actual username
	if (user.IsEmpty()) user = "user";
	String host_name = "vfsshell";  // Simple host name
	String s = user + "@" + host_name + ":" + cwd + " $ ";
	line_header = s;
	AddOutput(s);
}



bool VfsShellConsole::SetCurrentDirectory(const String& path) {
	String normalizedPath = ConvertToInternalPath(path, cwd);
	
	// Check if this is a VFS path
	if (IsVfsPath(normalizedPath)) {
		// Convert to VfsPath for VFS operations
		VfsPath vfsPath;
		vfsPath.Set(normalizedPath);
		
		MountManager& mm = MountManager::System();
		if (!mm.DirectoryExists(vfsPath))
			return false;
		cwd = normalizedPath;
		return true;
	} else {
		// This is a system path, check if directory exists using the mount system
		VfsPath vfsPath;
		vfsPath.Set(normalizedPath);

		MountManager& mm = MountManager::System();
		if (!mm.DirectoryExists(vfsPath))
			return false;
		cwd = normalizedPath;
		return true;
	}
}

// Implementation of pwd command
void VfsShellConsole::CmdPwd() {
	AddOutputLine(GetCurrentDirectory());
}

// Implementation of cd command
void VfsShellConsole::CmdCd(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("cd: missing directory operand");
		return;
	}
	
	String targetPath = args[1];
	
	if (SetCurrentDirectory(targetPath)) {
		// Update prompt to reflect new directory
		ClearOutput(); // Clear any previous output
	} else {
		AddOutputLine("cd: no such file or directory: " + targetPath);
	}
}

// Implementation of ls command
void VfsShellConsole::CmdLs(const ValueArray& args) {
	String targetPath;
	if (args.GetCount() > 1) {
		targetPath = args[1];
	} else {
		targetPath = GetCurrentDirectory();
	}
	
	String resolvedPath = ConvertToInternalPath(targetPath, GetCurrentDirectory());

	// Check if this is a VFS path
	if (IsVfsPath(resolvedPath)) {
		// Convert to VfsPath for VFS operations
		VfsPath vfsPath;
		vfsPath.Set(resolvedPath);

		MountManager& mm = MountManager::System();
		Vector<Shared<VfsItem>> items;
		if (mm.GetFiles(vfsPath, items)) {
			for (const Shared<VfsItem>& item : items) {
				AddOutputLine(item->name);
			}
		} else {
			AddOutputLine("ls: cannot access '" + resolvedPath + "': " + mm.last_error);
		}
	} else {
		// This is a system path
		if (DirectoryExists(resolvedPath)) {
			FindFile ff;
			if (ff.Search(AppendFileName(resolvedPath, "*"))) {
				do {
					String name = ff.GetName();
					if (name == "." || name == "..") continue;
					AddOutputLine(name);
				} while (ff.Next());
			}
		} else {
			AddOutputLine("ls: cannot access '" + resolvedPath + "': No such file or directory");
		}
	}
}

// Implementation of tree command
void VfsShellConsole::CmdTree(const ValueArray& args) {
	String targetPath;
	if (args.GetCount() > 1) {
		targetPath = args[1];
	} else {
		targetPath = GetCurrentDirectory();
	}
	
	String resolvedPath = ConvertToInternalPath(targetPath, GetCurrentDirectory());

	// Check if this is a VFS path
	if (IsVfsPath(resolvedPath)) {
		// Convert to VfsPath for VFS operations
		VfsPath vfsPath;
		vfsPath.Set(resolvedPath);

		MountManager& mm = MountManager::System();
		Vector<Shared<VfsItem>> items;
		if (mm.GetFiles(vfsPath, items)) {
			AddOutputLine(resolvedPath + "/");
			for (const Shared<VfsItem>& item : items) {
				AddOutputLine("├── " + item->name);
			}
		} else {
			AddOutputLine("tree: cannot access '" + resolvedPath + "': " + mm.last_error);
		}
	} else {
		// This is a system path, check using MountManager
		VfsPath vfsPath;
		vfsPath.Set(resolvedPath);

		MountManager& mm = MountManager::System();
		Vector<Shared<VfsItem>> items;
		if (mm.GetFiles(vfsPath, items)) {
			AddOutputLine(resolvedPath + "/");
			for (const Shared<VfsItem>& item : items) {
				AddOutputLine("├── " + item->name);
			}
		} else {
			AddOutputLine("tree: cannot access '" + resolvedPath + "': " + mm.last_error);
		}
	}
}

// Implementation of mkdir command
void VfsShellConsole::CmdMkdir(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("mkdir: missing operand");
		return;
	}
	
	String targetPath = args[1];
	String resolvedPath = ConvertToInternalPath(targetPath, GetCurrentDirectory());

	// Check if this is a VFS path
	if (IsVfsPath(resolvedPath)) {
		// Convert to VfsPath for VFS operations
		VfsPath vfsPath;
		vfsPath.Set(resolvedPath);

		// Note: The current VFS implementation doesn't have a CreateDirectory method
		// This would require an actual implementation in the underlying VFS system
		AddOutputLine("mkdir: command not fully implemented in VFS - underlying VFS doesn't support directory creation yet");
	} else {
		// This is a system path - for now, use system calls directly as MountManager doesn't have CreateDirectory
		if (RealizeDirectory(resolvedPath)) {
			// Directory created successfully
		} else {
			AddOutputLine("mkdir: cannot create directory '" + resolvedPath + "': " + GetLastErrorMessage());
		}
	}
}

// Implementation of touch command
void VfsShellConsole::CmdTouch(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("touch: missing file operand");
		return;
	}
	
	String targetPath = args[1];
	String resolvedPath = ConvertToInternalPath(targetPath, GetCurrentDirectory());

	// Check if this is a VFS path
	if (IsVfsPath(resolvedPath)) {
		// Convert to VfsPath for VFS operations
		VfsPath vfsPath;
		vfsPath.Set(resolvedPath);

		MountManager& mm = MountManager::System();

		// Check if file exists
		if (!mm.FileExists(vfsPath) && !mm.DirectoryExists(vfsPath)) {
			// Note: The current VFS implementation doesn't support creating files directly
			// This would require an actual implementation in the underlying VFS system
			AddOutputLine("touch: command not fully implemented in VFS - underlying VFS doesn't support file creation yet");
		}
		// If it exists, we could potentially update timestamp, but that's not implemented yet
	} else {
		// This is a system path
		if (!FileExists(resolvedPath) && !DirectoryExists(resolvedPath)) {
			// Create the file if it doesn't exist
			FileOut out(resolvedPath);
			if (!out.IsOpen()) {
				AddOutputLine("touch: cannot touch '" + resolvedPath + "': " + GetLastErrorMessage());
				return;
			}
		} else {
			// Update timestamp if it exists (not directly supported in U++, so just confirm existence)
		}
	}
}

// Implementation of rm command
void VfsShellConsole::CmdRm(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("rm: missing file operand");
		return;
	}
	
	String targetPath = args[1];
	VfsPath path;
	if (targetPath.StartsWith("/")) {
		path.Set(targetPath);
	} else {
		VfsPath relPath;
		relPath.Set(targetPath);
		VfsPath cwdPath;
		cwdPath.Set(cwd);
		path = cwdPath / relPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(path) || mm.DirectoryExists(path)) {
		// Note: The current VFS implementation doesn't have DeleteFile/DeleteDirectory methods
		// This would require an actual implementation in the underlying VFS system
		AddOutputLine("rm: command not fully implemented - underlying VFS doesn't support file/directory deletion yet");
	} else {
		AddOutputLine("rm: cannot remove '" + targetPath + "': No such file or directory");
	}
}

// Implementation of mv command
void VfsShellConsole::CmdMv(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("mv: missing destination file operand after source");
		return;
	}
	
	String srcPathStr = args[1];
	String dstPathStr = args[2];
	
	VfsPath srcPath;
	if (srcPathStr.StartsWith("/")) {
		srcPath.Set(srcPathStr);
	} else {
		VfsPath srcRelPath;
		srcRelPath.Set(srcPathStr);
		VfsPath cwdPath; cwdPath.Set(cwd); srcPath = cwdPath / srcRelPath;
	}
	
	VfsPath dstPath;
	if (dstPathStr.StartsWith("/")) {
		dstPath.Set(dstPathStr);
	} else {
		VfsPath dstRelPath;
		dstRelPath.Set(dstPathStr);
		VfsPath cwdPath2; cwdPath2.Set(cwd); dstPath = cwdPath2 / dstRelPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(srcPath) || mm.DirectoryExists(srcPath)) {
		// Note: The current VFS implementation doesn't have a Rename method
		// This would require an actual implementation in the underlying VFS system
		AddOutputLine("mv: command not fully implemented - underlying VFS doesn't support file/directory renaming yet");
	} else {
		AddOutputLine("mv: cannot stat '" + srcPathStr + "': No such file or directory");
	}
}

// Implementation of link command
void VfsShellConsole::CmdLink(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("link: missing destination file operand after source");
		return;
	}
	
	String srcPathStr = args[1];
	String dstPathStr = args[2];
	
	VfsPath srcPath;
	if (srcPathStr.StartsWith("/")) {
		srcPath.Set(srcPathStr);
	} else {
		VfsPath srcRelPath;
		srcRelPath.Set(srcPathStr);
		VfsPath cwdPath3; cwdPath3.Set(cwd); srcPath = cwdPath3 / srcRelPath;
	}
	
	VfsPath dstPath;
	if (dstPathStr.StartsWith("/")) {
		dstPath.Set(dstPathStr);
	} else {
		VfsPath dstRelPath;
		dstRelPath.Set(dstPathStr);
		VfsPath cwdPath4; cwdPath4.Set(cwd); dstPath = cwdPath4 / dstRelPath;
	}
	
	// Note: The current VFS implementation doesn't have link functionality
	// This would require an actual implementation in the underlying VFS system
	AddOutputLine("link: command not fully implemented - underlying VFS doesn't support linking yet");
}

// Implementation of export command
void VfsShellConsole::CmdExport(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("export: missing destination path");
		return;
	}
	
	String vfsPathStr = args[1];
	String hostPathStr = args[2];
	
	VfsPath vfsPath;
	if (vfsPathStr.StartsWith("/")) {
		vfsPath.Set(vfsPathStr);
	} else {
		VfsPath relPath;
		relPath.Set(vfsPathStr);
		VfsPath cwdPath5; cwdPath5.Set(cwd); vfsPath = cwdPath5 / relPath;
	}
	
	// Note: The current VFS implementation doesn't have a MapPath method
	// This would require an actual implementation of a filesystem backend
	MountManager& mm = MountManager::System();
	// We cannot directly access VFS content in the current implementation
	AddOutputLine("export: command not fully implemented - underlying VFS doesn't support direct file export yet");
}

// Implementation of cat command
void VfsShellConsole::CmdCat(const ValueArray& args) {
	if (args.GetCount() < 2) {
		// If no file args, cat should typically read from stdin until EOF
		// For now, just a placeholder
		AddOutputLine("cat: usage: cat [path...]");
		return;
	}
	
	MountManager& mm = MountManager::System();
	
	for (int i = 1; i < args.GetCount(); i++) {
		String pathStr = args[i];
		VfsPath path;
		if (pathStr.StartsWith("/")) {
			path.Set(pathStr);
		} else {
			VfsPath relPath;
			relPath.Set(pathStr);
			VfsPath cwdPath6; cwdPath6.Set(cwd); path = cwdPath6 / relPath;
		}
		
		if (mm.FileExists(path)) {
			// Note: The current VFS implementation doesn't allow direct file reading
			// This would require an actual implementation of data access methods
			AddOutputLine("cat: command not fully implemented - underlying VFS doesn't support direct file content access yet");
		} else {
			AddOutputLine("cat: " + pathStr + ": No such file or directory");
		}
	}
}

// Implementation of grep command
void VfsShellConsole::CmdGrep(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("grep: usage: grep [-i] <pattern> [path]");
		return;
	}
	
	int argStart = 1;
	bool caseInsensitive = false;
	
	if (args[1] == "-i") {
		caseInsensitive = true;
		argStart = 2;
		if (args.GetCount() < 3) {
			AddOutputLine("grep: missing pattern");
			return;
		}
	}
	
	String pattern = args[argStart];
	String targetPath;
	if (args.GetCount() > argStart + 1) {
		targetPath = args[argStart + 1];
	} else {
		// Default to current directory if no path specified
		targetPath = (String)GetCurrentDirectory();
	}
	
	// Implementation would grep through file content
	AddOutputLine("grep: command not fully implemented yet for VFS paths");
}

// Implementation of rg command (ripgrep-like)
void VfsShellConsole::CmdRg(const ValueArray& args) {
	// For now, treat it similarly to grep
	if (args.GetCount() < 2) {
		AddOutputLine("rg: usage: rg [-i] <pattern> [path]");
		return;
	}
	
	AddOutputLine("rg: ripgrep functionality not fully implemented yet for VFS paths");
}

// Implementation of head command
void VfsShellConsole::CmdHead(const ValueArray& args) {
	int lineCount = 10; // Default
	int argStart = 1;
	
	if (args.GetCount() > 1 && args[1] == "-n") {
		if (args.GetCount() < 3) {
			AddOutputLine("head: option requires an argument -- 'n'");
			return;
		}
		lineCount = StrInt((String)args[2]);
		argStart = 3;
	}
	
	if (args.GetCount() <= argStart) {
		AddOutputLine("head: missing file operand");
		return;
	}
	
	String pathStr = args[argStart];
	VfsPath path;
	if (pathStr.StartsWith("/")) {
		path.Set(pathStr);
	} else {
		VfsPath relPath;
		relPath.Set(pathStr);
		VfsPath cwdPath7; cwdPath7.Set(cwd); path = cwdPath7 / relPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(path)) {
		// Note: The current VFS implementation doesn't allow direct file reading
		// This would require an actual implementation of data access methods
		AddOutputLine("head: command not fully implemented - underlying VFS doesn't support direct file content access yet");
	} else {
		AddOutputLine("head: " + pathStr + ": No such file or directory");
	}
}

// Implementation of tail command
void VfsShellConsole::CmdTail(const ValueArray& args) {
	int lineCount = 10; // Default
	int argStart = 1;
	
	if (args.GetCount() > 1 && args[1] == "-n") {
		if (args.GetCount() < 3) {
			AddOutputLine("tail: option requires an argument -- 'n'");
			return;
		}
		lineCount = StrInt((String)args[2]);
		argStart = 3;
	}
	
	if (args.GetCount() <= argStart) {
		AddOutputLine("tail: missing file operand");
		return;
	}
	
	String pathStr = args[argStart];
	VfsPath path;
	if (pathStr.StartsWith("/")) {
		path.Set(pathStr);
	} else {
		VfsPath relPath;
		relPath.Set(pathStr);
		VfsPath cwdPath8; cwdPath8.Set(cwd); path = cwdPath8 / relPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(path)) {
		// Note: The current VFS implementation doesn't allow direct file reading
		// This would require an actual implementation of data access methods
		AddOutputLine("tail: command not fully implemented - underlying VFS doesn't support direct file content access yet");
	} else {
		AddOutputLine("tail: " + pathStr + ": No such file or directory");
	}
}

// Implementation of uniq command
void VfsShellConsole::CmdUniq(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("uniq: missing input file");
		return;
	}
	
	String pathStr = args[1];
	VfsPath path;
	if (pathStr.StartsWith("/")) {
		path.Set(pathStr);
	} else {
		VfsPath relPath;
		relPath.Set(pathStr);
		VfsPath cwdPath9; cwdPath9.Set(cwd); path = cwdPath9 / relPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(path)) {
		// Note: The current VFS implementation doesn't allow direct file reading
		// This would require an actual implementation of data access methods
		AddOutputLine("uniq: command not fully implemented - underlying VFS doesn't support direct file content access yet");
	} else {
		AddOutputLine("uniq: " + pathStr + ": No such file or directory");
	}
}

// Implementation of count command
void VfsShellConsole::CmdCount(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("count: missing input file");
		return;
	}
	
	String pathStr = args[1];
	VfsPath path;
	if (pathStr.StartsWith("/")) {
		path.Set(pathStr);
	} else {
		VfsPath relPath;
		relPath.Set(pathStr);
		VfsPath cwdPath10; cwdPath10.Set(cwd); path = cwdPath10 / relPath;
	}
	
	MountManager& mm = MountManager::System();
	if (mm.FileExists(path)) {
		// Note: The current VFS implementation doesn't allow direct file reading
		// This would require an actual implementation of data access methods
		AddOutputLine("count: command not fully implemented - underlying VFS doesn't support direct file content access yet");
	} else {
		AddOutputLine("count: " + pathStr + ": No such file or directory");
	}
}

// Implementation of history command
void VfsShellConsole::CmdHistory(const ValueArray& args) {
	AddOutputLine("history: command history functionality not fully implemented yet");
}

// Implementation of random command
void VfsShellConsole::CmdRandom(const ValueArray& args) {
	int minVal = 0;
	int maxVal = 100;
	
	if (args.GetCount() > 1) {
		minVal = StrInt((String)args[1]);
		if (args.GetCount() > 2) {
			maxVal = StrInt((String)args[2]);
		} else {
			maxVal = minVal;
			minVal = 0;
		}
	}
	
	if (minVal > maxVal) {
		int temp = minVal;
		minVal = maxVal;
		maxVal = temp;
	}
	
	int result = minVal + (Random() % (maxVal - minVal + 1));
	AddOutputLine(IntStr(result));
}

// Implementation of true/false commands
void VfsShellConsole::CmdTrueFalse(const ValueArray& args) {
	if (args.GetCount() < 1) return;
	
	String cmd = args[0];
	if (cmd == "true") {
		// Set exit code to 0 (success)
		host.SetExitCode(0);
	} else if (cmd == "false") {
		// Set exit code to 1 (failure)
		host.SetExitCode(1);
	}
}

// Implementation of echo command
void VfsShellConsole::CmdEcho(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine(""); // Echo newline if no args
		return;
	}
	
	String result;
	for (int i = 1; i < args.GetCount(); i++) {
		if (i > 1) result.Cat(" ");
		result.Cat((String)args[i]);
	}
	AddOutputLine(result);
}

// Implementation of IDE build command
void VfsShellConsole::CmdIdeBuild(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: idebuild <package_name> [assembly] [options]");
		AddOutputLine("Example: idebuild MyApp CLANG -release");
		return;
	}
	
	// Build command line args to pass to the IDE's console handler
	Vector<String> ide_args;
	ide_args.Add("build");
	
	for (int i = 1; i < args.GetCount(); i++) {
		ide_args.Add((String)args[i]);
	}
	
	// Call the IDE's command handler
	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of IDE workspace command
void VfsShellConsole::CmdIdeWorkspace(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: ideworkspace <command> [options]");
		AddOutputLine("Commands: list, create, open, close, scan");
		return;
	}
	
	String cmd = (String)args[1];
	Vector<String> ide_args;
	
	if (cmd == "list") {
		ide_args.Add("workspace");
		ide_args.Add("list");
	} else if (cmd == "scan") {
		ide_args.Add("workspace");
		ide_args.Add("scan");
	} else {
		AddOutputLine("Unknown workspace command: " + cmd);
		AddOutputLine("Supported commands: list, scan");
		return;
	}
	
	// Call the IDE's command handler
	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of IDE package command
void VfsShellConsole::CmdIdePackage(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: idepkg <command> [options]");
		AddOutputLine("Commands: list, create, scan, info");
		return;
	}
	
	String cmd = (String)args[1];
	Vector<String> ide_args;
	
	if (cmd == "list") {
		ide_args.Add("pkg");
		ide_args.Add("list");
	} else if (cmd == "scan") {
		ide_args.Add("pkg");
		ide_args.Add("scan");
	} else if (cmd == "create") {
		if (args.GetCount() < 3) {
			AddOutputLine("Usage: idepkg create <package_name>");
			return;
		}
		ide_args.Add("pkg");
		ide_args.Add("create");
		ide_args.Add((String)args[2]);
	} else {
		AddOutputLine("Unknown package command: " + cmd);
		AddOutputLine("Supported commands: list, create, scan");
		return;
	}
	
	// Call the IDE's command handler
	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of IDE install command
void VfsShellConsole::CmdIdeInstall(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: ideinstall <package_name>");
		return;
	}
	
	Vector<String> ide_args;
	ide_args.Add("install");
	
	for (int i = 1; i < args.GetCount(); i++) {
		ide_args.Add((String)args[i]);
	}
	
	// Call the IDE's command handler
	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of IDE uninstall command
void VfsShellConsole::CmdIdeUninstall(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: ideuninstall <package_name>");
		return;
	}
	
	Vector<String> ide_args;
	ide_args.Add("uninstall");
	
	for (int i = 1; i < args.GetCount(); i++) {
		ide_args.Add((String)args[i]);
	}
	
	// Call the IDE's command handler
	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder load command
void VfsShellConsole::CmdUppBuilderLoad(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.builder.load <directory-path> [-H]");
		AddOutputLine("Load all .bm files from directory, -H treats path as OS filesystem path");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("load");

	String path = (String)args[1];
	bool isHostPath = false;
	
	// Check if -H flag is present
	for (int i = 2; i < args.GetCount(); i++) {
		String arg = (String)args[i];
		if (arg == "-H") {
			isHostPath = true;
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.builder.load <directory-path> [-H]");
			return;
		}
	}

	ide_args.Add(path);
	if (isHostPath) {
		ide_args.Add("--host");
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder add command
void VfsShellConsole::CmdUppBuilderAdd(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.builder.add <bm-file-path> [-H]");
		AddOutputLine("Load a single .bm build method file, -H treats path as OS filesystem path");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("add");

	String path = (String)args[1];
	bool isHostPath = false;
	
	for (int i = 2; i < args.GetCount(); i++) {
		String arg = (String)args[i];
		if (arg == "-H") {
			isHostPath = true;
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.builder.add <bm-file-path> [-H]");
			return;
		}
	}

	ide_args.Add(path);
	if (isHostPath) {
		ide_args.Add("--host");
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder list command
void VfsShellConsole::CmdUppBuilderList(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("list");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder active set command
void VfsShellConsole::CmdUppBuilderActiveSet(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.builder.active.set <builder-name>");
		AddOutputLine("Set the active build method");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("set");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder get command
void VfsShellConsole::CmdUppBuilderGet(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.builder.get <key>");
		AddOutputLine("Show a key from the active build method");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("get");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder set command
void VfsShellConsole::CmdUppBuilderSet(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("Usage: upp.builder.set <key> <value>");
		AddOutputLine("Update a key in the active build method");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("set");
	ide_args.Add((String)args[1]);
	ide_args.Add((String)args[2]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder dump command
void VfsShellConsole::CmdUppBuilderDump(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.builder.dump <builder-name>");
		AddOutputLine("Dump all keys for a build method");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("dump");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ builder active dump command
void VfsShellConsole::CmdUppBuilderActiveDump(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("buildmethod");
	ide_args.Add("dump");
	ide_args.Add("active");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ startup load command
void VfsShellConsole::CmdUppStartupLoad(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.startup.load <directory-path> [-H] [-v]");
		AddOutputLine("Load all .var files from directory, -H treats path as OS filesystem path, -v verbose output");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("startup");
	ide_args.Add("load");

	String path = (String)args[1];
	bool isHostPath = false;
	bool verbose = false;
	
	for (int i = 2; i < args.GetCount(); i++) {
		String arg = (String)args[i];
		if (arg == "-H") {
			isHostPath = true;
		} else if (arg == "-v") {
			verbose = true;
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.startup.load <directory-path> [-H] [-v]");
			return;
		}
	}

	ide_args.Add(path);
	if (isHostPath) ide_args.Add("--host");
	if (verbose) ide_args.Add("--verbose");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ startup list command
void VfsShellConsole::CmdUppStartupList(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("startup");
	ide_args.Add("list");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ startup open command
void VfsShellConsole::CmdUppStartupOpen(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.startup.open <name> [-v]");
		AddOutputLine("Load a named startup assembly, -v for verbose");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("startup");
	ide_args.Add("open");

	ide_args.Add((String)args[1]);
	
	for (int i = 2; i < args.GetCount(); i++) {
		String arg = (String)args[i];
		if (arg == "-v") {
			ide_args.Add("--verbose");
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.startup.open <name> [-v]");
			return;
		}
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly load command
void VfsShellConsole::CmdUppAsmLoad(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.asm.load <var-file-path> [-H]");
		AddOutputLine("Load U++ assembly file, -H treats path as OS filesystem path");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("load");

	String path = (String)args[1];
	bool isHostPath = false;
	
	for (int i = 2; i < args.GetCount(); i++) {
		String arg = (String)args[i];
		if (arg == "-H") {
			isHostPath = true;
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.asm.load <var-file-path> [-H]");
			return;
		}
	}

	ide_args.Add(path);
	if (isHostPath) ide_args.Add("--host");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly create command
void VfsShellConsole::CmdUppAsmCreate(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("Usage: upp.asm.create <name> <output-path>");
		AddOutputLine("Create new U++ assembly");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("create");
	ide_args.Add((String)args[1]);
	ide_args.Add((String)args[2]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly list command
void VfsShellConsole::CmdUppAsmList(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("list");

	if (args.GetCount() > 1) {
		String arg = (String)args[1];
		if (arg == "-v") {
			ide_args.Add("--verbose");
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.asm.list [-v]");
			return;
		}
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly scan command
void VfsShellConsole::CmdUppAsmScan(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.asm.scan <directory-path>");
		AddOutputLine("Scan directory for U++ packages with .upp files");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("scan");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly load.host command
void VfsShellConsole::CmdUppAsmLoadHost(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.asm.load.host <host-var-file>");
		AddOutputLine("Mount host dir and load .var file from OS filesystem");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("loadhost");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ assembly refresh command
void VfsShellConsole::CmdUppAsmRefresh(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("assembly");
	ide_args.Add("refresh");

	if (args.GetCount() > 1) {
		String arg = (String)args[1];
		if (arg == "-v") {
			ide_args.Add("--verbose");
		} else {
			AddOutputLine("Unknown flag: " + arg);
			AddOutputLine("Usage: upp.asm.refresh [-v]");
			return;
		}
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace open command
void VfsShellConsole::CmdUppWkspOpen(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.wksp.open <pkg-name> [-v]  OR  upp.wksp.open -p <path> [-v]");
		AddOutputLine("Open a package from the list as workspace OR from path, -v for verbose");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("open");

	if ((String)args[1] == "-p" && args.GetCount() >= 3) {
		ide_args.Add("--path");
		ide_args.Add((String)args[2]);
		
		if (args.GetCount() > 3 && (String)args[3] == "-v") {
			ide_args.Add("--verbose");
		} else if (args.GetCount() > 3) {
			AddOutputLine("Unknown flag: " + (String)args[3]);
			AddOutputLine("Usage: upp.wksp.open <pkg-name> [-v]  OR  upp.wksp.open -p <path> [-v]");
			return;
		}
	} else {
		ide_args.Add((String)args[1]);
		
		if (args.GetCount() > 2 && (String)args[2] == "-v") {
			ide_args.Add("--verbose");
		} else if (args.GetCount() > 2) {
			AddOutputLine("Unknown flag: " + (String)args[2]);
			AddOutputLine("Usage: upp.wksp.open <pkg-name> [-v]  OR  upp.wksp.open -p <path> [-v]");
			return;
		}
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace close command
void VfsShellConsole::CmdUppWkspClose(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("close");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace package list command
void VfsShellConsole::CmdUppWkspPkgList(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("pkglist");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace package set command
void VfsShellConsole::CmdUppWkspPkgSet(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.wksp.pkg.set <package-name>");
		AddOutputLine("Set active package in workspace");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("pkgset");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace file list command
void VfsShellConsole::CmdUppWkspFileList(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("filelist");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace file set command
void VfsShellConsole::CmdUppWkspFileSet(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: upp.wksp.file.set <file-path>");
		AddOutputLine("Set active file in editor");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("fileset");
	ide_args.Add((String)args[1]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ workspace build command
void VfsShellConsole::CmdUppWkspBuild(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("workspace");
	ide_args.Add("build");

	// Add any additional options from the command line
	for (int i = 1; i < args.GetCount(); i++) {
		ide_args.Add((String)args[i]);
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of U++ GUI command
void VfsShellConsole::CmdUppGui(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("gui");

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of parse.file command
void VfsShellConsole::CmdParseFile(const ValueArray& args) {
	if (args.GetCount() < 2) {
		AddOutputLine("Usage: parse.file <filepath> [vfs-target-path]");
		AddOutputLine("Parse C++ file with libclang");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("parse");
	ide_args.Add("file");
	ide_args.Add((String)args[1]);

	if (args.GetCount() > 2) {
		ide_args.Add((String)args[2]);
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of parse.dump command
void VfsShellConsole::CmdParseDump(const ValueArray& args) {
	Vector<String> ide_args;
	ide_args.Add("parse");
	ide_args.Add("dump");

	if (args.GetCount() > 1) {
		ide_args.Add((String)args[1]);
	}

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// Implementation of parse.generate command
void VfsShellConsole::CmdParseGenerate(const ValueArray& args) {
	if (args.GetCount() < 3) {
		AddOutputLine("Usage: parse.generate <ast-path> <output-path>");
		AddOutputLine("Generate C++ code from AST");
		return;
	}

	Vector<String> ide_args;
	ide_args.Add("parse");
	ide_args.Add("generate");
	ide_args.Add((String)args[1]);
	ide_args.Add((String)args[2]);

	if(!HandleConsoleIdeArgs(ide_args))
		AddOutputLine(GetConsoleIdeExperimentalNotice());
}

// History management
void VfsShellConsole::AddToHistory(const String& command) {
	if (!command.IsEmpty()) {
		history.Add(command);
		history_index = -1; // Reset to show current input, not history entry
	}
}

String VfsShellConsole::GetHistoryEntry(int index) const {
	if (index >= 0 && index < history.GetCount()) {
		return history[index];
	}
	return String();
}

// Implementation of edit command
void VfsShellConsole::CmdEdit(const ValueArray& args) {
	String vfsPath;
	bool hasFilename = args.GetCount() > 1;

	if (hasFilename) {
		vfsPath = ConvertToInternalPath((String)args[1], GetCurrentDirectory());
	} else {
		AddOutputLine("Usage: edit <file_path>");
		return;
	}

	// Check if file exists
	VfsPath path;
	path.Set(vfsPath);

	MountManager& mm = MountManager::System();
	String content = "";
	bool fileExists = false;
	
	if (mm.FileExists(path)) {
		// For now, we'll try to read the file using the system path
		// Since the original MapPath doesn't exist, we'll try a different approach
		// We use the path directly since it was converted earlier and likely represents a real file path
		FileIn in(vfsPath);
		if (in.IsOpen()) {
			content = LoadFile(vfsPath); // Using LoadFile which is a standard U++ function
			fileExists = true;
		} else {
			// If LoadFile fails, the file may not exist at the host level
			content = "";
			fileExists = false;
		}
	} else {
		// File doesn't exist, create empty content for new file
		content = "";
		fileExists = false;
	}

	// Run the ncurses editor
	UPP::NcursesEditor::RunEditor(vfsPath, content);
}

// Implementation of qwen command
void VfsShellConsole::CmdQwen(const ValueArray& args) {
#ifdef flagQWEN
	// Convert U++ ValueArray to std::vector<std::string> for QwenCmd::cmd_qwen
	std::vector<std::string> qwen_args;
	for (int i = 1; i < args.GetCount(); i++) {  // Start from 1 to skip command name
		String arg_string = AsString(args[i]);
		qwen_args.push_back(std::string(arg_string.Begin(), arg_string.GetLength()));  // Convert U++ String to std::string
	}
	
	// Get the root VFS from MountManager (assuming / is mounted with SystemFS)
	MountManager& mm = MountManager::System();
	MountManager::MountPoint* mp = mm.Find("/");  // Find root mount point
	if (mp && mp->vfs) {
		// Use the VFS instance from mount manager
		QwenCmd::cmd_qwen(qwen_args, *mp->vfs);
	} else {
		AddOutputLine("Error: Could not access VFS for qwen command");
	}
#else
	AddOutputLine("Notice: Qwen assistant module is not linked in this build.");
#endif
}

END_UPP_NAMESPACE
