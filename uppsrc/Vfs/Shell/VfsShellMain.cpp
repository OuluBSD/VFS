#include "VfsShell.h"
#include <Core/Core.h>
#include <Vfs/Core/VfsBase.h>  // For VFS mounting functionality

#ifdef flagQWEN
#include <Qwen/CmdQwen.h>
#endif

#ifdef flagIDE
#include <ide/Core/ConsoleHeadless.h>  // For IDE command handling
#else
namespace Upp {
inline bool HandleConsoleIdeArgs(const Vector<String>& ide_args) { return false; }
inline const char* GetConsoleIdeExperimentalNotice() { return "TheIDE console integration is not available in standalone VfsShell."; }
}
#endif

#include <iostream>
#include <stdio.h>   // For standard I/O functions

#if defined(PLATFORM_POSIX)
#include <termios.h> // For terminal manipulation
#include <unistd.h>  // For read, STDIN_FILENO
#endif

// This translation unit is the standalone VfsShell console entry point. GUI
// consumers of the VfsShell library (Explorer) and tests bring their own main(),
// so the whole TU is compiled out unless building the standalone main.
#if !defined(flagGUI) && defined(flagMAIN)

using namespace std;

using namespace Upp;

// Initialize MountManager with system filesystem mounted at root
void InitializeMountSystem() {
    MountManager& mm = MountManager::System();

    // Mount system filesystem at root "/"
    mm.MountOwned("/", new SystemFS(), "sysfs");

    // This makes the system filesystem available at the root path
    // Additional VFS mounts can be added here for "/vfs/" path or other locations
}

namespace {

Vector<String> ToStringVector(const ValueArray& args, int start_index = 0)
{
	Vector<String> out;
	for(int i = start_index; i < args.GetCount(); i++)
		out.Add((String)args[i]);
	return out;
}

}

// Forward declarations for internal functions
Vector<String> getPathCompletion(const String& partialPath, VfsShellConsole& console);
#if defined(PLATFORM_POSIX)
String readLineWithHistory(VfsShellConsole& console);
#endif

// Function to perform path completion
Vector<String> getPathCompletion(const String& partialPath, VfsShellConsole& console) {
    Vector<String> completions;
    
    // Determine the directory to search in and the partial filename
    String dirPath = GetFileDirectory(partialPath);
    String fileNamePrefix = GetFileName(partialPath);
    
    // If no directory is specified, use the current directory
    if (dirPath.IsEmpty()) {
        dirPath = console.GetCurrentDirectory();
        fileNamePrefix = partialPath;
    } else {
        // Convert to internal path format
        dirPath = VfsShellConsole::ConvertToInternalPath(dirPath, console.GetCurrentDirectory());
    }
    
    // Try to list files in the directory
    VfsPath vfsDir;
    vfsDir.Set(dirPath);
    
    MountManager& mm = MountManager::System();
    Vector<Shared<VfsItem>> items;
    if (mm.GetFiles(vfsDir, items)) {
        for (const Shared<VfsItem>& item : items) {
            if (item->name.StartsWith(fileNamePrefix)) {
                String completion = AppendFileName(dirPath, item->name);
                completions.Add(completion);
            }
        }
    }
    
    return completions;
}

