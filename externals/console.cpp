#include<stdio.h>

#define export __declspec(dllexport) __stdcall

extern "C" {
	// Funcitons, same interface as unary operations have
	void export print_int(void*& buf, void* l) { printf("%d", *(int*)l); }
	void export read_int(void*& buf, void* l) { scanf_s("%d", (int*)buf); }
	void export print_char(void*& buf, void* l) { printf("%c", *(char*)l); }
	void export read_char(void*& buf, void* l) { *(char*)buf = getchar(); }
	// Number of exporting funcitons
	int export allc = 4;

	// Exporting functions names
	void export alln(int i, char*& buf) {
		switch (i) {
		case 0:
			buf = (char*)"print_int";
			break;
		case 1:
			buf = (char*)"read_int";
			break;
		case 2:
			buf = (char*)"print_char";
			break;
		case 3:
			buf = (char*)"read_char";
			break;
		}
	}

	// Exporting functions result types
	void export allt(int i, char*& buf) {
		switch (i) {
		case 0:
			buf = (char*)"blank";
			break;
		case 1:
			buf = (char*)"int";
			break;
		case 2:
			buf = (char*)"blank";
			break;
		case 3:
			buf = (char*)"char";
			break;
		}
	}
}