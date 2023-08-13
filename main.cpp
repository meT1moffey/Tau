#include<iostream>
#include<iomanip>
#include<fstream>

#include<cmath>

#include<string>
#include<vector>
#include<map>

#include<chrono>

using byte = uint8_t;

const int PTR_SIZE = sizeof(void*);

using namespace std;
using namespace chrono;

struct algorithm;

void execute(algorithm*, void*&, byte*);

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
			if (c == '.' && isdigit(word.back())) {
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
			case '-':
			case ';':
			case '.':
			case '(':
			case ')':
			case '?':
			case '{':
			case '}':
			case '@':
			case '*':
			case '&':
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
			else if (word == "var" || word == "object") {
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
	size_t size = 0, pos = -1;
	char type = 'g';

	operand() {}
	operand(void* value, char type) {
		this->value = value;
		this->type = type;
	}
	operand(void(value)(void*&, void*, void*), size_t pos, size_t size) {
		this->value = value;
		this->type = 'f';
		this->pos = pos;
		this->size = size;
	}
	operand(void(value)(void*&, void*), size_t pos, size_t size) {
		this->value = value;
		this->type = 'u';
		this->pos = pos;
		this->size = size;
	}
};

// ��������� ���������
struct algorithm {
	operand** strings = nullptr;
	size_t* string_sizes = nullptr;
	size_t string_count = 0, mem_require = 0, stack_mem_require = 0, stack_size = 0;

	algorithm() {}
	algorithm(operand** strings, size_t* string_sizes, size_t string_count, size_t mem_require, size_t stack_mem_require, size_t stack_size) {
		this->strings = strings;
		this->string_sizes = string_sizes;
		this->string_count = string_count;
		this->mem_require = mem_require;
		this->stack_mem_require = stack_mem_require;
		this->stack_size = stack_size;
	}
};

// ������������ ����� ������
enum type {
	blank,
	func,
	int8,
	int32,
	int64,
	uint64,
	float32,
	float64,
	ptr,
	object
};

struct full_type {
	type t = blank;
	vector<full_type> args;

	full_type() {}
	full_type(type t) : t(t) {}
	full_type(type t, vector<full_type> args) : t(t), args(args) {}
};

// ��������� ����������
struct var {
	void* pos = nullptr;
	full_type t = blank;

	var() {}
	var(void* pos, full_type t) : pos(pos), t(t) {}
};

// ������� �������� ���� ��������
template<typename T>
void sum(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l + *(T*)r; }

template<typename T>
void diff(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l - *(T*)r; }

// ������� ����������� ��������
template<int size>
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, size); }

// ������� ��������� �������� �� ���������
template<int size>
void equal(void*& buf, void* l, void* r) { *(bool*)buf = memcmp(l, r, size) == 0; }

// ������� ������ ���������
void call(void*& buf, void* l, void* r) { execute((algorithm*)l, buf, (byte*)r); }

template<typename T>
void create(void*& buf, void* l) { *(void**)buf = malloc(*(T*)l); }

void destroy(void*& buf, void* l) { free(*(void**)l); }

void from(void*& buf, void* l) { buf = *(void**)l;  }

void addr(void*& buf, void* l) { *(void**)buf = l; }

template<typename T>
void shift(void*& buf, void* l, void* r) { buf = (byte*)l + *(T*)r; }

// ������� ��� ����������� ������� � ������������ ������
template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

void write(string text) {
	cout << text;
}

map<string, type> types = {
	{"blank", blank},
	{"int", int32},
	{"long", int64},
	{"float", float32},
	{"byte", int8},
	{"ulong", uint64},
	{"double", float64},
	{"ptr", ptr}
}; // ���� ������

map<type, size_t> t_sizes = {
	{blank, 0},
	{int8, 1},
	{int32, 4},
	{int64, 8},
	{uint64, 8},
	{float32, 4},
	{float64, 8},
	{ptr, PTR_SIZE}
};

struct func_info {
	string name;
	vector<string> words;
	full_type result_t;
	map<string, var> args;

	func_info() {}
	func_info(string name, vector<string> words, full_type result_t, map<string, var> args)
		: name(name), words(words), result_t(result_t), args(args) {}
};