#if defined(PLATFORM_POSIX)
// Function to enable raw terminal mode for advanced input handling
void enableRawMode(struct termios *orig_termios) {
    struct termios raw = *orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(BRKINT | IXON | ICRNL);
    raw.c_cflag |= (CS8);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw terminal mode and restore original settings
void disableRawMode(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

// Function to read an input line with history and editing support
String readLineWithHistory(VfsShellConsole& console) {
    struct termios orig_termios;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        // Fallback to regular getline if terminal doesn't support raw mode
        std::string input;
        std::getline(std::cin, input);
        return String(input.c_str());
    }

    enableRawMode(&orig_termios);

    String line = "";
    int cursor_pos = 0;
    int history_pos = -1;
    
    // Save original line if we're starting to search through history
    String history_search_prefix = "";

    printf("%s%s", console.GetCurrentDirectory().ToStd().c_str(), " $ ");
    fflush(stdout);

    while (true) {
        char c;
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        
        if (nread <= 0) {
            // Error reading or EOF
            break;
        }

        if (c == 127 || c == '\b') {  // Backspace or delete
            if (cursor_pos > 0) {
                line.Remove(cursor_pos - 1, 1);
                cursor_pos--;
                
                // Redraw the line after backspace
                printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                // Reposition cursor
                int new_cursor_pos = console.GetCurrentDirectory().GetCount() + 3 + cursor_pos;
                printf("\033[%dG", new_cursor_pos);
                fflush(stdout);
            }
        }
        else if (c == '\033') {  // Escape sequence (e.g., arrow keys)
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') { // Up arrow - history
                        if (history_pos == -1) {
                            // First history access, save current line to search
                            history_search_prefix = line;
                            history_pos = console.GetHistorySize();
                        }
                        if (history_pos > 0) {
                            history_pos--;
                            // Find previous command that matches prefix
                            while (history_pos >= 0) {
                                String hist = console.GetHistoryEntry(history_pos);
                                if (history_search_prefix.IsEmpty() || hist.StartsWith(history_search_prefix)) {
                                    line = hist;
                                    cursor_pos = line.GetCount();
                                    break;
                                }
                                history_pos--;
                            }
                        }
                        
                        // Redraw the line
                        printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                        // Reposition cursor at the end
                        printf("\033[%luG", (long unsigned) (console.GetCurrentDirectory().GetCount() + 3 + cursor_pos + 1));
                        fflush(stdout);
                    }
                    else if (seq[1] == 'B') { // Down arrow - forward in history
                        if (history_pos < console.GetHistorySize() - 1) {
                            history_pos++;
                            // Find next command that matches prefix
                            bool found = false;
                            while (history_pos < console.GetHistorySize()) {
                                String hist = console.GetHistoryEntry(history_pos);
                                if (history_search_prefix.IsEmpty() || hist.StartsWith(history_search_prefix)) {
                                    line = hist;
                                    cursor_pos = line.GetCount();
                                    found = true;
                                    break;
                                }
                                history_pos++;
                            }
                            if (!found) {
                                // Go back to the current input line
                                line = history_search_prefix;
                                cursor_pos = line.GetCount();
                                history_pos = console.GetHistorySize(); // Signal that we're off history
                            }
                        } else {
                            // Go back to the current input line
                            line = history_search_prefix;
                            cursor_pos = line.GetCount();
                            history_pos = console.GetHistorySize(); // Signal that we're off history
                        }
                        
                        // Redraw the line
                        printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                        // Reposition cursor at the end
                        printf("\033[%luG", (long unsigned) (console.GetCurrentDirectory().GetCount() + 3 + cursor_pos + 1));
                        fflush(stdout);
                    }
                    else if (seq[1] == 'C') { // Right arrow - move cursor right
                        if (cursor_pos < line.GetCount()) {
                            cursor_pos++;
                            printf("\033[C");
                            fflush(stdout);
                        }
                    }
                    else if (seq[1] == 'D') { // Left arrow - move cursor left
                        if (cursor_pos > 0) {
                            cursor_pos--;
                            printf("\033[D");
                            fflush(stdout);
                        }
                    }
                }
            }
        }
        else if (c == '\n' || c == '\r') {  // Enter
            printf("\n");
            break;
        }
        else if (c == '\t') {  // Tab - path completion
            if (!line.IsEmpty()) {
                Vector<String> completions = getPathCompletion(line, console);
                if (completions.GetCount() == 1) {
                    // Single completion found
                    line = completions[0];
                    cursor_pos = line.GetCount();
                    // Redraw the line with the completed path
                    printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                    // Reposition cursor at the end
                    printf("\033[%luG", (long unsigned) (console.GetCurrentDirectory().GetCount() + 3 + cursor_pos + 1));
                    fflush(stdout);
                } else if (completions.GetCount() > 1) {
                    // Multiple completions found - show them
                    printf("\n");
                    for (const String& comp : completions) {
                        printf("  %s\n", comp.ToStd().c_str());
                    }
                    // Redraw the prompt and line
                    printf("%s%s", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                    fflush(stdout);
                    // Reposition cursor at the end
                    cursor_pos = line.GetCount();
                }
            }
        }
        else if (c == 11) {  // Ctrl+K - kill line after cursor
            if (cursor_pos < line.GetCount()) {
                line = line.Mid(0, cursor_pos);  // Keep only text up to cursor
                // Redraw the line after killing
                printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
                // Reposition cursor at the end
                printf("\033[%luG", (long unsigned) (console.GetCurrentDirectory().GetCount() + 3 + cursor_pos + 1));
                fflush(stdout);
            }
        }
        else if (c >= 32 && c <= 126) {  // Printable ASCII characters
            // Insert character at current cursor position
            line.Insert(cursor_pos, String((char)c, 1));
            cursor_pos++;
            
            // Redraw the line after character insertion
            printf("\r%s%s \033[K", console.GetCurrentDirectory().ToStd().c_str(), line.ToStd().c_str());
            // Reposition cursor after the inserted character
            printf("\033[%luG", (long unsigned) (console.GetCurrentDirectory().GetCount() + 3 + cursor_pos + 1));
            fflush(stdout);
        }
    }

    disableRawMode(&orig_termios);
    return line;
}
#endif

