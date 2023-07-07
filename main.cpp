#include<iostream>
#include<fstream>

#include<string>
#include<vector>
#include<map>

#include<chrono>

using namespace std;

vector<string> read_words(istream& input) {
	vector<string> words;
	char state = '\0';
	string word;

	for (char c = '\n'; !input.eof(); input.read(&c, 1)) {
		switch (state) {
		case '\0':
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
				state = 'a';
			}
			else switch (c) {
			case '+':
			case '=':
			case ';':
				words.push_back(string(1, c));
				break;
			}
			break;
		case 'a':
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
			}
			else {
				words.push_back(word);
				word.clear();
				state = '\0';
				switch (c) {
				case '+':
				case '=':
				case ';':
					words.push_back(string(1, c));
					break;
				}
			}
			break;
		}
	}

	switch (state) {
	case 'a':
		words.push_back(word);
		break;
	}

	return words;
}

struct operand {
	void* value = nullptr;
	void(*func)(void*&, void*, void*) = nullptr;
	char type = 'r';

	operand() {}
	operand(void* value, char type) : value(value), type(type) {}
	operand(void(*func)(void*&, void*, void*), char type) : func(func), type(type), value(malloc(4)) {}
};

struct algoritm {
	operand** strings;
	size_t* string_sizes, string_count;
	size_t mem_require, stack_size;

	algoritm(operand** strings, size_t* string_sizes, size_t string_count, size_t mem_require, size_t stack_size)
		: strings(strings),
		string_sizes(string_sizes),
		string_count(string_count),
		mem_require(mem_require),
		stack_size(stack_size) {}
};

void int_sum(void*& buf, void* l, void* r) { *(int*)buf = *(int*)l + *(int*)r; }
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, 4); }

template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

algoritm compile(istream& input) {
	vector<string> words = read_words(input);
	map<string, int> vars;
	size_t mem_require = 0, max_stack = 0, cur_stack = 0;
	vector<operand*> algo;
	vector<size_t> str_sizes;
	vector<operand> str;

	char state = 'b';
	for (string word : words) {
		switch (state) {
		case 'b':
			if (word == "vars") {
				state = 'v';
				break;
			}
		case '\0':
			if (vars.find(word) != vars.end()) {
				str.push_back(operand((void*)vars[word], 'l'));
				cur_stack++;
			}
			else if (isdigit(word[0])) {
				str.push_back(operand(new int(stoi(word)), 'r'));
				cur_stack++;
			}
			else switch (word[0]) {
			case '=':
				str.push_back(operand(&set, 'f'));
				break;
			case '+':
				str.push_back(operand(&int_sum, 'f'));
				break;
			case ';':
				algo.push_back(copy_data(str));
				str_sizes.push_back(str.size());
				str.clear();
				max_stack = max(max_stack, cur_stack);
				break;
			}
			break;
		case 'v':
			if (word == ";") {
				state = '\0';
			}
			else {
				vars[word] = mem_require;
				mem_require += 4;
			}
			break;
		}
	}

	return algoritm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack);
}

void execute(algoritm algo) {
	void* data = malloc(algo.mem_require);
	void** stack = new void* [algo.stack_size];
	size_t stack_c;

	for (int _ = 0; _ < 1e6; _++) {
		for (int i = 0; i < algo.string_count; i++) {
			stack_c = 0;
			for (int j = 0; j < algo.string_sizes[i]; j++) {
				operand op = algo.strings[i][j];

				void* l, * r, (*f)(void*&, void*, void*);
				switch (op.type) {
				case 'f':
					r = stack[stack_c--];
					l = stack[stack_c--];

					f = (void(*)(void*&, void*, void*))op.func;
					f(op.value, l, r);
				case 'r':
					stack[++stack_c] = op.value;
					break;
				case 'l':
					stack[++stack_c] = (char*)data + (int)op.value;
					break;
				}
			}
		}
	}

	for (int i = 0; i < algo.mem_require / 4; i++) {
		cout << *((int*)data + i) << endl;
	}

	free(data);
}

int main() {
	ifstream code("code.t");

	algoritm algo = compile(code);

	chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
	execute(algo);
	chrono::time_point<chrono::system_clock> end = chrono::system_clock::now();
	cout << chrono::duration_cast<chrono::nanoseconds>(end - start).count() / 1e6;
}