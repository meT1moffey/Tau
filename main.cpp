#include<iostream>
#include<fstream>

#include<string>
#include<vector>
#include<map>

#include<chrono>

using byte = unsigned char;

using namespace std;
using namespace chrono;

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
			case '.':
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
				case '.':
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
	operand(void(*func)(void*&, void*, void*), char type, size_t size) : func(func), type(type), value(new byte[size]) {}
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

enum type {
	int32,
	int64,
	float32,
};

struct var {
	size_t id = 0;
	type t;

	var() {}
	var(size_t id, type t) : id(id), t(t) {}
};

template<typename T>
void sum(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l + *(T*)r; }
template<int size>
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, size); }

template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

algoritm compile(istream& input) {
	vector<string> words = read_words(input);
	map<string, var> vars;
	map<string, pair<type, size_t>> types = { {"int32", {int32, 4}}, {"int64", {int64, 8}}, {"float32", {float32, 4}} };
	size_t mem_require = 0, max_stack = 0, cur_stack = 0;
	vector<operand*> algo;
	vector<size_t> str_sizes;
	vector<operand> str;
	vector<type> str_types;

	string var_t;
	int64_t hc_num;

	char state = 'b';
	for (string word : words) {
		switch (state) {
		case 'b':
			if (word == "vars") {
				state = 't';
				break;
			}
		case 'f':
			str.push_back(operand(new float(hc_num + stoll(word) * pow(0.1, word.size())), 'r'));
			str_types.push_back(float32);
			cur_stack++;
			state = '\0';
			break;
		case '1':
			if (word == ".") {
				state = 'f';
				break;
			}
			else {
				str.push_back(operand(new int64_t(hc_num), 'r'));
				str_types.push_back(int64);
				cur_stack++;
				state = '\0';
			}
		case '\0':
			if (vars.find(word) != vars.end()) {
				str.push_back(operand((void*)vars[word].id, 'l'));
				str_types.push_back(vars[word].t);
				cur_stack++;
			}
			else if (isdigit(word[0])) {
				hc_num = stoll(word);
				state = '1';
			}
			else switch (word[0]) {
			case '=':
				switch (str_types.back()) {
				case int32:
					str.push_back(operand(&set<4>, 'f', 4));
					str_types.push_back(int32);
					break;
				case int64:
					str.push_back(operand(&set<8>, 'f', 8));
					str_types.push_back(int64);
					break;
				case float32:
					str.push_back(operand(&set<4>, 'f', 4));
					str_types.push_back(float32);
					break;
				}
				break;
			case '+':
				switch (str_types.back()) {
				case int32:
					str.push_back(operand(&sum<int>, 'f', 4));
					str_types.push_back(int32);
					break;
				case int64:
					str.push_back(operand(&sum<int64_t>, 'f', 8));
					str_types.push_back(int64);
					break;
				case float32:
					str.push_back(operand(&sum<float>, 'f', 4));
					str_types.push_back(float32);
					break;
				}
				break;
			case ';':
				algo.push_back(copy_data(str));
				str_sizes.push_back(str.size());
				str.clear();
				str_types.clear();
				max_stack = max(max_stack, cur_stack);
				break;
			}
			break;
		case 't':
			if (word == ";") {
				state = '\0';
				break;
			}
			var_t = word;
			state = 'v';
			break;
		case 'v':
			vars[word] = var(mem_require, types[var_t].first);
			mem_require += types[var_t].second;
			state = 't';
			break;
		}
	}

	return algoritm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack);
}

void execute(algoritm algo) {
	void* data = new byte[algo.mem_require];
	void** stack = new void* [algo.stack_size];
	size_t stack_c;

	//int repeats = 1e6;
	//time_point<system_clock> start = system_clock::now();
	//for(int _ = 0; _ < repeats; _++) {
	for (int i = 0; i < algo.string_count; i++) {
		stack_c = 0;
		for (int j = 0; j < algo.string_sizes[i]; j++) {
			operand op = algo.strings[i][j];

			void* l, * r, (*f)(void*&, void*, void*);
			switch (op.type) {
			case 'f':
				r = stack[--stack_c];
				l = stack[--stack_c];

				f = (void(*)(void*&, void*, void*))op.func;
				f(op.value, l, r);
			case 'r':
				stack[stack_c++] = op.value;
				break;
			case 'l':
				stack[stack_c++] = (byte*)data + (size_t)op.value;
				break;
			}
		}
	}
	//}
	//time_point<system_clock> end = system_clock::now();
	//cout << "Total time: " << duration_cast<nanoseconds>(end - start).count() / float(repeats) << "ns\n";

	cout << hex;
	for (byte* it = (byte*)data + algo.mem_require - 1; it + 1 != data; it--)
		cout << (int)*it << ' ';

	delete[] data;
}

int main() {
	ifstream code("code.t");
	algoritm algo = compile(code);
	execute(algo);
}