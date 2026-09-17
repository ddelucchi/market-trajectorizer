// Self-contained JSON value reader.  Implements just enough of the spec to
// support engine-config files (top-level object of scalar key/value pairs,
// optional nested objects, strings, numbers, true/false/null) without pulling
// any third-party dependency.  Keys are looked up by exact string match in
// the *flat* top-level object scope; nested keys can be addressed with
// dotted paths ("section.N_phi").

#include "mt/api/json_io.hpp"
#include "mt/core/errors.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace mt::json_io {

std::string read_text(std::string_view path) {
    std::ifstream f((std::string(path)));
    if (!f) throw DataError("read_text: could not open " + std::string(path));
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

void write_text(std::string_view path, std::string_view text) {
    std::ofstream f((std::string(path)));
    if (!f) throw DataError("write_text: could not open " + std::string(path));
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
}

namespace {

struct Cursor {
    const char* p;
    const char* end;
    void skip_ws() {
        while (p < end) {
            char c = *p;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') { ++p; }
            else break;
        }
    }
    bool peek(char c) { skip_ws(); return p < end && *p == c; }
    bool eat(char c)  { skip_ws(); if (p < end && *p == c) { ++p; return true; } return false; }
};

// Read a quoted JSON string.  Supports \\, \", \n, \t escapes (sufficient for keys).
bool read_string(Cursor& cur, std::string& out) {
    cur.skip_ws();
    if (cur.p >= cur.end || *cur.p != '"') return false;
    ++cur.p;
    out.clear();
    while (cur.p < cur.end) {
        char c = *cur.p++;
        if (c == '"') return true;
        if (c == '\\' && cur.p < cur.end) {
            char e = *cur.p++;
            switch (e) {
                case '"':  out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'n':  out.push_back('\n'); break;
                case 't':  out.push_back('\t'); break;
                case 'r':  out.push_back('\r'); break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                default:   out.push_back(e);    break;
            }
        } else {
            out.push_back(c);
        }
    }
    return false;
}

// Skip a single JSON value (object/array/scalar), tracking nesting.
void skip_value(Cursor& cur) {
    cur.skip_ws();
    if (cur.p >= cur.end) return;
    char c = *cur.p;
    if (c == '"') { std::string tmp; read_string(cur, tmp); return; }
    if (c == '{' || c == '[') {
        char open = c, close = (c == '{') ? '}' : ']';
        int depth = 0; bool in_str = false;
        while (cur.p < cur.end) {
            char ch = *cur.p++;
            if (in_str) {
                if (ch == '\\' && cur.p < cur.end) { ++cur.p; continue; }
                if (ch == '"') in_str = false;
            } else {
                if (ch == '"') in_str = true;
                else if (ch == open)  ++depth;
                else if (ch == close) { --depth; if (depth == 0) return; }
            }
        }
        return;
    }
    // scalar (number / true / false / null)
    while (cur.p < cur.end && std::strchr(",}]\n\r\t ", *cur.p) == nullptr) ++cur.p;
}

// Find the value text for a (possibly dotted) key path.  Returns true on
// success and sets [v_begin, v_end) to the raw value slice (whitespace
// trimmed at the front).
bool find_key(std::string_view json_text, std::string_view key,
              const char*& v_begin, const char*& v_end)
{
    // Split key by '.' to support shallow nesting ("section.N_phi").
    Vec<std::string> parts;
    {
        std::string s(key); std::string cur;
        for (char c : s) { if (c == '.') { parts.push_back(cur); cur.clear(); } else cur.push_back(c); }
        parts.push_back(cur);
    }

    Cursor cur{ json_text.data(), json_text.data() + json_text.size() };
    for (usize lvl = 0; lvl < parts.size(); ++lvl) {
        if (!cur.eat('{')) return false;
        bool found = false;
        while (!cur.peek('}')) {
            std::string k;
            if (!read_string(cur, k)) return false;
            cur.skip_ws();
            if (!cur.eat(':')) return false;
            cur.skip_ws();
            if (k == parts[lvl]) {
                if (lvl + 1 == parts.size()) {
                    v_begin = cur.p;
                    skip_value(cur);
                    v_end = cur.p;
                    return true;
                }
                // descend into nested object
                found = true;
                break;
            } else {
                skip_value(cur);
                cur.skip_ws();
                if (!cur.peek('}')) cur.eat(',');
            }
        }
        if (!found) return false;
    }
    return false;
}

}  // namespace

real read_real(std::string_view json_text, std::string_view key, real def) {
    const char* b = nullptr; const char* e = nullptr;
    if (!find_key(json_text, key, b, e)) return def;
    while (b < e && (*b == ' ' || *b == '\t' || *b == '\n')) ++b;
    if (b >= e) return def;
    std::string s(b, static_cast<usize>(e - b));
    try { return std::stod(s); } catch (...) { return def; }
}

int read_int(std::string_view json_text, std::string_view key, int def) {
    const char* b = nullptr; const char* e = nullptr;
    if (!find_key(json_text, key, b, e)) return def;
    while (b < e && (*b == ' ' || *b == '\t' || *b == '\n')) ++b;
    if (b >= e) return def;
    std::string s(b, static_cast<usize>(e - b));
    try { return std::stoi(s); } catch (...) { return def; }
}

bool read_bool(std::string_view json_text, std::string_view key, bool def) {
    const char* b = nullptr; const char* e = nullptr;
    if (!find_key(json_text, key, b, e)) return def;
    while (b < e && (*b == ' ' || *b == '\t' || *b == '\n')) ++b;
    std::string s(b, static_cast<usize>(e - b));
    auto starts = [&](const char* w) { return s.rfind(w, 0) == 0; };
    if (starts("true"))  return true;
    if (starts("false")) return false;
    if (starts("1"))     return true;
    if (starts("0"))     return false;
    return def;
}

}  // namespace mt::json_io
