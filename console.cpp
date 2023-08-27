// VS Code can't make dlls, so code needs to be copied to VS during development

#include"pch.h" // I have no idea why this must be here, but it does
#include<iostream>

#define export __declspec(dllexport) __stdcall

extern "C" {
	// Funcitons, same interface as unary operations have
	void export print_int(void*& buf, void* l) { std::cout << *(int*)l; }
	void export read_int(void*& buf, void* l) { std::cin >> *(int*)buf; }

	// Number of exporting funcitons
	int export allc = 2;

	// Exporting functions names
	void export alln(int i, char*& buf) {
		switch (i) {
		case 0:
			buf = (char*)"print_int";
			break;
		case 1:
			buf = (char*)"read_int";
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
		}
	}
}