// ������� ��� ���������� ����
algorithm compile_function(func_info info, map<string, var> globals) {
	map<string, int> marks = get_marks(info.words);
	map<string, var> vars; // ��� ��� �������� ����������
	size_t mem_require = 0, max_stack = 0, cur_stack = 0, cur_stack_require = 0, max_stack_require = 0;
	vector<operand*> algo; // ������ ������ ���������
	vector<size_t> str_sizes; // ������ ������� �����
	vector<operand> str; // ������ �������� ������� ������
	vector<full_type> str_types; // ������ ���� ��������� ������� ������

	type buf_t; // ��� ������� ����������

	string text = "", buf_name; // ���� ��� ������

	char state = '\0';
	for (string word : info.words) {
		switch (state) {
		case '\0':
			if (word == "var") {
				// �������� ����� ������������ ���������� ����������
				state = 't';
				break;
			}
			else if (word == "mark") {
				// �������� �����, ������������ ����� ��� �������� (��� �������)
				state = 'm';
			}
			else if (word == "jump") {
				// �������� �����, ����������� ������� � �����
				state = 'j';
			}
			else if (word == "log") {
				str.push_back(operand(nullptr, -1));
			}
			else if (word == "create") {
				switch (str_types[str_types.size() - 1].t) {
				case int8:
					str.push_back(operand(&create<int8_t>, cur_stack_require, PTR_SIZE));
					str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				case int32:
					str.push_back(operand(&create<int>, cur_stack_require, PTR_SIZE));
					str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				case int64:
					str.push_back(operand(&create<int64_t>, cur_stack_require, PTR_SIZE));
					str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				case uint64:
					str.push_back(operand(&create<uint64_t>, cur_stack_require, PTR_SIZE));
					str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				}
			}
			else if (word == "destroy") {
				switch (str_types[str_types.size() - 1].t) {
				case ptr:
					str.push_back(operand(&destroy, cur_stack_require, 0));
					str_types.pop_back();
					str_types.push_back(blank);
					cur_stack_require += PTR_SIZE;
					break;
				}
			}
			else if (word == "result") {
				str.push_back(operand(nullptr, 'r'));
				str_types.push_back(info.result_t);
				cur_stack++;
			}
			else if (word == "object") {
				state = 'o';
			}
			else if (word == "write") {
				state = 'w';
			}
			else if (vars.find(word) != vars.end()) {
				// ���� ����� - ����������, ��������� �� ����� � ������ ���������
				str.push_back(operand(vars[word].pos, 'l'));
				str_types.push_back(vars[word].t);
				cur_stack++;
			}
			else if (info.args.find(word) != info.args.end()) {
				// ���� ����� - ����������, ��������� �� ����� � ������ ���������
				str.push_back(operand(info.args[word].pos, 'a'));
				str_types.push_back(info.args[word].t);
				cur_stack++;
			}
			else if (globals.find(word) != globals.end()) {
				// ���� ����� - �������, ��� ����������� � ��������
				str.push_back(operand(globals[word].pos, 'g'));
				str_types.push_back(globals[word].t);
				cur_stack++;
			}
			else if (isdigit(word[0])) {
				// ���� ����� ���������� � �����, �������� ���������� �����
				if (word.find('.') != string::npos) {
					// ���� �������� �����, ������, ��������� ����� - ���������� ����� (����� �����)
					str.push_back(operand(new float(stof(word)), 'g'));
					str_types.push_back(float32);
					cur_stack++;
				}
				else {
					// ���� �������� ����� �����, ��������� ��� � ������ ���������
					str.push_back(operand(new int64_t(stoll(word)), 'g'));
					str_types.push_back(int64);
					cur_stack++;
				}
			}
			else switch (word[0]) {
			case '=':
				if(word.size() == 1)
				// ���� ����� - '=', ��������� ��������������� ������� � ������ ���������
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
				case comb(int32, int64):
				case comb(int64, int32):
					str.push_back(operand(&set<4>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(int64, int64):
					str.push_back(operand(&set<8>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(float32, float32):
				case comb(float32, float64):
				case comb(float64, float32):
					str.push_back(operand(&set<4>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
				case comb(int8, int32):
				case comb(int8, int64):
				case comb(int32, int8):
				case comb(int64, int8):
					str.push_back(operand(&set<1>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int8);
					cur_stack_require += 1;
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&set<8>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(uint64);
					cur_stack_require += 8;
					break;
				case comb(float64, float64):
					str.push_back(operand(&set<8>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float64);
					cur_stack_require += 8;
					break;
				case comb(ptr, ptr):
					str.push_back(operand(&set<PTR_SIZE>, cur_stack_require, PTR_SIZE));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				}
				else switch (word[1]) {
				case '=':
					// ���� == (��������� �� ���������)
					switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
					case comb(int32, int32):
					case comb(int32, int64):
					case comb(int64, int32):
						str.push_back(operand(&equal<4>, cur_stack_require, 1));
						str_types.pop_back(); str_types.pop_back();
						str_types.push_back(int8);
						cur_stack_require += 1;
						break;
					case comb(int64, int64):
						str.push_back(operand(&equal<8>, cur_stack_require, 1));
						str_types.pop_back(); str_types.pop_back();
						str_types.push_back(int8);
						cur_stack_require += 1;
						break;
					case comb(float32, float32):
						str.push_back(operand(&equal<4>, cur_stack_require, 1));
						str_types.pop_back(); str_types.pop_back();
						str_types.push_back(int8);
						cur_stack_require += 1;
						break;
					case comb(int8, int8):
					case comb(int8, int32):
					case comb(int8, int64):
					case comb(int32, int8):
					case comb(int64, int8):
						str.push_back(operand(&equal<1>, cur_stack_require, 1));
						str_types.pop_back(); str_types.pop_back();
						str_types.push_back(int8);
						cur_stack_require += 1;
						break;
					case comb(uint64, uint64):
						str.push_back(operand(&equal<8>, cur_stack_require, 1));
						str_types.pop_back(); str_types.pop_back();
						str_types.push_back(int8);
						cur_stack_require += 1;
						break;
					}
					break;
				}
				break;
			case '+':
				// ���� ����� - '+', ��������� ��������������� ������� � ������ ���������
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
				case comb(int32, int64):
				case comb(int64, int32):
					str.push_back(operand(&sum<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(int64, int64):
					str.push_back(operand(&sum<int64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(float32, float32):
				case comb(float32, float64):
				case comb(float64, float32):
					str.push_back(operand(&sum<float>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
				case comb(int8, int32):
				case comb(int8, int64):
				case comb(int32, int8):
				case comb(int64, int8):
					str.push_back(operand(&sum<int8_t>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int8);
					cur_stack_require += 1;
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&sum<uint64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(uint64);
					cur_stack_require += 8;
					break;
				case comb(float64, float64):
					str.push_back(operand(&sum<double>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float64);
					cur_stack_require += 8;
					break;
				case comb(ptr, int8):
					str.push_back(operand(&sum<int8_t>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, int32):
					str.push_back(operand(&sum<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, int64):
					str.push_back(operand(&sum<int64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, uint64):
					str.push_back(operand(&sum<uint64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				}
				break;
			case '-':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
				case comb(int32, int64):
				case comb(int64, int32):
					str.push_back(operand(&diff<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(int64, int64):
					str.push_back(operand(&diff<int64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(float32, float32):
				case comb(float32, float64):
				case comb(float64, float32):
					str.push_back(operand(&diff<float>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
				case comb(int8, int32):
				case comb(int8, int64):
				case comb(int32, int8):
				case comb(int64, int8):
					str.push_back(operand(&diff<int8_t>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int8);
					cur_stack_require += 1;
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&diff<uint64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(uint64);
					cur_stack_require += 8;
					break;
				case comb(float64, float64):
					str.push_back(operand(&diff<double>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float64);
					cur_stack_require += 8;
					break;
				case comb(ptr, int8):
					str.push_back(operand(&diff<int8_t>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, int32):
					str.push_back(operand(&diff<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, int64):
					str.push_back(operand(&diff<int64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				case comb(ptr, uint64):
					str.push_back(operand(&diff<uint64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				}
				break;
			case '*':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(ptr, int8):
					str.pop_back();
					str.push_back(operand(&from, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int8);
					cur_stack_require += 1;
					break;
				case comb(ptr, int32):
					str.pop_back();
					str.push_back(operand(&from, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(ptr, int64):
					str.pop_back();
					str.push_back(operand(&from, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				}
				break;
			case '&':
				str.push_back(operand(&addr, cur_stack_require, PTR_SIZE));
				str_types.pop_back();
				str_types.push_back(ptr);
				cur_stack_require += PTR_SIZE;
				break;
			case '@':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(func, object):
					str.push_back(operand(&call, cur_stack_require, 0));
					buf_t = str_types[str_types.size() - 2].args[0].t;
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(buf_t);
					break;
				}
				break;
			case '.':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(object, int64):
					// ������ �������� ������ ���� ����������
					int i = 0;
					for (int k = 0; k < *(int64_t*)str.back().value; k += t_sizes[str_types[str_types.size() - 2].args[i++].t]);
					buf_t = str_types[str_types.size() - 2].args[i].t;
					str.push_back(operand(&shift<int64_t>, cur_stack_require, 0));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(buf_t);
					break;
				}
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
				cur_stack = 0;
				max_stack_require = max(max_stack_require, cur_stack_require);
				cur_stack_require = 0;
				break;
			}
			break;
		case 't':
			if (word == ";") {
				// ���� ����� - ';', ����������� �������� ����������
				state = '\0';
				break;
			}
			buf_t = types[word];
			state = 'v';
			break;
		case 'v':
			// ���� ��������� 'v', ������, ��������� ����� - ��� ����������
			vars[word] = var((void*)mem_require, buf_t); // ��������� ���������� � ������ ����������
			mem_require += t_sizes[buf_t]; // ����������� ���������� �� ������
			state = 't';
			break;
		case 'o':
			buf_name = word;
			vars[buf_name] = var((void*)mem_require, object);
			state = 'l';
			break;
		case 'l':
			if (word == ";") {
				state = '\0';
				break;
			}
			vars[buf_name].t.args.push_back(types[word]);
			mem_require += t_sizes[types[word]];
			break;
		case 'm':
			// ����� ������ ���� ��� �����, ������� ��� �������
			state = '\0';
			break;
		case 'j':
			str.push_back(operand((void*)marks[word], 'j'));
			str_types.push_back(blank);
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
				state = 'g';
			}
			break;
		case 'g':
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
	return algorithm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack_require, max_stack);
}

// ������� �������� ������� � ����
map<string, var> parse_functions(vector<string> words) {
	vector<func_info> parsed;
	map<string, var> globals;

	string name;
	type buf_type;
	vector<string> func_words;
	map<string, var> args;
	size_t argl = 0;
	char state = '\0';

	for (string word : words) {
		switch (state) {
		case '\0':
			if (word == "func") {
				state = 'r';
			}
			else if (word == "var") {
				state = 't';
			}
			break;
		case 'r':
			buf_type = types[word];
			state = 'f';
			break;
		case 'f':
			name = word;
			state = 'o';
			break;
		case 'v':
			globals[word] = var(malloc(t_sizes[buf_type]), buf_type);
			state = 't';
			break;
		case 't':
			if (word == ";") {
				state = '\0';
				break;
			}
			buf_type = types[word];
			state = 'v';
			break;
		case 'o':
			if (word == "{") {
				state = 'b';
				break;
			}
			buf_type = types[word];
			state = 'a';
			break;
		case 'a':
			args[word] = var((void*)argl, buf_type);
			argl += t_sizes[buf_type];
			state = 'o';
			break;
		case 'b':
			if (word == "}") {
				state = '\0';
				parsed.push_back(func_info(name, func_words, buf_type, args));
				globals[name] = var(new algorithm, full_type(func, { buf_type }));
				func_words.clear();
				args.clear();
				argl = 0;
			}
			else {
				func_words.push_back(word);
			}
			break;
		}
	}

	for (auto fun : parsed)
		*(algorithm*)(globals[fun.name].pos) = compile_function(fun, globals);

	return map<string, var>(globals);
}

#define MEASURE 0;
// ������� ���������� ���������
void execute(algorithm* algo, void*& result_buf, byte* args) {
	byte* data = (byte*)malloc(algo->mem_require); // ������ ��� ������
	byte* stack_data = (byte*)malloc(algo->stack_mem_require);
	void** stack = new void* [algo->stack_size]; // ���� ���������
	size_t stack_c;
	for (int i = 0; i < algo->string_count; i++) {
		stack_c = 0;
		for (int j = 0; j < algo->string_sizes[i]; j++) {
			operand* op = &algo->strings[i][j];

			void* l, *r, *f, *buf;
			switch (op->type) {
			case 'f':
				// ���� ��� �������� - �������, ��������� ������� ��� ���������� �� �����
				r = stack[--stack_c];
				l = stack[--stack_c];

				f = op->value;
				buf = stack_data + op->pos;
				((void(*)(void*&, void*, void*))f)(buf, l, r);
				stack[stack_c++] = buf;
				break;
			case 'u':
				l = stack[--stack_c];

				f = op->value;
				buf = stack_data + op->pos;
				((void(*)(void*&, void*))f)(buf, l);
				stack[stack_c++] = buf;
				break;
			case 'g':
				// ���� ��� �������� - ��������, �������� ��� �� ����
				stack[stack_c++] = op->value;
				break;
			case 'l':
				// ���� ��� �������� - �����, �������� ��������������� �������� �� ������ �� ����
				stack[stack_c++] = data + (size_t)op->value;
				break;
			case 'r':
				stack[stack_c++] = result_buf;
				break;
			case 'a':
				stack[stack_c++] = args + (size_t)op->value;
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
			case -1:
				// ����� ���� �������� � ������. ������������� ��� �������
				cout << hex << setfill('0');
				for (byte* it = data + algo->mem_require - 1; it + 1 != data; it--)
					cout << setw(2) << +*it << ' ';
				cout << '\n';
				break;
			}
		}
	newExpression:;
	}

	free(data);
	free(stack_data);
	delete[] stack;
}

int main() {
	ifstream code("code.tau");
	vector<string> words = read_words(code);
	map<string, var> globals = parse_functions(words);

	void* nullbuf = malloc(0);
#if MEASURE
	time_point<system_clock> start = system_clock::now();
	int count = 1e4;
	for (int _ = 0; _ < count; _++) {
#endif
		execute((algorithm*)globals["main"].pos, nullbuf, nullptr);
#if MEASURE
	}
	time_point<system_clock> end = system_clock::now();
	cout << duration_cast<nanoseconds>(end - start).count() / (double)count << "ns\n";
#endif

	for (auto value : globals) {
		delete value.second.pos;
	}
}
