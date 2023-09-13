#include<fstream>
#include<sstream>
#include<windows.h>

#include"structs.cpp"
#include"algo.cpp"

// Ptr size diffences on systems
const int PTR_SIZE = sizeof(void*);

algorithm compile_function(func_info, map<string, var>, map<string, struct_info>, map<string, var>);

// Parse file into vector of words (tokens)
vector<string> read_words(istream& input) {
	vector<string> words;
	char state = '\0';
	string word;

	char c;
	for (c = '\n'; !input.eof(); input.read(&c, 1)) {
		switch (state) {
		case 'o':
			// Reading operator 
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
			// Reading identifier or number
			if (isalpha(c) || isdigit(c) || c == '_') {
				word.push_back(c);
				break;
			}
			if (c == '.' && isdigit(word.back())) {
				word.push_back(c);
				break;
			}
		newWord:
			// Save current word
			words.push_back(word);
			word.clear();
			state = '\0';
		case '\0':
			// Reading new word
			if (isalpha(c) || isdigit(c) || c == '_') {
				word.push_back(c);
				state = 'a';
			}
			else switch (c) {
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
			case '/':
			case ',':
				// These symbols are always single word
				words.push_back(string(1, c));
				break;
			case '=':
				// These symbols (currently only this one) can be the beginning of non identifier word
				state = 'o';
				word.push_back(c);
				break;
			case '\'':
				// Start of char
				state = 'c';
				word.push_back(c);
				break;
			}
			break;
		case 'c':
			if(c == '\'') {
				state = '\0';
				words.push_back(word);
				word.clear();
			}
			else if(c == '\\') {
				state = 'C';
			}
			else {
				word.push_back(c);
			}
			break;
		case 'C':
			switch(c) {
			case 'n':
				word.push_back('\n');
				break;
			case '0':
				word.push_back('\0');
				break;
			case '\'':
				word.push_back('\'');
				break;
			case '\\':
				word.push_back('\\');
				break;
			}
			state = 'c';
			break;
		}
	}

	// Forced saving last word
	if (c != '\n') {
		c = '\n';
		goto newWord;
	}

	return words;
}

// Parse marks
map<string, int> get_marks(vector<string> words) {
	map<string, int> marks;
	int next_line = 0;

	char state = '\0';
	for (string word : words) {
		switch (state) {
		case '\0':
			// Default state
			if (word == ";")
				next_line++;
			else if (word == "mark") {
				// Key word that defines mark
				state = 'm';
			}
			else if (word == "var" || word == "object") {
				// "var" and "object" expressions are fake, so they must be skipped
				state = 'v';
			}
			break;
		case 'v':
			// Reading fake expression
			if (word == ";")
				state = '\0';
			break;
		case 'm':
			// Reading mark name
			marks[word] = next_line;
			state = '\0';
			break;
		}
	}

	return marks;
}

// Key words for every standart non generic type
map<string, type> types = {
	{"blank", blank},
	{"byte", int8},
	{"int", int32},
	{"long", int64},
	{"ulong", uint64},
	{"float", float32},
	{"double", float64},
	{"char", char_t}
};

// Size of every standart non generic type
map<type, size_t> t_sizes = {
	{blank, 0},
	{int8, 1},
	{int32, 4},
	{int64, 8},
	{uint64, 8},
	{float32, 4},
	{float64, 8},
	{char_t, 1},
	{ptr, PTR_SIZE}
};

