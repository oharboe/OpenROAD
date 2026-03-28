// SPDX-License-Identifier: BSD-3-Clause
// MiniTcl - Expression evaluator (recursive descent parser)

#include "expr.h"
#include "eval.h"
#include "interp.h"
#include "parser.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace minitcl {

// Expression value: either int, double, or string
struct ExprVal {
    enum Type { INT, DOUBLE, STRING } type;
    long long ival = 0;
    double dval = 0.0;
    std::string sval;

    static ExprVal makeInt(long long v) { return {INT, v, 0.0, ""}; }
    static ExprVal makeDouble(double v) { return {DOUBLE, 0, v, ""}; }
    static ExprVal makeString(const std::string &s) { return {STRING, 0, 0.0, s}; }

    double asDouble() const {
        if (type == INT) return static_cast<double>(ival);
        if (type == DOUBLE) return dval;
        return strtod(sval.c_str(), nullptr);
    }
    long long asInt() const {
        if (type == INT) return ival;
        if (type == DOUBLE) return static_cast<long long>(dval);
        return strtoll(sval.c_str(), nullptr, 0);
    }
    bool asBool() const {
        if (type == INT) return ival != 0;
        if (type == DOUBLE) return dval != 0.0;
        return !sval.empty() && sval != "0" && sval != "false" && sval != "no";
    }
    std::string toString() const {
        if (type == INT) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%lld", ival);
            return buf;
        }
        if (type == DOUBLE) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%g", dval);
            return buf;
        }
        return sval;
    }
};

// Recursive descent expression parser
class ExprParser {
  public:
    ExprParser(Tcl_Interp *interp, const char *expr)
        : interp_(interp), p_(expr), code_(TCL_OK) {}

    ExprVal parse() {
        skipWS();
        ExprVal result = parseTernary();
        return result;
    }

    int code() const { return code_; }

  private:
    Tcl_Interp *interp_;
    const char *p_;
    int code_;

    void skipWS() {
        while (*p_ == ' ' || *p_ == '\t' || *p_ == '\n' || *p_ == '\r' ||
               (*p_ == '\\' && *(p_ + 1) == '\n')) {
            if (*p_ == '\\' && *(p_ + 1) == '\n') {
                p_ += 2;  // skip backslash-newline continuation
            } else {
                p_++;
            }
        }
    }

    void error(const std::string &msg) {
        if (code_ == TCL_OK) {
            code_ = TCL_ERROR;
            auto *impl = getImpl(interp_);
            impl->result = msg;
        }
    }

    ExprVal parseTernary() {
        ExprVal v = parseOr();
        skipWS();
        if (*p_ == '?') {
            p_++;
            ExprVal trueVal = parseTernary();
            skipWS();
            if (*p_ == ':') p_++;
            ExprVal falseVal = parseTernary();
            return v.asBool() ? trueVal : falseVal;
        }
        return v;
    }

    ExprVal parseOr() {
        ExprVal v = parseAnd();
        skipWS();
        while (*p_ == '|' && *(p_ + 1) == '|') {
            p_ += 2;
            ExprVal r = parseAnd();
            v = ExprVal::makeInt(v.asBool() || r.asBool());
            skipWS();
        }
        return v;
    }

    ExprVal parseAnd() {
        ExprVal v = parseEquality();
        skipWS();
        while (*p_ == '&' && *(p_ + 1) == '&') {
            p_ += 2;
            ExprVal r = parseEquality();
            v = ExprVal::makeInt(v.asBool() && r.asBool());
            skipWS();
        }
        return v;
    }