// Simple implementation of VfsShellHostBase for console application
class VfsShellHost : public VfsShellHostBase {
private:
	String output;
	int last_exit_code;
	
public:
	VfsShellHost() : last_exit_code(0) {} // Initialize to success
	
	bool Command(VfsShellConsole& shell, const ValueArray& args) override {
		if (args.GetCount() == 0) return false;

		String cmd = args[0];
		if (cmd == "help") {
			shell.CmdHelp(args);
			return true;
		}
		else if (cmd == "pwd") {
			shell.CmdPwd();
			return true;
		}
		else if (cmd == "cd") {
			shell.CmdCd(args);
			return true;
		}
		else if (cmd == "ls") {
			shell.CmdLs(args);
			return true;
		}
		else if (cmd == "tree") {
			shell.CmdTree(args);
			return true;
		}
		else if (cmd == "mkdir") {
			shell.CmdMkdir(args);
			return true;
		}
		else if (cmd == "touch") {
			shell.CmdTouch(args);
			return true;
		}
		else if (cmd == "rm") {
			shell.CmdRm(args);
			return true;
		}
		else if (cmd == "mv") {
			shell.CmdMv(args);
			return true;
		}
		else if (cmd == "link") {
			shell.CmdLink(args);
			return true;
		}
		else if (cmd == "export") {
			shell.CmdExport(args);
			return true;
		}
		else if (cmd == "cat") {
			shell.CmdCat(args);
			return true;
		}
		else if (cmd == "grep") {
			shell.CmdGrep(args);
			return true;
		}
		else if (cmd == "rg") {
			shell.CmdRg(args);
			return true;
		}
		else if (cmd == "head") {
			shell.CmdHead(args);
			return true;
		}
		else if (cmd == "tail") {
			shell.CmdTail(args);
			return true;
		}
		else if (cmd == "uniq") {
			shell.CmdUniq(args);
			return true;
		}
		else if (cmd == "count") {
			shell.CmdCount(args);
			return true;
		}
		else if (cmd == "history") {
			shell.CmdHistory(args);
			return true;
		}
		else if (cmd == "random") {
			shell.CmdRandom(args);
			return true;
		}
		else if (cmd == "true" || cmd == "false") {
			// For true/false, we need to handle exit status properly
			if (cmd == "true") {
				this->SetExitCode(0);  // Success
			} else { // cmd == "false"
				this->SetExitCode(1);  // Failure
			}
			shell.CmdTrueFalse(args);
			return true;
		}
		else if (cmd == "echo") {
			shell.CmdEcho(args);
			return true;
		}
		else if (cmd == "edit" || cmd == "ee") {
			shell.CmdEdit(args);
			return true;
		}
		else if (cmd == "idebuild") {
			shell.CmdIdeBuild(args);
			return true;
		}
		else if (cmd == "ideworkspace") {
			shell.CmdIdeWorkspace(args);
			return true;
		}
		else if (cmd == "idepkg") {
			shell.CmdIdePackage(args);
			return true;
		}
		else if (cmd == "ideinstall") {
			shell.CmdIdeInstall(args);
			return true;
		}
		else if (cmd == "ideuninstall") {
			shell.CmdIdeUninstall(args);
			return true;
		}
		else if (cmd == "theide") {
			Vector<String> ide_args = ToStringVector(args, 1);
			if(!HandleConsoleIdeArgs(ide_args))
				AddOutputLine(GetConsoleIdeExperimentalNotice());
			return true;
		}
		else if (cmd == "upp.builder.load") {
			shell.CmdUppBuilderLoad(args);
			return true;
		}
		else if (cmd == "upp.builder.add") {
			shell.CmdUppBuilderAdd(args);
			return true;
		}
		else if (cmd == "upp.builder.list") {
			shell.CmdUppBuilderList(args);
			return true;
		}
		else if (cmd == "upp.builder.active.set") {
			shell.CmdUppBuilderActiveSet(args);
			return true;
		}
		else if (cmd == "upp.builder.get") {
			shell.CmdUppBuilderGet(args);
			return true;
		}
		else if (cmd == "upp.builder.set") {
			shell.CmdUppBuilderSet(args);
			return true;
		}
		else if (cmd == "upp.builder.dump") {
			shell.CmdUppBuilderDump(args);
			return true;
		}
		else if (cmd == "upp.builder.active.dump") {
			shell.CmdUppBuilderActiveDump(args);
			return true;
		}
		else if (cmd == "upp.startup.load") {
			shell.CmdUppStartupLoad(args);
			return true;
		}
		else if (cmd == "upp.startup.list") {
			shell.CmdUppStartupList(args);
			return true;
		}
		else if (cmd == "upp.startup.open") {
			shell.CmdUppStartupOpen(args);
			return true;
		}
		else if (cmd == "upp.asm.load") {
			shell.CmdUppAsmLoad(args);
			return true;
		}
		else if (cmd == "upp.asm.create") {
			shell.CmdUppAsmCreate(args);
			return true;
		}
		else if (cmd == "upp.asm.list") {
			shell.CmdUppAsmList(args);
			return true;
		}
		else if (cmd == "upp.asm.scan") {
			shell.CmdUppAsmScan(args);
			return true;
		}
		else if (cmd == "upp.asm.load.host") {
			shell.CmdUppAsmLoadHost(args);
			return true;
		}
		else if (cmd == "upp.asm.refresh") {
			shell.CmdUppAsmRefresh(args);
			return true;
		}
		else if (cmd == "upp.wksp.open") {
			shell.CmdUppWkspOpen(args);
			return true;
		}
		else if (cmd == "upp.wksp.close") {
			shell.CmdUppWkspClose(args);
			return true;
		}
		else if (cmd == "upp.wksp.pkg.list") {
			shell.CmdUppWkspPkgList(args);
			return true;
		}
		else if (cmd == "upp.wksp.pkg.set") {
			shell.CmdUppWkspPkgSet(args);
			return true;
		}
		else if (cmd == "upp.wksp.file.list") {
			shell.CmdUppWkspFileList(args);
			return true;
		}
		else if (cmd == "upp.wksp.file.set") {
			shell.CmdUppWkspFileSet(args);
			return true;
		}
		else if (cmd == "upp.wksp.build") {
			shell.CmdUppWkspBuild(args);
			return true;
		}
		else if (cmd == "upp.gui") {
			shell.CmdUppGui(args);
			return true;
		}
		else if (cmd == "parse.file") {
			shell.CmdParseFile(args);
			return true;
		}
		else if (cmd == "parse.dump") {
			shell.CmdParseDump(args);
			return true;
		}
		else if (cmd == "parse.generate") {
			shell.CmdParseGenerate(args);
			return true;
		}
		else if (cmd == "qwen") {
			shell.CmdQwen(args);
			return true;
		}

		return false; // Command not handled
	}