map<string, var> parse_loads(vector<string> words) {
	map<string, var> externs;

	char state = '\0';
	for(string word : words) {
		switch(state) {
		case '\0':
			if(word == "load") {
				state = 'l';
			}
			else {
				return externs;
			}
			break;
		case 'l':
			// Imports all functions from dll
			string dll_name = "libs\\" + word + ".dll";
			HINSTANCE dll = LoadLibraryA(dll_name.c_str());

			auto allc = (int(__stdcall*))GetProcAddress(dll, "allc");
			auto alln = (void(__stdcall*)(int, char*&))GetProcAddress(dll, "alln");
			auto allt = (void(__stdcall*)(int, char*&))GetProcAddress(dll, "allt");
			for(int i = 0; i < *allc; i++) {
				char* n, *t;
				alln(i, n);
				allt(i, t);
				vector<full_type> args;
				istringstream in(t);
				for(string s; in >> s;)
					args.push_back(types[s]);
				
				auto fn = (void(__stdcall*)(void*&, void**))GetProcAddress(dll, n);
				externs[n] = var((void*)fn, full_type(func, args));
			}

			state = '\0';
			break;
		}
	}

	return externs;
}

// Compiles function algorithm
map<string, var> parse_functions(vector<string> words) {
	vector<func_info> parsed;
	map<string, var> globals;
	map<string, var> externs = parse_loads(words);
	map<string, struct_info> structs;

	string name;
	full_type buf_type, result_type;
	vector<string> func_words;
	map<string, var> args;
	vector<full_type> argt;
	size_t argl = 0;
	char state = '\0';

	for (string word : words) {
		switch (state) {
		case '\0':
			// Reading new function / variable
			if (word == "func") {
				state = 'r';
			}
			else if (word == "var") {
				state = 't';
			}
			else if (word == "struct") {
				state = 's';
			}
			else if(word == "load") {
				state = 'd';
			}
			break;
		case 'r':
			// Reading result type of current funciton
			if(key_exists(types, word)) {
				result_type = types[word];
			}
			else if(key_exists(structs, word)) {
				result_type = word;
			}
			argt.push_back(result_type);
			state = 'f';
			break;
		case 'f':
			// Reading name of function
			name = word;
			state = 'o';
			break;
		case 'v':
			// Reading name of variable
			globals[word] = var(malloc(buf_type.t == object ?
				structs[buf_type.name].size : t_sizes[buf_type.t]), buf_type);
			state = 't';
			break;
		case 't':
			// Reading type of variable
			if (word == ";") {
				state = '\0';
				break;
			}
			if(key_exists(types, word)) {
				buf_type = types[word];
			}
			else if(key_exists(structs, word)) {
				buf_type = word;
			}
			state = 'v';
			break;
		case 'o':
			// Reading argument type
			if (word == "{") {
				state = 'b';
				break;
			}
			if(key_exists(types, word)) {
				buf_type = types[word];
			}
			else if(key_exists(structs, word)) {
				buf_type = word;
			}
			argt.push_back(buf_type);
			state = 'a';
			break;
		case 'a':
			// Reading argument name
			args[word] = var((void*)argl, buf_type);
			argl++;
			state = 'o';
			break;
		case 'b':
			// Reading word inside funciton
			if (word == "}") {
				parsed.push_back(func_info(name, func_words, result_type, args));
				globals[name] = var(new algorithm, full_type(func, argt));
				func_words.clear();
				args.clear();
				argl = 0;
				state = '\0';
			}
			else {
				func_words.push_back(word);
			}
			break;
		case 's':
			// Reading struct name
			name = word;
			state = 'p';
			break;
		case 'p':
			// Reading struct field type
			if (word == ";") {
				structs[name] = struct_info(args, argl);
				args.clear();
				argl = 0;
				state = '\0';
				break;
			}
			if(key_exists(types, word)) {
				buf_type = types[word];
			}
			else if(key_exists(structs, word)) {
				buf_type = word;
			}
			state = 'l';
			break;
		case 'l':
			// Reading struct field name
			args[word] = var((void*)argl, buf_type);
			argl += buf_type.t == object ?
				structs[buf_type.name].size : t_sizes[buf_type.t];
			state = 'p';
			break;
		case 'd':
			// Imports all functions from dll (already done in parse_loaders())
			state = '\0';
			break;
		}
	}

	for (auto fun : parsed)
		*(algorithm*)(globals[fun.name].pos) = compile_function(fun, globals, structs, externs);

	return map<string, var>(globals);
}