    ExprVal parseEquality() {
        ExprVal v = parseRelational();
        skipWS();
        while (true) {
            if (*p_ == '=' && *(p_ + 1) == '=') {
                p_ += 2;
                ExprVal r = parseRelational();
                if (v.type == ExprVal::STRING || r.type == ExprVal::STRING)
                    v = ExprVal::makeInt(v.toString() == r.toString());
                else
                    v = ExprVal::makeInt(v.asDouble() == r.asDouble());
            } else if (*p_ == '!' && *(p_ + 1) == '=') {
                p_ += 2;
                ExprVal r = parseRelational();
                if (v.type == ExprVal::STRING || r.type == ExprVal::STRING)
                    v = ExprVal::makeInt(v.toString() != r.toString());
                else
                    v = ExprVal::makeInt(v.asDouble() != r.asDouble());
            } else if (*p_ == 'e' && *(p_ + 1) == 'q' && !isalnum(*(p_ + 2))) {
                p_ += 2;
                ExprVal r = parseRelational();
                v = ExprVal::makeInt(v.toString() == r.toString());
            } else if (*p_ == 'n' && *(p_ + 1) == 'e' && !isalnum(*(p_ + 2))) {
                p_ += 2;
                ExprVal r = parseRelational();
                v = ExprVal::makeInt(v.toString() != r.toString());
            } else if (*p_ == 'i' && *(p_ + 1) == 'n' && !isalnum(*(p_ + 2))) {
                p_ += 2;
                ExprVal r = parseRelational();
                // Check if v is in the list r
                std::string needle = v.toString();
                auto list = minitcl::parseScript(r.toString().c_str());
                bool found = false;
                for (auto &cmd : list)
                    for (auto &w : cmd.words)
                        if (w.text == needle) found = true;
                v = ExprVal::makeInt(found ? 1 : 0);
            } else if (*p_ == 'n' && *(p_ + 1) == 'i' && !isalnum(*(p_ + 2))) {
                p_ += 2;
                ExprVal r = parseRelational();
                std::string needle = v.toString();
                auto list = minitcl::parseScript(r.toString().c_str());
                bool found = false;
                for (auto &cmd : list)
                    for (auto &w : cmd.words)
                        if (w.text == needle) found = true;
                v = ExprVal::makeInt(found ? 0 : 1);
            } else {
                break;
            }
            skipWS();
        }
        return v;
    }

    ExprVal parseRelational() {
        ExprVal v = parseAddSub();
        skipWS();
        while (true) {
            if (*p_ == '<' && *(p_ + 1) == '=') {
                p_ += 2;
                ExprVal r = parseAddSub();
                v = ExprVal::makeInt(v.asDouble() <= r.asDouble());
            } else if (*p_ == '>' && *(p_ + 1) == '=') {
                p_ += 2;
                ExprVal r = parseAddSub();
                v = ExprVal::makeInt(v.asDouble() >= r.asDouble());
            } else if (*p_ == '<') {
                p_++;
                ExprVal r = parseAddSub();
                v = ExprVal::makeInt(v.asDouble() < r.asDouble());
            } else if (*p_ == '>') {
                p_++;
                ExprVal r = parseAddSub();
                v = ExprVal::makeInt(v.asDouble() > r.asDouble());
            } else {
                break;
            }
            skipWS();
        }
        return v;
    }

    ExprVal parseAddSub() {
        ExprVal v = parseMulDiv();
        skipWS();
        while (*p_ == '+' || *p_ == '-') {
            char op = *p_++;
            ExprVal r = parseMulDiv();
            if (v.type == ExprVal::INT && r.type == ExprVal::INT) {
                v = ExprVal::makeInt(op == '+' ? v.ival + r.ival
                                               : v.ival - r.ival);
            } else {
                v = ExprVal::makeDouble(op == '+' ? v.asDouble() + r.asDouble()
                                                  : v.asDouble() - r.asDouble());
            }
            skipWS();
        }
        return v;
    }

    ExprVal parseMulDiv() {
        ExprVal v = parseUnary();
        skipWS();
        while (*p_ == '*' && *(p_ + 1) != '*' || *p_ == '/' || *p_ == '%') {
            char op = *p_++;
            ExprVal r = parseUnary();
            if (op == '*') {
                if (v.type == ExprVal::INT && r.type == ExprVal::INT)
                    v = ExprVal::makeInt(v.ival * r.ival);
                else
                    v = ExprVal::makeDouble(v.asDouble() * r.asDouble());
            } else if (op == '/') {
                if (v.type == ExprVal::INT && r.type == ExprVal::INT) {
                    if (r.ival == 0) { error("divide by zero"); return v; }
                    v = ExprVal::makeInt(v.ival / r.ival);
                } else {
                    double rd = r.asDouble();
                    if (rd == 0.0) { error("divide by zero"); return v; }
                    v = ExprVal::makeDouble(v.asDouble() / rd);
                }
            } else {
                if (r.asInt() == 0) { error("divide by zero"); return v; }
                v = ExprVal::makeInt(v.asInt() % r.asInt());
            }
            skipWS();
        }
        // Power operator **
        if (*p_ == '*' && *(p_ + 1) == '*') {
            p_ += 2;
            ExprVal r = parseUnary();
            if (v.type == ExprVal::INT && r.type == ExprVal::INT && r.ival >= 0) {
                long long result = 1;
                for (long long i = 0; i < r.ival; i++) result *= v.ival;
                v = ExprVal::makeInt(result);
            } else {
                v = ExprVal::makeDouble(pow(v.asDouble(), r.asDouble()));
            }
        }
        return v;
    }

