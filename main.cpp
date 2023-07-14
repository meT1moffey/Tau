#include<iostream>
#include<fstream>

#include<cmath>

#include<string>
#include<vector>
#include<map>

#include<chrono>

using byte = unsigned char;

using namespace std;
using namespace chrono;

struct algorithm;

void execute(algorithm*);

// ���������� 2 ����� � ����. ������������� ��� switch/case
constexpr int64_t comb(int f, int s) {
	return ((0ll + f) << 32) + s;
}

// ������� ��� ������ ���� �� ������
vector<string> read_words(istream& input) {
	vector<string> words;
	char state = '\0';
	string word;

	char c;
	for (c = '\n'; !input.eof(); input.read(&c, 1)) {
		switch (state) {
		case 'o':
			switch (comb(word[0], c)) {
			case comb('=', '='):
				word.push_back(c);
				words.push_back(word);
				word.clear();
				break;
			default:
				goto newWord;
			}
			break;
		case 'a':
			// ���� ������ �������� ������ ��� ������, ��������� ��� � �������� �����
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
				break;
			}
		newWord:
			// ���� �������� ����� �����, ��������� ��� � ������ ���� � ���������� ������� �����
			words.push_back(word);
			word.clear();
			state = '\0';
		case '\0':
			// ���� ������ �������� ������ ��� ������, ��������� ��� � �������� �����
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
				state = 'a';
			}
			else switch (c) {
				// ���� ������ �������� ����� �� ���� �������������, ��������� ��� ��� ��������� �����
			case '+':
			case ';':
			case '.':
			case '(':
			case ')':
			case '?':
			case '{':
			case '}':
			case '@':
				words.push_back(string(1, c));
				break;
			case '=':
				state = 'o';
				word.push_back(c);
				break;
			case '"':
				state = 's';
				words.push_back(string(1, c));
				break;
			}
			break;
		case 's':
			if (c == '"') {
				state = '\0';
				words.push_back(string(1, c));
			}
			else {
				words.push_back(string(1, c));
			}
			break;
		}
	}

	if (c != '\n') {
		c = '\n';
		goto newWord;
	}

	return words;
}

// ������� ���������� ����� ��������
map<string, int> get_marks(vector<string> words) {
	map<string, int> marks; // ����� ��� ��������
	int next_line = 0; // ����� _����������_ ���������

	char state = '\0';
	for (string word : words) {
		switch (state) {
		case '\0':
			if (word == ";")
				next_line++;
			else if (word == "mark") {
				// �������� ����� ������������ ����� ��������
				state = 'm';
			}
			else if (word == "vars") {
				// ������ ��������� �� ������ � ��������, ������� ������ ������ ���� ���������
				state = 'v';
			}
			break;
		case 'v':
			if (word == ";")
				state = '\0';
			break;
		case 'm':
			// ��������� ����� -- ��� �����
			marks[word] = next_line;
			state = '\0';
			break;
		}
	}

	return marks;
}

// ��������� ��������
struct operand {
	void* value = nullptr;
	void(*func)(void*&, void*, void*) = nullptr;
	char type = 'r';

	operand() {}
	operand(void* value, char type)
	{
		this->value = value;
		this->type = type;
	}
	operand(void(*func)(void*&, void*, void*), char type, size_t size)
	{
		this->func = func;
		this->type = type;
		this->value = new byte[size];
	}
};

// ��������� ���������
struct algorithm {
	operand** strings = nullptr;
	size_t* string_sizes = nullptr;
	size_t string_count = 0, mem_require = 0, stack_size = 0;

	algorithm() {}
	algorithm(operand** strings, size_t* string_sizes, size_t string_count, size_t mem_require, size_t stack_size)
	{
		this->strings = strings;
		this->string_sizes = string_sizes;
		this->string_count = string_count;
		this->mem_require = mem_require;
		this->stack_size = stack_size;
	}
};

