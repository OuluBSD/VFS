#include "ShellSyntax.h"

NAMESPACE_UPP

ShellSyntaxParser::ShellSyntaxParser(const String& input) : input(input), pos(0) {}

ShellSyntaxParser::~ShellSyntaxParser() {
    // No need to explicitly delete nodes here since they are managed elsewhere
}

void ShellSyntaxParser::SkipWhitespace() {
    while (pos < input.GetCount() && isspace(input[pos])) {
        pos++;
    }
}

bool ShellSyntaxParser::IsAtEnd() const {
    return pos >= input.GetCount();
}

char ShellSyntaxParser::CurrentChar() const {
    if (pos < input.GetCount()) {
        return input[pos];
    }
    return '\0';
}

char ShellSyntaxParser::Peek(int offset) const {
    int next_pos = pos + offset;
    if (next_pos < input.GetCount()) {
        return input[next_pos];
    }
    return '\0';
}

void ShellSyntaxParser::Advance() {
    if (pos < input.GetCount()) {
        pos++;
    }
}

String ShellSyntaxParser::ConsumeToken() {
    SkipWhitespace();
    if (IsAtEnd()) {
        return String();
    }
    
    String token;
    if (CurrentChar() == '|' && Peek() == '|') {
        // ||
        token = "||";
        Advance(); // move past first |
        Advance(); // move past second |
    } else if (CurrentChar() == '&' && Peek() == '&') {
        // &&
        token = "&&";
        Advance(); // move past first &
        Advance(); // move past second &
    } else if (CurrentChar() == '|') {
        // |
        token = "|";
        Advance();
    } else if (CurrentChar() == '&') {
        // &
        token = "&";
        Advance();
    } else if (CurrentChar() == ';') {
        // ;
        token = ";";
        Advance();
    } else if (CurrentChar() == '(') {
        token = "(";
        Advance();
    } else if (CurrentChar() == ')') {
        token = ")";
        Advance();
    } else {
        // Not a special token, consume a word
        return ParseWord();
    }
    
    SkipWhitespace();
    return token;
}

String ShellSyntaxParser::ParseWord() {
    SkipWhitespace();
    if (IsAtEnd()) {
        return String();
    }
    
    String word;
    bool in_quotes = false;
    char quote_char = '\0';
    
    while (pos < input.GetCount()) {
        char c = CurrentChar();
        
        // Handle quotes
        if ((c == '"' || c == '\'') && !in_quotes) {
            in_quotes = true;
            quote_char = c;
            Advance();
            continue;
        } else if (c == quote_char && in_quotes) {
            in_quotes = false;
            Advance();
            continue;
        }
        
        // If not in quotes, check for token separators
        if (!in_quotes && (isspace(c) || c == '|' || c == '&' || c == ';' || c == '(' || c == ')')) {
            break;
        }
        
        word.Cat(c);
        Advance();
    }
    
    return word;
}

CmdNode* ShellSyntaxParser::ParseSimpleCommand() {
    Vector<String> args;
    
    SkipWhitespace();
    if (IsAtEnd() || CurrentChar() == '|' || CurrentChar() == '&' || CurrentChar() == ';' || CurrentChar() == ')') {
        return nullptr;
    }
    
    // Parse command and its arguments
    while (!IsAtEnd()) {
        char c = CurrentChar();
        if (c == '|' || c == '&' || c == ';' || c == ')') {
            break;
        }
        
        String word = ParseWord();
        if (!word.IsEmpty()) {
            args.Add(word);
        } else {
            break;
        }
        
        SkipWhitespace();
    }
    
    if (args.GetCount() > 0) {
        CommandNode* cmd = new CommandNode();
        for (const String& arg : args) {
            cmd->args.Add(arg);
        }
        return cmd;
    }
    
    return nullptr;
}

CmdNode* ShellSyntaxParser::ParseCommand() {
    return ParseLogicalExpr();
}

CmdNode* ShellSyntaxParser::ParsePipeline() {
    CmdNode* left = ParseSimpleCommand();
    if (!left) return nullptr;
    
    SkipWhitespace();
    while (!IsAtEnd() && CurrentChar() == '|') {
        if (Peek() == '|') {
            // That's logical OR, not pipeline
            break;
        }
        
        // Consume the pipe character
        Advance();
        
        CmdNode* right = ParseSimpleCommand();
        if (!right) {
            // A pipe with no command after it is an error - but for our parser, we'll just return the left side
            return left;
        }
        
        // Create a pipeline node
        PipelineNode* pipeline = new PipelineNode();
        if (left->GetType() == CmdNodeType::PIPELINE) {
            // If left is already a pipeline, append to it
            PipelineNode* pl = static_cast<PipelineNode*>(left);
            pl->commands.Add(right);
            left = pl;
        } else {
            // Otherwise create a new pipeline
            pipeline->commands.Add(left);
            pipeline->commands.Add(right);
            left = pipeline;
        }
        
        SkipWhitespace();
    }
    
    return left;
}

CmdNode* ShellSyntaxParser::ParseLogicalExpr() {
    CmdNode* left = ParsePipeline();
    if (!left) return nullptr;
    
    SkipWhitespace();
    while (!IsAtEnd()) {
        if (CurrentChar() == '&' && Peek() == '&') {
            // && operator
            Advance(); // consume first &
            Advance(); // consume second &
            
            CmdNode* right = ParsePipeline();
            if (!right) {
                // Error case - right side of && is empty
                return left;
            }
            
            AndIfNode* andNode = new AndIfNode(left, right);
            left = andNode;
        } else if (CurrentChar() == '|' && Peek() == '|') {
            // || operator
            Advance(); // consume first |
            Advance(); // consume second |
            
            CmdNode* right = ParsePipeline();
            if (!right) {
                // Error case - right side of || is empty
                return left;
            }
            
            OrIfNode* orNode = new OrIfNode(left, right);
            left = orNode;
        } else {
            break;
        }
        
        SkipWhitespace();
    }
    
    return left;
}

CmdNode* ShellSyntaxParser::Parse() {
    if (IsAtEnd()) {
        return nullptr;
    }
    
    Vector<CmdNode*> commands;
    
    while (!IsAtEnd()) {
        CmdNode* cmd = ParseCommand();
        if (cmd) {
            commands.Add(cmd);
        }
        
        SkipWhitespace();
        if (IsAtEnd()) {
            break;
        }
        
        if (CurrentChar() == ';') {
            Advance(); // consume ;
        } else if (CurrentChar() == '&') {
            // Handle background execution if needed
            Advance(); // consume &
            // For now, we'll just treat it as a sequence
        }
        
        SkipWhitespace();
    }
    
    if (commands.GetCount() == 0) {
        return nullptr;
    } else if (commands.GetCount() == 1) {
        return commands[0];
    } else {
        // Multiple commands separated by ; or &
        SequenceNode* seq = new SequenceNode();
        for (CmdNode* cmd : commands) {
            seq->commands.Add(cmd);
        }
        return seq;
    }
}

CmdNode* ShellSyntaxParser::ParseString(const String& input) {
    ShellSyntaxParser parser(input);
    return parser.Parse();
}

END_UPP_NAMESPACE