    ExprVal parseUnary() {
        skipWS();
        if (*p_ == '-') {
            p_++;
            ExprVal v = parseUnary();
            if (v.type == ExprVal::INT) return ExprVal::makeInt(-v.ival);
            return ExprVal::makeDouble(-v.asDouble());
        }
        if (*p_ == '+') {
            p_++;
            return parseUnary();
        }
        if (*p_ == '!') {
            p_++;
            ExprVal v = parseUnary();
            return ExprVal::makeInt(!v.asBool());
        }
        if (*p_ == '~') {
            p_++;
            ExprVal v = parseUnary();
            return ExprVal::makeInt(~v.asInt());
        }
        return parsePrimary();
    }

    ExprVal parsePrimary() {
        skipWS();

        // Parenthesized expression
        if (*p_ == '(') {
            p_++;
            ExprVal v = parseTernary();
            skipWS();
            if (*p_ == ')') p_++;
            return v;
        }

        // String literal (double-quoted)
        if (*p_ == '"') {
            p_++;
            std::string s;
            while (*p_ && *p_ != '"') {
                if (*p_ == '\\' && *(p_ + 1)) { s += *p_++; s += *p_++; }
                else s += *p_++;
            }
            if (*p_ == '"') p_++;
            return ExprVal::makeString(backslashSubst(s));
        }

        // Braced string literal (no substitution)
        if (*p_ == '{') {
            p_++;
            std::string s;
            int depth = 1;
            while (*p_ && depth > 0) {
                if (*p_ == '{') depth++;
                else if (*p_ == '}') { depth--; if (depth == 0) break; }
                s += *p_++;
            }
            if (*p_ == '}') p_++;
            // Try to parse as number
            char *end;
            long long ival = strtoll(s.c_str(), &end, 0);
            if (end != s.c_str() && *end == '\0') return ExprVal::makeInt(ival);
            double dval = strtod(s.c_str(), &end);
            if (end != s.c_str() && *end == '\0') return ExprVal::makeDouble(dval);
            return ExprVal::makeString(s);
        }

        // Variable substitution
        if (*p_ == '$') {
            p_++;
            std::string varName;
            if (*p_ == '{') {
                p_++;
                while (*p_ && *p_ != '}') varName += *p_++;
                if (*p_ == '}') p_++;
            } else {
                while (*p_ && (isalnum(*p_) || *p_ == '_' || (*p_ == ':' && *(p_ + 1) == ':'))) {
                    if (*p_ == ':') { varName += "::"; p_ += 2; }
                    else varName += *p_++;
                }
            }
            const char *val = Tcl_GetVar(interp_, varName.c_str(), 0);
            if (!val) {
                error("can't read \"" + varName + "\": no such variable");
                return ExprVal::makeInt(0);
            }
            // Try to parse as number
            char *end;
            long long ival = strtoll(val, &end, 0);
            if (end != val && *end == '\0') return ExprVal::makeInt(ival);
            double dval = strtod(val, &end);
            if (end != val && *end == '\0') return ExprVal::makeDouble(dval);
            return ExprVal::makeString(val);
        }

        // Command substitution
        if (*p_ == '[') {
            p_++;
            int depth = 1;
            const char *start = p_;
            while (*p_ && depth > 0) {
                if (*p_ == '[') depth++;
                else if (*p_ == ']') depth--;
                if (depth > 0) p_++;
            }
            std::string cmd(start, p_ - start);
            if (*p_ == ']') p_++;
            int rc = Tcl_Eval(interp_, cmd.c_str());
            if (rc != TCL_OK) { code_ = rc; return ExprVal::makeInt(0); }
            const char *val = Tcl_GetStringResult(interp_);
            char *end;
            long long ival = strtoll(val, &end, 0);
            if (end != val && *end == '\0') return ExprVal::makeInt(ival);
            double dval = strtod(val, &end);
            if (end != val && *end == '\0') return ExprVal::makeDouble(dval);
            return ExprVal::makeString(val);
        }

        // Function call or named constant
        if (isalpha(*p_) || *p_ == '_') {
            std::string name;
            while (*p_ && (isalnum(*p_) || *p_ == '_')) name += *p_++;
            skipWS();

            // Named constants
            if (name == "true") return ExprVal::makeInt(1);
            if (name == "false") return ExprVal::makeInt(0);

            // Functions
            if (*p_ == '(') {
                p_++;
                ExprVal arg = parseTernary();
                // Check for second arg (min, max, pow)
                ExprVal arg2 = ExprVal::makeDouble(0);
                bool hasArg2 = false;
                skipWS();
                if (*p_ == ',') {
                    p_++;
                    arg2 = parseTernary();
                    hasArg2 = true;
                }
                skipWS();
                if (*p_ == ')') p_++;

                if (name == "int") return ExprVal::makeInt(arg.asInt());
                if (name == "wide") return ExprVal::makeInt(arg.asInt());
                if (name == "double") return ExprVal::makeDouble(arg.asDouble());
                if (name == "round") return ExprVal::makeInt(llround(arg.asDouble()));
                if (name == "abs") {
                    if (arg.type == ExprVal::INT)
                        return ExprVal::makeInt(arg.ival < 0 ? -arg.ival : arg.ival);
                    return ExprVal::makeDouble(fabs(arg.asDouble()));
                }
                if (name == "ceil") return ExprVal::makeDouble(ceil(arg.asDouble()));
                if (name == "floor") return ExprVal::makeDouble(floor(arg.asDouble()));
                if (name == "sqrt") return ExprVal::makeDouble(sqrt(arg.asDouble()));
                if (name == "exp") return ExprVal::makeDouble(exp(arg.asDouble()));
                if (name == "log") return ExprVal::makeDouble(log(arg.asDouble()));
                if (name == "log10") return ExprVal::makeDouble(log10(arg.asDouble()));
                if (name == "sin") return ExprVal::makeDouble(sin(arg.asDouble()));
                if (name == "cos") return ExprVal::makeDouble(cos(arg.asDouble()));
                if (name == "tan") return ExprVal::makeDouble(tan(arg.asDouble()));
                if (name == "pow" && hasArg2)
                    return ExprVal::makeDouble(pow(arg.asDouble(), arg2.asDouble()));
                if (name == "min" && hasArg2)
                    return ExprVal::makeDouble(
                        arg.asDouble() < arg2.asDouble() ? arg.asDouble() : arg2.asDouble());
                if (name == "max" && hasArg2)
                    return ExprVal::makeDouble(
                        arg.asDouble() > arg2.asDouble() ? arg.asDouble() : arg2.asDouble());

                error("unknown math function \"" + name + "\"");
                return ExprVal::makeInt(0);
            }

            error("unknown token \"" + name + "\" in expression");
            return ExprVal::makeInt(0);
        }

        // Number literal
        if (isdigit(*p_) || (*p_ == '.' && isdigit(*(p_ + 1)))) {
            const char *start = p_;
            bool isDouble = false;

            // Hex
            if (*p_ == '0' && (*(p_ + 1) == 'x' || *(p_ + 1) == 'X')) {
                p_ += 2;
                while (isxdigit(*p_)) p_++;
                return ExprVal::makeInt(strtoll(start, nullptr, 0));
            }
            // Octal
            if (*p_ == '0' && isdigit(*(p_ + 1))) {
                while (*p_ >= '0' && *p_ <= '7') p_++;
                if (*p_ == '.' || *p_ == 'e' || *p_ == 'E') {
                    isDouble = true;
                } else {
                    return ExprVal::makeInt(strtoll(start, nullptr, 0));
                }
            }

            while (isdigit(*p_)) p_++;
            if (*p_ == '.') { isDouble = true; p_++; while (isdigit(*p_)) p_++; }
            if (*p_ == 'e' || *p_ == 'E') {
                isDouble = true;
                p_++;
                if (*p_ == '+' || *p_ == '-') p_++;
                while (isdigit(*p_)) p_++;
            }

            if (isDouble) return ExprVal::makeDouble(strtod(start, nullptr));
            return ExprVal::makeInt(strtoll(start, nullptr, 10));
        }

        if (*p_) {
            error(std::string("unexpected character '") + *p_ + "' in expression");
            p_++;
        }
        return ExprVal::makeInt(0);
    }
};

static int exprCmd(ClientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const objv[]) {
    if (objc < 2) {
        auto *impl = getImpl(interp);
        impl->result = "wrong # args: should be \"expr arg ?arg ...?\"";
        return TCL_ERROR;
    }

    // Concatenate all arguments
    std::string expr;
    for (int i = 1; i < objc; i++) {
        if (i > 1) expr += ' ';
        expr += Tcl_GetString(objv[i]);
    }

    ExprParser parser(interp, expr.c_str());
    ExprVal result = parser.parse();
    if (parser.code() != TCL_OK) return parser.code();

    Tcl_SetObjResult(interp, Tcl_NewStringObj(result.toString().c_str(), -1));
    return TCL_OK;
}

void registerExprCommand(Tcl_Interp *interp) {
    Tcl_CreateObjCommand(interp, "expr", exprCmd, nullptr, nullptr);
}

}  // namespace minitcl
