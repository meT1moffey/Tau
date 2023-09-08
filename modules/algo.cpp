#include<map>
#include<vector>

using namespace std;

// Copy vector into dynamic array
template<typename T>
T* copy_data(vector<T> v) {
	return (T*)memcpy(new T[v.size()], v.data(), v.size() * sizeof(T));
}

// Combines 2 intergers for switch/case
constexpr long long comb(int f, int s) {
	return ((0ll + f) << 32) + s;
}

template<typename K, typename V>
bool key_exists(map<K, V> m, K key) { return m.find(key) != m.end(); }