// ������������ ����� ������
enum type {
	func,
	int8,
	int32,
	int64,
	uint64,
	float32,
	float64
};

// ��������� ����������
struct var {
	size_t id = 0;
	type t;

	var() {}
	var(size_t id, type t)
	{
		this->id = id;
		this->t = t;
	}
};

// ������� �������� ���� ��������
template<typename T>
void sum(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l + *(T*)r; }

// ������� ����������� ��������
template<int size>
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, size); }

// ������� ��������� �������� �� ���������
template<int size>
void equal(void*& buf, void* l, void* r) { *(bool*)buf = memcmp(l, r, size) == 0; }

// ������� ������ ���������
void call(void*& buf, void* l, void* r) { execute((algorithm*)l); }

// ������� ��� ����������� ������� � ������������ ������
template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

void write(string text) {
	cout << text;
}

// ������� ��� ���������� ����
algorithm compile_function(vector<string> words, map<string, algorithm*> functions) {
	map<string, int> marks = get_marks(words);
	map<string, var> vars; // ��� ��� �������� ����������
	map<string, pair<type, size_t>> types = { {"int", {int32, 4}}, {"long", {int64, 8}}, {"float", {float32, 4}}, {"byte", {int8, 1}}, {"ulong", {uint64, 8}}, {"double", {float64, 8 }} }; // ���� ������
	size_t mem_require = 0, max_stack = 0, cur_stack = 0;
	vector<operand*> algo; // ������ ������ ���������
	vector<size_t> str_sizes; // ������ ������� �����
	vector<operand> str; // ������ �������� ������� ������
	vector<type> str_types; // ������ ���� ��������� ������� ������

	string var_t; // ��� ������� ����������
	int64_t hc_num; // ��������� �������� �����

	string text = ""; // ���� ��� ������

	char state = 'b';
	for (string word : words) {
		switch (state) {
		case 'b':
			if (word == "vars") {
				state = 't';
				break;
			}
		case 'f':
			// ���� ��������� 'f', ������, ��������� ����� - ���������� ����� (����� �����)
			str.push_back(operand(new float(hc_num + stoll(word) * pow(0.1, word.size())), 'r'));
			str_types.push_back(float32);
			cur_stack++;
			state = '\0';
			break;
		case '1':
			if (word == ".") {
				// ���� �������� �����, ������, ��������� ����� - ���������� ����� (����� �����)
				state = 'f';
				break;
			}
			else {
				// ���� �������� ����� �����, ��������� ��� � ������ ���������
				str.push_back(operand(new int64_t(hc_num), 'r'));
				str_types.push_back(int64);
				cur_stack++;
				state = '\0';
			}
		case '\0':
			if (word == "mark") {
				// �������� �����, ������������ ����� ��� �������� (��� �������)
				state = 'm';
			}
			else if (word == "jump") {
				// �������� �����, ����������� ������� � �����
				state = 'j';
			}
			else if (word == "write") {
				state = 'w';
			}
			else if (vars.find(word) != vars.end()) {
				// ���� ����� - ����������, ��������� �� ����� � ������ ���������
				str.push_back(operand((void*)vars[word].id, 'l'));
				str_types.push_back(vars[word].t);
				cur_stack++;
			}
			else if (functions.find(word) != functions.end()) {
				// ���� ����� - �������, ��� ����������� � ��������
				str.push_back(operand(functions[word], 'r'));
				str_types.push_back(func);
				cur_stack++;
			}
			else if (isdigit(word[0])) {
				// ���� ����� ���������� � �����, �������� ���������� �����
				hc_num = stoll(word);
				state = '1';
			}
			else switch (word[0]) {
			case '=':
				if(word.size() == 1)
				// ���� ����� - '=', ��������� ��������������� ������� � ������ ���������
				switch (comb(str_types[str_types.size() - 1], str_types[str_types.size() - 2])) {
				case comb(int32, int32):
				case comb(int32, int64):
				case comb(int64, int32):
					str.push_back(operand(&set<4>, 'f', 4));
					str_types.push_back(int32);
					break;
				case comb(int64, int64):
					str.push_back(operand(&set<8>, 'f', 8));
					str_types.push_back(int64);
					break;
				case comb(float32, float32):
				case comb(float32, float64):
				case comb(float64, float32):
					str.push_back(operand(&set<4>, 'f', 4));
					str_types.push_back(float32);
					break;
				case comb(int8, int8):
				case comb(int8, int32):
				case comb(int8, int64):
				case comb(int32, int8):
				case comb(int64, int8):
					str.push_back(operand(&set<1>, 'f', 1));
					str_types.push_back(int8);
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&set<8>, 'f', 8));
					str_types.push_back(uint64);
					break;
				case comb(float64, float64):
					str.push_back(operand(&set<8>, 'f', 8));
					str_types.push_back(float64);
					break;
				}
				else switch (word[1]) {
				case '=':
					// ���� == (��������� �� ���������)
					switch (comb(str_types[str_types.size() - 1], str_types[str_types.size() - 2])) {
					case comb(int32, int32):
					case comb(int32, int64):
					case comb(int64, int32):
						str.push_back(operand(&equal<4>, 'f', 1));
						str_types.push_back(int8);
						break;
					case comb(int64, int64):
						str.push_back(operand(&equal<8>, 'f', 1));
						str_types.push_back(int8);
						break;
					case comb(float32, float32):
						str.push_back(operand(&equal<4>, 'f', 1));
						str_types.push_back(int8);
						break;
					case comb(int8, int8):
					case comb(int8, int32):
					case comb(int8, int64):
					case comb(int32, int8):
					case comb(int64, int8):
						str.push_back(operand(&equal<1>, 'f', 1));
						str_types.push_back(int8);
						break;
					case comb(uint64, uint64):
						str.push_back(operand(&equal<8>, 'f', 1));
						str_types.push_back(int8);
						break;
					}
					break;
				}
				break;
			case '+':
				// ���� ����� - '+', ��������� ��������������� ������� � ������ ���������
				switch (comb(str_types[str_types.size() - 1], str_types[str_types.size() - 2])) {
				case comb(int32, int32):
				case comb(int32, int64):
				case comb(int64, int32):
					str.push_back(operand(&sum<int>, 'f', 4));
					str_types.push_back(int32);
					break;
				case comb(int64, int64):
					str.push_back(operand(&sum<int64_t>, 'f', 8));
					str_types.push_back(int64);
					break;
				case comb(float32, float32):
				case comb(float32, float64):
				case comb(float64, float32):
					str.push_back(operand(&sum<float>, 'f', 4));
					str_types.push_back(float32);
					break;
				case comb(int8, int8):
				case comb(int8, int32):
				case comb(int8, int64):
				case comb(int32, int8):
				case comb(int64, int8):
					str.push_back(operand(&sum<char>, 'f', 1));
					str_types.push_back(int8);
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&sum<unsigned long long>, 'f', 8));
					str_types.push_back(uint64);
					break;
				case comb(float64, float64):
					str.push_back(operand(&sum<double>, 'f', 8));
					str_types.push_back(float64);
					break;
				}
				break;
			case '@':
				str.push_back(operand(&call, 'f', 0));
				break;
			case '?':
				str.push_back(operand(nullptr, 'c'));
				break;
			case ';':
				// ���� ����� - ';', ����������� ������� ������ ���������
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
				// ���� ����� - ';', ����������� �������� ����������
				state = '\0';
				break;
			}
			var_t = word;
			state = 'v';
			break;
		case 'v':
			// ���� ��������� 'v', ������, ��������� ����� - ��� ����������
			vars[word] = var(mem_require, types[var_t].first); // ��������� ���������� � ������ ����������
			mem_require += types[var_t].second; // ����������� ���������� �� ������
			state = 't';
			break;
		case 'm':
			// ����� ������ ���� ��� �����, ������� ��� �������
			state = '\0';
			break;
		case 'j':
			str.push_back(operand((void*)marks[word], 'j'));
			state = '\0';
			break;
		case 'w':
			if (word == "(") {
				text = "";
				state = 's';
			}
			else if (word == ";") {
				state = '\0';
			}
			break;
		case 's':
			if (word == "\"") {
				state = 'r';
			}
			break;
		case 'r':
			if (word != "\"") {
				text += word;
			}
			else {
				write(text);
				state = 'w';
			}
			break;
		}
	}

	// ������� ��������� ��������� � ���������� ��
	return algorithm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack);
}

