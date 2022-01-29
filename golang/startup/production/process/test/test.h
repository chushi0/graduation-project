#pragma once
/**
 * Auto-generate header file.
 * After you re-generate file, ALL YOUR CHANGE WILL BE LOST!
 */

// By modify this macro, you can define automaton code in namespace
// #define LL_5577006791947779410_8674665223082153551

#ifdef LL_5577006791947779410_8674665223082153551
namespace LL_5577006791947779410_8674665223082153551 {
#endif

	// Production Definition Code
	// 
	// 	S := E
	// 	E := T E0
	// 	E0 := + T E0
	// 		|
	// 	T := F T0
	// 	T0 := * F T0
	// 		|
	// 	F := ( E )
	// 		| id
	// 	

	// Terminals Definition
	constexpr int TerminalEOF = -1;
	constexpr int Terminal_Plus = 0; // +
	constexpr int Terminal_Asterisk = 1; // *
	constexpr int Terminal_LeftParentheses = 2; // (
	constexpr int Terminal_RightParentheses = 3; // )
	constexpr int Terminal_id = 4; // id

	// Nonterminals Definition
	constexpr int Nonterminal_E = 5; // E
	constexpr int Nonterminal_E0 = 6; // E0
	constexpr int Nonterminal_T = 7; // T
	constexpr int Nonterminal_T0 = 8; // T0
	constexpr int Nonterminal_F = 9; // F
	constexpr int Nonterminal_S = 10; // S

	// Productions Definition
	constexpr int Production_F_LeftParentheses_E_RightParentheses = 0; // F := ( E )
	constexpr int Production_F_id = 1; // F := id
	constexpr int Production_S_T_E0 = 2; // S := T E0
	constexpr int Production_E_T_E0 = 3; // E := T E0
	constexpr int Production_E0_Plus_T_E0 = 4; // E0 := + T E0
	constexpr int Production_E0 = 5; // E0 :=
	constexpr int Production_T_LeftParentheses_E_RightParentheses_T0 = 6; // T := ( E ) T0
	constexpr int Production_T_id_T0 = 7; // T := id T0
	constexpr int Production_T0_Asterisk_F_T0 = 8; // T0 := * F T0
	constexpr int Production_T0 = 9; // T0 :=

	// Compile Error (will throw by parser function)
	struct CompileError {
		// an array contains terminal's id what parser expect
		int* Expected;
		// size of Expected
		int ExpectedSize;
		// parser read actually
		int Actual;
	};

	// Interface which can be easily defined by user
	class IParser {
	public:
		// Read a terminal from lexer. If no more terminals, return TerminalEOF(-1)
		virtual int Next() = 0;
		// Infer left nonterminal by production
		virtual void Infer(int id) = 0;
		// Error occurred when parse (parser will exit)
		virtual void Panic(CompileError* error) = 0;
	};

	// main entry parser
	// true means success, false means fail (after calling IParser.Panic)
	bool Parse(IParser* parser);

#ifdef LL_5577006791947779410_8674665223082153551
}
#endif
