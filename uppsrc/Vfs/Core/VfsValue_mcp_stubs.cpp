#include "Core.h"

NAMESPACE_UPP
// No-op Env API implementations to allow linking until real logic is added.

EnvNodeInfo EnvLocate(const String& file, int line, int column) {
    EnvNodeInfo n;
    n.id = String().Cat() << file << ":" << line << ":" << column;
    n.kind = "unknown";
    n.name = GetFileName(file);
    n.file = file;
    n.start_line = line;
    n.start_col = column;
    n.end_line = line;
    n.end_col = column + 1;
    return n;
}

EnvNodeInfo EnvGet(const String& id) {
    EnvNodeInfo n;
    n.id = id;
    Vector<String> p = Split(id, ':');
    if(p.GetCount() >= 3) {
        n.file = p[0];
        n.start_line = StrInt(p[1]);
        n.start_col = StrInt(p[2]);
        n.end_line = n.start_line;
        n.end_col = n.start_col + 1;
        n.kind = "unknown";
        n.name = GetFileName(n.file);
    }
    return n;
}

Vector<EnvNodeInfo> EnvDefinition(const String& id) {
    Vector<EnvNodeInfo> v;
    v.Add(EnvGet(id));
    return v;
}

EnvRefPage EnvReferences(const String& id, const String& page_token, int limit) {
    EnvRefPage p; (void)id; (void)page_token; (void)limit; return p;
}

String EnvCodeById(const String& id) {
    EnvNodeInfo n = EnvGet(id);
    return String();
}

String EnvCodeByRange(const String& file, int sline, int scol, int eline, int ecol) {
    (void)file; (void)sline; (void)scol; (void)eline; (void)ecol; return String();
}

EnvStatusInfo EnvStatus() {
    // Report not initialized by default; MCP will fallback
    EnvStatusInfo st; st.initialized = false; return st;
}

END_UPP_NAMESPACE
