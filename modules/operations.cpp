#include<stdlib.h>
#include<bitset>

#define byte std::byte
struct algorithm;
void execute(algorithm*, void*&, void**);

// Functions that are used for basic operations
template<typename T>
void sum(void*& buf, void** args) { *(T*)buf = *(T*)(args[0]) + *(T*)(args[1]); } // l + r

template<typename T>
void diff(void*& buf, void** args) { *(T*)buf = *(T*)args[0] - *(T*)args[1]; } // l - r

template<typename T>
void div(void*& buf, void** args) {*(T*)buf = *(T*)args[0] / *(T*)args[1]; } // l / r

template<int size>
void set(void*& buf, void** args) { buf = memcpy(args[0], args[1], size); } // l = r

template<int size>
void equal(void*& buf, void** args) { *(bool*)buf = memcmp(args[0], args[1], size) == 0; } // l == r

void call(void*& buf, void** args) { execute((algorithm*)args[0], buf, args + 1); } // l(r)

template<typename T>
void create(void*& buf, void** args) { *(void**)buf = malloc(*(T*)args[0]); } // malloc(l)

void destroy(void*& buf, void** args) { free(*(void**)args[0]); } // free(l)

void from(void*& buf, void** args) { buf = *(void**)args[0];  } // *l

void addr(void*& buf, void** args) { *(void**)buf = args[0]; } // &l

template<typename T>
void shift(void*& buf, void** args) { buf = (byte*)args[0] + (T)args[1]; } // l[r]

template<typename T1, typename T2>
void convert(void*& buf, void** args) { *(T2*)buf = *(T1*)args[0]; } // type(l)
