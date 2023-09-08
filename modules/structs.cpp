#include<vector>
#include<string>
#include<map>

using namespace std;

// Operand. Expression unit
struct operand {
	void* value = nullptr; // ptr to value (local or global, depending on type)
	size_t size = 0, pos = -1;
	char type = 'g'; // detailed types explaination in execute()

	operand() {}
	operand(void* value, char type) {
		this->value = value;
		this->type = type;
	}
	operand(void(value)(void*&, void*, void*), size_t pos, size_t size) {
		this->value = (void*)value;
		this->type = 'f';
		this->pos = pos;
		this->size = size;
	}
	operand(void(value)(void*&, void*), size_t pos, size_t size) {
		this->value = (void*)value;
		this->type = 'u';
		this->pos = pos;
		this->size = size;
	}
};

// Algorithm aka function aka array of expressions (arrays of oprands)
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

// Non generic standart types
enum type {
	blank, // aka void
	func,
	int8, // aka char
	int32,
	int64,
	uint64,
	float32,
	float64,
	char_t,
	ptr,
	object
};

// Type with all arguments-types
struct full_type {
	type t = blank;
	vector<full_type> args;
	string name = ""; // used only for structs

	full_type() {}
	full_type(type t) : t(t) {}
	full_type(type t, vector<full_type> args) : t(t), args(args) {}
	full_type(string name) : name(name), t(object) {}
};

// Variable data
struct var {
	void* pos = nullptr;
	full_type t = blank;

	var() {}
	var(void* pos, full_type t) : pos(pos), t(t) {}
};

// Function data
struct func_info {
	string name;
	vector<string> words;
	full_type result_t;
	map<string, var> args;

	func_info() {}
	func_info(string name, vector<string> words, full_type result_t, map<string, var> args)
		: name(name), words(words), result_t(result_t), args(args) {}
};

// Struct data
struct struct_info {
	map<string, var> fields;
	size_t size = 0;
	
	struct_info() {}
	struct_info(map<string, var> fields, size_t size) : fields(fields), size(size) {}
};

