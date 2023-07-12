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

// Функция для чтения слов из потока
vector<string> read_words(istream& input) {
	vector<string> words;
	char state = '\0';
	string word;

	for (char c = '\n'; !input.eof(); input.read(&c, 1)) {
		switch (state) {
		case '\0':
			// Если символ является буквой или цифрой, добавляем его к текущему слову
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
				state = 'a';
			}
			else switch (c) {
				// Если символ является одним из: '+', '=', ';', '.', добавляем его как отдельное слово
			case '+':
			case '=':
			case ';':
			case '.':
				words.push_back(string(1, c));
				break;
			}
			break;
		case 'a':
			// Если символ является буквой или цифрой, добавляем его к текущему слову
			if (isalpha(c) || isdigit(c)) {
				word.push_back(c);
			}
			else {
				// Если достигли конца слова, добавляем его в список слов и сбрасываем текущее слово
				words.push_back(word);
				word.clear();
				state = '\0';
				switch (c) {
					// Если символ является одним из: '+', '=', ';', '.', добавляем его как отдельное слово
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
		// Если достигли конца потока, добавляем текущее слово в список слов
		words.push_back(word);
		break;
	}

	return words;
}

// Функция нахождения меток перехода
map<string, int> get_marks(vector<string> words) {
	map<string, int> marks; // Метки для перехода
	int next_line = 0; // Номер _следующего_ выражения

	char state = '\0';
	for (string word : words) {
		switch (state) {
		case '\0':
			if (word == ";")
				next_line++;
			else if (word == "mark") {
				// Ключевое слово обозначающее метку перехода
				state = 'm';
			}
			break;
		case 'm':
			// Следующее слово -- имя метки
			marks[word] = next_line;
			state = '\0';
			break;
		}
	}

	return marks;
}

// Структура операнда
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

// Структура алгоритма
struct algorithm {
	operand** strings;
	size_t* string_sizes, string_count;
	size_t mem_require, stack_size;

	algorithm(operand** strings, size_t* string_sizes, size_t string_count, size_t mem_require, size_t stack_size)
	{
		this->strings = strings;
		this->string_sizes = string_sizes;
		this->string_count = string_count;
		this->mem_require = mem_require;
		this->stack_size = stack_size;
	}
};

// Перечисление типов данных
enum type {
	int32,
	int64,
	float32,
	int8,
	uint64,
};

// Структура переменной
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

// Функция для сложения двух значений
template<typename T>
void sum(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l + *(T*)r; }

// Функция для копирования данных
template<int size>
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, size); }

// Функция для копирования вектора в динамический массив
template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

// Объединяет 2 числа в одно. Предназначено для switch/case
constexpr int64_t comb(int f, int s) {
	return ((0ll + f) << 32) + s;
}

