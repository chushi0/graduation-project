#include <iostream>

// include your header file

using namespace std;

class Parser : public IParser{
public:
	Parser(int* array, int size) :index(0), size(size), array(array) {
	}

	int Next() {
		if (index >= size) {
			return TerminalEOF;
		}
		return array[index++];
	}
	
	void Infer(int id) {
		cout << "Infer: " << id << endl;
	}
	
	void Shift() {
		cout << "Shift" << endl;
	}
	
	void Reduce(int id) {
		cout << "Reduce: " << id << endl;
	}
	
	bool Panic(ParserContext *ctx, CompileError* error) {
		cout << "Panic" << endl;
		cout << "  Expected: ";
		for (int i = 0; i < error->ExpectedSize; i++) {
			cout << error->Expected[i] << " ";
		}
		cout << "\n  Actual: " << error->Actual << endl;
		*ctx->token = Next();
		return *ctx->token != TerminalEOF;
	}
	
private:
	int index;
	int size;
	int* array;
};

int main() {
	int array[] = {
		// define your program
		// assume that this array is returned by your lexical analyzer
	};
	Parser p(array, sizeof(array) / sizeof(array[0]));
	bool res = Parse(&p);
	cout << "Result: " << (res ? "true" : "false") << endl;
	return 0;
}