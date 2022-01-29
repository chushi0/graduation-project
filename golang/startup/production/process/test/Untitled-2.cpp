#include "test.h"
#include <vector>

#ifdef LL_5577006791947779410_8674665223082153551
using namespace LL_5577006791947779410_8674665223082153551;
#endif

#ifdef LL_5577006791947779410_8674665223082153551
bool LL_5577006791947779410_8674665223082153551::Parse(IParser* parser)
#else
bool Parse(IParser* parser)
#endif
{
    std::vector<int> stack;
    stack.push_back(Nonterminal_S);
    while(stack.size() > 0) {
        int last = stack.back();
        stack.pop_back();
        int token = parser->Next();
        switch (last) {
            case Nonterminal_S:{
                switch(token) {
                    case Terminal_LeftParentheses: {
                        // S := T E0
                        parser->Infer(Production_S_T_E0);
                        stack.push_back(Nonterminal_E0);
                        stack.push_back(Nonterminal_T);
                        break;
                    }
                    default: {
                        int expected[] = {Terminal_LeftParentheses};
                        CompileError compileError = {
                            expected, sizeof(expected) / sizeof(int), token
                        };
                        parser->Panic(&compileError);
                        return false;
                    }
                }
                break;
            }
            default: {
                int expected[] = {last};
                CompileError compileError = {
                    expected, sizeof(expected) / sizeof(int), token
                };
                parser->Panic(&compileError);
                return false;
            }
        }
    }
    auto token = parser->Next();
    if (token == TerminalEOF) {
        return true;
    }
    int expected[] = {TerminalEOF};
    CompileError compileError = {
        expected, sizeof(expected) / sizeof(int), token
    };
    parser->Panic(&compileError);
    return false;
}