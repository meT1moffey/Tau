#include<iostream>
#include<iomanip>

#include<cmath>
#include<chrono>

#include"modules\compiling.cpp"

#define MEASURE 0

using namespace chrono;

void execute(algorithm* algo, void*& result_buf, byte* args) {
	byte* data = (byte*)malloc(algo->mem_require); // Memory slice for variables
	byte* stack_data = (byte*)malloc(algo->stack_mem_require);
	void** stack = new void* [algo->stack_size]; // Stack for results
	size_t stack_c;
	for (int i = 0; i < algo->string_count; i++) {
		stack_c = 0;
		for (int j = 0; j < algo->string_sizes[i]; j++) {
			operand* op = &algo->strings[i][j];

			void* l, *r, *f, *buf;
			switch (op->type) {
			case 'f':
				// C++ binary (with 2 operands) function
				r = stack[--stack_c];
				l = stack[--stack_c];

				f = op->value;
				buf = stack_data + op->pos;
				((void(*)(void*&, void*, void*))f)(buf, l, r);
				stack[stack_c++] = buf;
				break;
			case 'u':
				// C++ unary function
				l = stack[--stack_c];
				f = op->value;
				buf = stack_data + op->pos;
				((void(*)(void*&, void*))f)(buf, l);
				stack[stack_c++] = buf;
				break;
			case 'g':
				// Global value (global variable, functions, hard coded values)
				stack[stack_c++] = op->value;
				break;
			case 'l':
				// Local variable
				stack[stack_c++] = data + (size_t)op->value;
				break;
			case 'r':
				// Variable for function result
				stack[stack_c++] = result_buf;
				break;
			case 'a':
				// Argument value
				stack[stack_c++] = args + (size_t)op->value;
				break;
			case 'j':
				// Jump action
				i = (uint64_t)op->value - 1;
				goto newExpression;
				break;
			case 'c':
				// Branching operator. If first byte of top element in stack is false, breaking current expression
				if (*(byte*)stack[--stack_c] == byte(0)) {
					goto newExpression;
				}
				break;
			case -1:
				// Printing all data in allocated memory
				cout << hex << setfill('0');
				for (byte* it = data + algo->mem_require - 1; it + 1 != data; it--)
					cout << setw(2) << +(unsigned char)*it << ' ';
				cout << '\n';
				break;
			}
		}
	newExpression:;
	}

	free(data);
	free(stack_data);
	//delete[] stack; It causes infinite loop and I have no idea why 
}

int main() {
	ifstream code("code.tau");
	vector<string> words = read_words(code);
	map<string, var> globals = parse_functions(words);

	void* nullbuf = malloc(0);
#if MEASURE
	time_point<system_clock> start = system_clock::now();
	int count = 1e4;
	for (int _ = 0; _ < count; _++)
#endif
		execute((algorithm*)globals["main"].pos, nullbuf, nullptr);
#if MEASURE
	time_point<system_clock> end = system_clock::now();
	cout << duration_cast<nanoseconds>(end - start).count() / (double)count << "ns\n";
#endif

	for (auto value : globals)
		delete value.second.pos;
}
