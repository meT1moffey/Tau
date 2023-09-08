#include"precompiling.cpp"
#include"operations.cpp"

// Parse functions and compile them
algorithm compile_function(func_info info, map<string, var> globals, map<string, struct_info> structs, map<string, var> externs) {
	map<string, int> marks = get_marks(info.words);
	map<string, var> vars;
	size_t mem_require = 0, max_stack = 0, cur_stack = 0, cur_stack_require = 0, max_stack_require = 0;
	vector<operand*> algo;
	vector<size_t> str_sizes;
	vector<operand> str;
	vector<full_type> str_types;

	full_type buf_t;

	string text = "", buf_name;

	char state = '\0';
	
	for (string word : info.words) {
		switch (state) {
		case '\0':
			// Default state, default read
			if (word == "var") {
				// This expression contains variables initialisation
				state = 't';
				break;
			}
			else if (word == "mark") {
				// Keyword for defining mark
				state = 'm';
			}
			else if (word == "jump") {
				// Keyword for goto some mark
				state = 'j';
			}
			else if (word == "log") {
				// Keyword to print all algo data (used only for debug)
				str.push_back(operand(nullptr, -1));
			}
			else if (word == "create") {
				// Operand, that is equal to "new" (currently works as malloc())
				switch (str_types[str_types.size() - 1].t) {
				case int32:
					str.push_back(operand(&create<int>, cur_stack_require, PTR_SIZE));
					str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += PTR_SIZE;
					break;
				}
			}
			else if (word == "destroy") {
				// Operator that is equal to "delete" (currently works as free())
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
				// Special varibale that contains output of function
				str.push_back(operand(nullptr, 'r'));
				str_types.push_back(info.result_t);
				cur_stack++;
			}
			else if (word == "object") {
				// This expression contains object initialisation
				state = 'o';
			}
			else if(word[0] == '\'') {
				str.push_back(operand(new char(word[1]), 'g'));
				str_types.push_back(char_t);
				cur_stack++;
			}
			else if(key_exists(types, word)) {
				if(str_types[str_types.size() - 1].t == types[word])
					break;
				
				switch (comb(str_types[str_types.size() - 1].t, types[word])) {
				case comb(int32, int64):
					str.push_back(operand(&convert<int, int64_t>, cur_stack_require, 8));
					str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(int32, float32):
					str.push_back(operand(&convert<int, float>, cur_stack_require, 4));
					str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int64, int32):
					str.push_back(operand(&convert<int64_t, int>, cur_stack_require, 4));
					str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(int64, float32):
					str.push_back(operand(&convert<int, float>, cur_stack_require, 4));
					str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(float32, int32):
					str.push_back(operand(&convert<float, int>, cur_stack_require, 4));
					str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(float32, int64):
					str.push_back(operand(&convert<float, int64_t>, cur_stack_require, 8));
					str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(char_t, int32):
					str.push_back(operand(&convert<char, int>, cur_stack_require, 4));
					str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(char_t, int64):
					str.push_back(operand(&convert<char, int64_t>, cur_stack_require, 8));
					str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				}
				break;
			}
			else if (vars.find(word) != vars.end()) {
				// Adding variable with the same name as word
				str.push_back(operand(vars[word].pos, 'l'));
				str_types.push_back(vars[word].t);
				cur_stack++;
			}
			else if (info.args.find(word) != info.args.end()) {
				// Adding argument with the same name as word
				str.push_back(operand(info.args[word].pos, 'a'));
				str_types.push_back(info.args[word].t);
				cur_stack++;
			}
			else if (globals.find(word) != globals.end()) {
				// Adding global variable with the same name as word
				str.push_back(operand(globals[word].pos, 'g'));
				str_types.push_back(globals[word].t);
				cur_stack++;
			}
			else if(key_exists(externs, word)) {
				str.push_back(operand((void(*)(void*&, void*))externs[word].pos, cur_stack_require, t_sizes[externs[word].t.args[0].t]));
				str_types.pop_back();
				str_types.push_back(externs[word].t.args[0].t);
				cur_stack_require += t_sizes[externs[word].t.args[0].t];
				break;
			}
			else if (isdigit(word[0])) {
				// Adding word as a number
				if (word.find('.') != string::npos) {
					// Number has floating point
					str.push_back(operand(new float(stof(word)), 'g'));
					str_types.push_back(float32);
					cur_stack++;
				}
				else {
					// Number doesn't has floating point
					str.push_back(operand(new int(stoi(word)), 'g'));
					str_types.push_back(int32);
					cur_stack++;
				}
			}
			else switch (word[0]) {
				// Reading word as operand
			case '=':
				if(word.size() == 1)
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
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
					str.push_back(operand(&set<4>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
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
				case comb(char_t, char_t):
					str.push_back(operand(&set<1>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(char_t);
					cur_stack_require += 1;
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
					// "==" operand
					switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
					case comb(int32, int32):
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
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
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
					str.push_back(operand(&sum<float>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
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
				case comb(ptr, int32):
					str.push_back(operand(&sum<int>, cur_stack_require, 4));
					buf_t = str_types[str_types.size() - 2];
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(buf_t);
					cur_stack_require += 8;
					break;
				}
				break;
			case '-':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
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
					str.push_back(operand(&diff<float>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
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
				case comb(ptr, int32):
					str.push_back(operand(&diff<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(ptr);
					cur_stack_require += 8;
					break;
				}
				break;
			case '/':
				switch (comb(str_types[str_types.size() - 2].t, str_types[str_types.size() - 1].t)) {
				case comb(int32, int32):
					str.push_back(operand(&div<int>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int32);
					cur_stack_require += 4;
					break;
				case comb(int64, int64):
					str.push_back(operand(&div<int64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int64);
					cur_stack_require += 8;
					break;
				case comb(float32, float32):
					str.push_back(operand(&div<float>, cur_stack_require, 4));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float32);
					cur_stack_require += 4;
					break;
				case comb(int8, int8):
					str.push_back(operand(&div<int8_t>, cur_stack_require, 1));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(int8);
					cur_stack_require += 1;
					break;
				case comb(uint64, uint64):
					str.push_back(operand(&div<uint64_t>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(uint64);
					cur_stack_require += 8;
					break;
				case comb(float64, float64):
					str.push_back(operand(&div<double>, cur_stack_require, 8));
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(float64);
					cur_stack_require += 8;
					break;
				}
				break;
			case '*':
				// Currently it only works as reading value from ptr, not as multiplying
				switch (str_types[str_types.size() - 1].t) {
				case ptr:
					buf_t = str_types[str_types.size() - 1].args[0];
					str.push_back(operand(&from, cur_stack_require, 1));
					str_types.pop_back();
					str_types.push_back(buf_t);
					cur_stack_require += buf_t.t == object ?
						structs[buf_t.name].size : t_sizes[buf_t.t];
					break;
				}
				break;
			case '&':
				buf_t = str_types[str_types.size() - 1];
				str.push_back(operand(&addr, cur_stack_require, PTR_SIZE));
				str_types.pop_back();
				str_types.push_back(full_type(ptr, { buf_t }));
				cur_stack_require += PTR_SIZE;
				break;
			case '@':
				// Call operand (will be collapsed into () one day)
				if(str_types[str_types.size() - 2].t == func) {
					str.push_back(operand(&call, cur_stack_require, 0));
					buf_t = str_types[str_types.size() - 2].args[0];
					str_types.pop_back(); str_types.pop_back();
					str_types.push_back(buf_t);
					cur_stack_require += buf_t.t == object ?
						structs[buf_t.name].size : t_sizes[buf_t.t];
				}
				break;
			case '.':
				// Next word is property name.
				// This operator is infix, because property name can't be handled corrently without stucture that owns it
				state = 'i';
				break;
			case '?':
				// Branching operator. If first byte of last value in stack is 0 expression breaking
				str.push_back(operand(nullptr, 'c'));
				break;
			case ';':
				// End of expression, obviously
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
			// Reading current variable type
			if (word == ";") {
				state = '\0';
				break;
			}
			if (key_exists(types, word)) {
				buf_t = types[word];
			}
			else if(key_exists(structs, word)) {
				buf_t = word;
			}
			state = 'v';
			break;
		case 'v':
			// Reading name of current variable
			if (word == "*") {
				// "type*" is ptr to type
				buf_t = full_type(ptr, { buf_t });
			}
			else {
				vars[word] = var((void*)mem_require, buf_t);
				mem_require += buf_t.t == object ?
						structs[buf_t.name].size : t_sizes[buf_t.t];
				state = 't';
			}
			break;
		case 'o':
			// Reading current object name
			buf_name = word;
			vars[buf_name] = var((void*)mem_require, object);
			state = 'l';
			break;
		case 'l':
			// Reading params types
			if (word == ";") {
				state = '\0';
				break;
			}
			vars[buf_name].t.args.push_back(types[word]);
			mem_require += buf_t.t == object ?
						structs[buf_t.name].size : t_sizes[buf_t.t];
			break;
		case 'm':
			// Reading mark name (was handled in get_marks())
			state = '\0';
			break;
		case 'j':
			// Reading name of mark to jump
			str.push_back(operand((void*)marks[word], 'j'));
			str_types.push_back(blank);
			state = '\0';
			break;
		case 'i':
			// Reading property name and convert it to the local position
			buf_name = str_types.back().name;
			buf_t = structs[buf_name].fields[word].t;
			str.push_back(operand(structs[buf_name].fields[word].pos, 'g'));
			str.push_back(operand(&shift<int64_t>, cur_stack_require, 0));
			str_types.pop_back();
			str_types.push_back(buf_t);
			state = '\0';
			break;
		}
	}

	return algorithm(copy_data(algo), copy_data(str_sizes), algo.size(), mem_require, max_stack_require, max_stack);
}