// Функция для компиляции кода
algorithm compile(istream& input) {
	vector<string> words = read_words(input);
	map<string, int> marks = get_marks(words);
	map<string, var> vars; // Мап для хранения переменных
	map<string, pair<type, size_t>> types = { {"int", {int32, 4}}, {"long", {int64, 8}}, {"float", {float32, 4}}, {"byte", {int8, 1}}, {"ulong", {uint64, 8}} }; // Типы данных
	size_t mem_require = 0, max_stack = 0, cur_stack = 0;
	vector<operand*> algo; // Хранит строки алгоритма
	vector<size_t> str_sizes; // Хранит размеры строк
	vector<operand> str; // Хранит операнды текущей строки
	vector<type> str_types; // Хранит типы операндов текущей строки

	string var_t; // Тип текущей переменной
	int64_t hc_num; // Временное хранение числа

	char state = 'b';
	for (string word : words) {
		switch (state) {
		case 'b':
			if (word == "vars") {
				state = 't';
				break;
			}
		case 'f':
			// Если состояние 'f', значит, следующее слово - десятичная дробь (после точки)
			str.push_back(operand(new float(hc_num + stoll(word) * pow(0.1, word.size())), 'r'));
			str_types.push_back(float32);
			cur_stack++;
			state = '\0';
			break;
		case '1':
			if (word == ".") {
				// Если достигли точки, значит, следующее слово - десятичная дробь (после точки)
				state = 'f';
				break;
			}
			else {
				// Если достигли конца числа, добавляем его в список операндов
				str.push_back(operand(new int64_t(hc_num), 'r'));
				str_types.push_back(int64);
				cur_stack++;
				state = '\0';
			}
		case '\0':
			if (word == "mark") {
				// Ключевое слово, обозначающее метку для перехода (уже считана)
				state = 'm';
			}
			else if (word == "jump") {
				// Ключевое слово, обзначающее переход к метке
				state = 'j';
			}
			else if (vars.find(word) != vars.end()) {
				// Если слово - переменная, добавляем ее адрес в список операндов
				str.push_back(operand((void*)vars[word].id, 'l'));
				str_types.push_back(vars[word].t);
				cur_stack++;
			}
			else if (isdigit(word[0])) {
				// Если слово начинается с цифры, начинаем считывание числа
				hc_num = stoll(word);
				state = '1';
			}
			else switch (word[0]) {
			case '=':
				// Если слово - '=', добавляем соответствующую функцию в список операндов
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
				case uint64:
					str.push_back(operand(&set<8>, 'f', 8));
					str_types.push_back(uint64);
					break;
				}
				break;
			case '+':
				// Если слово - '+', добавляем соответствующую функцию в список операндов
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
				case uint64:
					str.push_back(operand(&sum<unsigned long long>, 'f', 8));
					str_types.push_back(uint64);
					break;
				}
				break;
			case ';':
				// Если слово - ';', заканчиваем текущую строку алгоритма
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
				// Если слово - ';', заканчиваем описание переменных
				state = '\0';
				break;
			}
			var_t = word;
			state = 'v';
			break;
		case 'v':
			// Если состояние 'v', значит, следующее слово - имя переменной
			vars[word] = var(mem_require, types[var_t].first); // Добавляем переменную в список переменных
			mem_require += types[var_t].second; // Увеличиваем требования по памяти
			state = 't';
			break;
		case 'm':
			// Здесь должно быть имя метки, которое уже считано
			state = '\0';
			break;
		case 'j':
			str.push_back(operand((void*)marks[word], 'j'));
			state = '\0';
			break;
		}
	}

	// Создаем структуру алгоритма и возвращаем ее
	return algorithm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack);
}

#define MEASURE true;
// Функция выполнения алгоритма
void execute(algorithm& algo) {
	void* data = malloc(algo.mem_require); // Память для данных
	void** stack = new void* [algo.stack_size]; // Стек операндов
	size_t stack_c;
#if MEASURE
	time_point<system_clock> start = system_clock::now();
	int count = 1e6;
	for (int _ = 0; _ < count; _++) {
#endif
		for (int i = 0; i < algo.string_count; i++) {
			stack_c = 0;
			for (int j = 0; j < algo.string_sizes[i]; j++) {
				operand* op = &algo.strings[i][j];

				void* l, * r, (*f)(void*&, void*, void*);
				switch (op->type) {
				case 'f':
					// Если тип операнда - функция, выполняем функцию над операндами на стеке
					r = stack[--stack_c];
					l = stack[--stack_c];

					f = (void(*)(void*&, void*, void*))op->func;
					f(op->value, l, r);
				case 'r':
					// Если тип операнда - значение, помещаем его на стек
					stack[stack_c++] = op->value;
					break;
				case 'l':
					// Если тип операнда - адрес, помещаем соответствующее значение из памяти на стек
					stack[stack_c++] = (byte*)data + (size_t)op->value;
					break;
				case 'j':
					// При считывании этого значения существляется переход к указанной строке
					i = (int)op->value - 1;
					j = INT_MAX;
					break;
				}
			}
		}
#if MEASURE
	}
	time_point<system_clock> end = system_clock::now();
	cout << duration_cast<nanoseconds>(end - start).count() / (double)count << "ns\n";
#endif

	delete[] stack;
	// Выводим значения из памяти в шестнадцатеричном формате
	cout << hex;
	for (byte* it = (byte*)data + algo.mem_require - 1; it + 1 != data; it--)
		cout << (int)*it << ' ';

	delete[] data;
}

int main() {
	ifstream code("code.t");
	algorithm algo = compile(code);
	execute(algo);
}
