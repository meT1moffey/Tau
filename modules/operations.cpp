#include<stdlib.h>
#include<bitset>

#define byte std::byte
struct algorithm;
void execute(algorithm*, void*&, byte*);

// Functions that are used for basic operations
template<typename T>
void sum(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l + *(T*)r; } // l + r

template<typename T>
void diff(void*& buf, void* l, void* r) { *(T*)buf = *(T*)l - *(T*)r; } // l - r

template<typename T>
void div(void*& buf, void* l, void* r) {*(T*)buf = *(T*)l / *(T*)r; } // l / r

template<int size>
void set(void*& buf, void* l, void* r) { buf = memcpy(l, r, size); } // l = r

template<int size>
void equal(void*& buf, void* l, void* r) { *(bool*)buf = memcmp(l, r, size) == 0; } // l == r

void call(void*& buf, void* l, void* r) { execute((algorithm*)l, buf, (byte*)r); } // l(r)

template<typename T>
void create(void*& buf, void* l) { *(void**)buf = malloc(*(T*)l); } // malloc(l)

void destroy(void*& buf, void* l) { free(*(void**)l); } // free(l)

void from(void*& buf, void* l) { buf = *(void**)l;  } // *l

void addr(void*& buf, void* l) { *(void**)buf = l; } // &l

template<typename T>
void shift(void*& buf, void* l, void* r) { buf = (byte*)l + (T)r; } // l[r]

template<typename T1, typename T2>
void convert(void*& buf, void* l) { *(T2*)buf = *(T1*)l; } // type(l)