	String GetOutput() const override {
		return output;
	}

	void ClearOutput() override {
		output.Clear();
	}
	
	int GetLastExitCode() const override {
		return last_exit_code;
	}
	
	void SetExitCode(int code) override {
		last_exit_code = code;
	}

	void AddOutput(const String& s) override {
		output << s;
	}

	void AddOutputLine(const String& s) override {
		output << s << "\n";
	}
};

int main(int argc, char* argv[])
{
	using namespace Upp;

	// Initialize the mount system before creating VfsShell
	InitializeMountSystem();

	if(argc > 1 && String(argv[1]).IsEqual("theide")) {
		Vector<String> ide_args;
		for(int i = 2; i < argc; i++)
			ide_args.Add(String(argv[i]));
		if(!HandleConsoleIdeArgs(ide_args))
			Cout() << GetConsoleIdeExperimentalNotice() << "\n";
		return 0;
	}

	// Check for help option
	for (int i = 1; i < argc; i++) {
		String arg = String(argv[i]);
		if (arg == "-h" || arg == "--help") {
			Cout() << "VFS Shell - A virtual file system shell supporting csh/tcsh syntax\n";
			Cout() << "Usage: VfsShell [options] [command]\n";
			Cout() << "Options:\n";
			Cout() << "  -h, --help     Show this help message\n";
			Cout() << "  -l, --login    Start as a login shell (read init files)\n";
			Cout() << "Commands:\n";
			Cout() << "  Supports standard commands like ls, cd, pwd, etc.\n";
			Cout() << "  Supports csh syntax: pipes (a | b), logical AND (a && b), logical OR (a || b)\n";
			Cout() << "  Example: ls | grep \"txt\" && echo \"Found text files\" || echo \"No text files\"\n";
			return 0;
		}
	}

	VfsShellHost host;
	VfsShellConsole console(host);

	auto flush_host_output = [&]() {
		String shell_output = host.GetOutput();
		if(shell_output.IsEmpty())
			return;
		Cout() << shell_output;
		Cout().Flush();
		host.ClearOutput();
	};

	auto print_prompt = [&]() {
		console.PrintLineHeader();
		flush_host_output();
	};

	auto run_interactive_shell = [&]() {
		Cout() << "VFS Shell - Type 'help' for available commands or 'quit' to exit\n";
		Cout() << "Current directory: " << (String)console.GetCurrentDirectory() << "\n";
		print_prompt();

		for (;;) {
			std::string temp_input;
			if (!std::getline(std::cin, temp_input)) {
				Cout() << "\n";
				break;
			}
			String input = String(temp_input.c_str());
			if (input == "quit" || input == "exit")
				break;

			// Set the input and execute
			console.output = input;
			console.Execute();
			flush_host_output();
			print_prompt();
		}
	};

	// Check for login shell flag to load initialization files
	bool is_login_shell = false;
	for (int i = 1; i < argc; i++) {
		String arg = String(argv[i]);
		if (arg == "-l" || arg == "--login") {
			is_login_shell = true;
			break;
		}
	}

	// Always set current directory to the current working directory (not the executable's directory)
	String current_dir = GetCurrentDirectory();
	if (current_dir.IsEmpty()) {
		// Fallback to executable directory if current directory cannot be determined
		current_dir = GetFileDirectory(GetExeFilePath());
	}
	console.SetCurrentDirectory(current_dir);

	// Load initialization files if this is a login shell or if no command line args
	if (is_login_shell || argc == 1) {
		// Try to load ~/.vfshrc first, then ~/.cshrc as fallback
		String home_dir = GetEnv("HOME");
		if (!home_dir.IsEmpty()) {
			String vfshrc_path = AppendFileName(home_dir, ".vfshrc");
			if (FileExists(vfshrc_path)) {
				Cout() << "Loading initialization file: " << vfshrc_path << "\n";
				// Execute the initialization file
				console.ExecuteFile(vfshrc_path);
				flush_host_output();
			} else {
				String cshrc_path = AppendFileName(home_dir, ".cshrc");
				if (FileExists(cshrc_path)) {
					Cout() << "Loading initialization file: " << cshrc_path << "\n";
					// Execute the initialization file
					console.ExecuteFile(cshrc_path);
					flush_host_output();
				}
			}
		}
	}

	// For console application, just process commands from standard input or arguments
	if (argc > 1) {
		// Check if this is just running a single command and not the interactive shell
		bool is_single_command = true;
		for (int i = 1; i < argc; i++) {
			String arg = String(argv[i]);
			if (arg == "-l" || arg == "--login") {
				is_single_command = false;  // If login flag is present, might be interactive
			}
		}
		
		if (is_single_command) {
			// Process command line arguments as a command
			String full_command;
			for (int i = 1; i < argc; i++) {
				String arg = String(argv[i]);
				// Skip flags like -h, -l
				if (arg != "-h" && arg != "--help" && arg != "-l" && arg != "--login") {
					if (!full_command.IsEmpty()) full_command << " ";
					full_command << arg;
				}
			}
			
			if (!full_command.IsEmpty()) {
				// Store the command in the console's output field and execute
				console.output = full_command;
				console.Execute();
				flush_host_output();
			}
		} else {
			run_interactive_shell();
		}
	} else {
		// Interactive shell mode with init file loading
		run_interactive_shell();
	}
	return 0;
}

#endif // !flagGUI && flagMAIN