// ������� �������� ������� � ����
map<string, algorithm*> parse_functions(vector<string> words) {
	vector<pair<string, vector<string>>> parsed;
	map<string, algorithm*> functions;

	string name;
	vector<string> func_words;
	char state = '\0';

	for (string word : words) {
		switch (state) {
		case '\0':
			name = word;
			state = 'o';
			break;
		case 'o':
			if (word == "{") {
				state = 'b';
				break;
			}
		case 'b':
			if (word == "}") {
				state = '\0';
				parsed.push_back({ name, func_words });
				functions[name] = new algorithm;
				func_words.clear();
			}
			else {
				func_words.push_back(word);
			}
			break;
		}
	}

	for (auto fun : parsed) {
		*functions[fun.first] = compile_function(fun.second, functions);
	}

	return functions;
}

#define MEASURE true;
// ������� ���������� ���������
void execute(algorithm* algo) {
	void* data = malloc(algo->mem_require); // ������ ��� ������
	void** stack = new void* [algo->stack_size]; // ���� ���������
	size_t stack_c;
	for (int i = 0; i < algo->string_count; i++) {
		stack_c = 0;
		for (int j = 0; j < algo->string_sizes[i]; j++) {
			operand* op = &algo->strings[i][j];

			void* l, * r, (*f)(void*&, void*, void*);
			switch (op->type) {
			case 'f':
				// ���� ��� �������� - �������, ��������� ������� ��� ���������� �� �����
				r = stack[--stack_c];
				l = stack[--stack_c];

				f = (void(*)(void*&, void*, void*))op->func;
				f(op->value, l, r);
			case 'r':
				// ���� ��� �������� - ��������, �������� ��� �� ����
				stack[stack_c++] = op->value;
				break;
			case 'l':
				// ���� ��� �������� - �����, �������� ��������������� �������� �� ������ �� ����
				stack[stack_c++] = (byte*)data + (size_t)op->value;
				break;
			case 'j':
				// ��� ���������� ����� �������� ������������� ������� � ��������� ������
				i = (int)op->value - 1;
				goto newExpression;
				break;
			case 'c':
				// ��� ���������� ���������� ������ ���� ���������� ��������. ���� �� = 0, ������������� �� ��������� ������
				if (*(byte*)stack[--stack_c] == 0) {
					goto newExpression;
				}
				break;
			}
		}
	newExpression:;
	}

	delete[] stack;
	// ������� �������� �� ������ � ����������������� �������
	/*cout << dec;
	for (byte* it = (byte*)data + algo->mem_require - 1; it + 1 != data; it--)
		cout << (int)*it << ' ';*/

	free(data);
}

int main() {
	ifstream code("code.t");
	vector<string> words = read_words(code);
	map<string, algorithm*> functions = parse_functions(words);

	time_point<system_clock> start = system_clock::now();
	int count = 1e4;
	for (int _ = 0; _ < count; _++) {
		execute(functions["main"]);
	}
	time_point<system_clock> end = system_clock::now();
	cout << duration_cast<nanoseconds>(end - start).count() / (double)count << "ns\n";
}
