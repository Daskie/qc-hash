#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>

#include "QMap.hpp"



using namespace qmu;



template <typename T> struct Generator;
template <> struct Generator<s08> {
	s08 v;
	void next() { ++v; }
	void reset() { v = 0; }
};
template <> struct Generator<s16> {
	s16 v;
	void next() { ++v; }
	void reset() { v = 0; }
};
template <> struct Generator<s32> {
	s32 v;
	void next() { ++v; }
	void reset() { v = 0; } };
template <> struct Generator<s64> {
	s64 v;
	void next() { ++v; }
	void reset() { v = 0; }
};
template <> struct Generator<s128> {
	s128 v;
	void next() { v = s128{ ++v.s64_1, ++v.s64_2 }; }
	void reset() { v = { 0, 0 }; }
};
template <> struct Generator<std::string> {
	unsigned char c;
	std::string v;
	void next() { ++c; v = std::string(c, c); }
	void reset() { v = std::string(); }
};

template <typename T>
double testPerformance(int n, Generator<T> g) {
	Map<int> m1;
	std::unordered_map<T, int> m2;
	std::chrono::time_point<std::chrono::high_resolution_clock> then;
	std::chrono::duration<double> dt1, dt2;
	double p, average(0.0);

	std::cout << "accessing..." << std::endl;
	std::cout << "qmu map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		m1[g.v] = i;
		g.next();
	}
	dt1 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt1.count() << std::endl;
	std::cout << "std map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		m2[g.v] = i;
		g.next();
	}
	dt2 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt2.count() << std::endl;
	p = (dt1.count() < dt2.count()) ? (dt2.count() / dt1.count()) : (-dt1.count() / dt2.count());
	if (p > 0.0)
		std::cout << p * 100.0 << "% faster" << std::endl;
	else if (p < 0.0)
		std::cout << -p * 100.0 << "% slower" << std::endl;
	else
		std::cout << "equal" << std::endl;
	std::cout << std::endl;
	average += p;

	std::cout << "at-ing..." << std::endl;
	std::cout << "qmu map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		m1.at(g.v);
		g.next();
	}
	dt1 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt1.count() << std::endl;
	std::cout << "std map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		m2.at(g.v);
		g.next();
	}
	dt2 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt2.count() << std::endl;
	p = (dt1.count() < dt2.count()) ? (dt2.count() / dt1.count()) : (-dt1.count() / dt2.count());
	if (p > 0.0)
		std::cout << p * 100.0 << "% faster" << std::endl;
	else if (p < 0.0)
		std::cout << -p * 100.0 << "% slower" << std::endl;
	else
		std::cout << "equal" << std::endl;
	std::cout << std::endl;
	average += p;

	std::cout << "iterator..." << std::endl;
	std::cout << "qmu map...";
	then = std::chrono::high_resolution_clock::now();
	for (auto it = m1.begin(); it != m1.end(); ++it) {
	}
	dt1 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt1.count() << std::endl;
	std::cout << "std map...";
	then = std::chrono::high_resolution_clock::now();
	for (auto it = m2.begin(); it != m2.end(); ++it) {
	}
	dt2 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt2.count() << std::endl;
	p = (dt1.count() < dt2.count()) ? (dt2.count() / dt1.count()) : (-dt1.count() / dt2.count());
	if (p > 0.0)
		std::cout << p * 100.0 << "% faster" << std::endl;
	else if (p < 0.0)
		std::cout << -p * 100.0 << "% slower" << std::endl;
	else
		std::cout << "equal" << std::endl;
	std::cout << std::endl;
	average += p;

	std::cout << "counting and erasing..." << std::endl;
	std::cout << "qmu map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		if (m1.count(g.v)) m1.erase(g.v);
		g.next();
	}
	dt1 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt1.count() << std::endl;
	std::cout << "std map...";
	g.reset();
	then = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < n; ++i) {
		if (m2.count(g.v)) m2.erase(g.v);
		g.next();
	}
	dt2 = std::chrono::high_resolution_clock::now() - then;
	std::cout << " " << dt2.count() << std::endl;
	p = (dt1.count() < dt2.count()) ? (dt2.count() / dt1.count()) : (-dt1.count() / dt2.count());
	if (p > 0.0)
		std::cout << p * 100.0 << "% faster" << std::endl;
	else if (p < 0.0)
		std::cout << -p * 100.0 << "% slower" << std::endl;
	else
		std::cout << "equal" << std::endl;
	std::cout << std::endl;
	average += p;

	average /= 4.0;

	return average;
}

int main() {
	double p, average(0.0);

	std::cout << "k:    s08, n: 1000000" << std::endl;
	std::cout << "----------------------------------------------" << std::endl;
	p = testPerformance<   s08>(1000000, Generator<   s08>());
	std::cout << std::endl;
	average += p;

	std::cout << "k:    s16, n:  500000" << std::endl;
	std::cout << "----------------------------------------------" << std::endl;
	p = testPerformance<   s16>( 500000, Generator<   s16>());
	std::cout << std::endl;
	average += p;

	std::cout << "k:    s32, n:  250000" << std::endl;
	std::cout << "----------------------------------------------" << std::endl;
	p = testPerformance<   s32>( 250000, Generator<   s32>());
	std::cout << std::endl;
	average += p;

	std::cout << "k:    s64, n:  125000" << std::endl;
	std::cout << "----------------------------------------------" << std::endl;
	p = testPerformance<   s64>( 125000, Generator<   s64>());
	std::cout << std::endl;
	average += p;

	std::cout << "k: string, n: 100000" << std::endl;
	std::cout << "----------------------------------------------" << std::endl;
	p = testPerformance<std::string>( 100000, Generator<std::string>());
	std::cout << std::endl;
	average += p;

	average /= 5.0;

	std::cout << std::endl;

	std::cout << "Overall: ";
	if (average > 0.0)
		std::cout << average * 100 << "% faster";
	else if (average < 0.0)
		std::cout << -average * 100 << "% slower";
	else
		std::cout << "even";
	std::cout << std::endl;

	std::cin.get();
	return 0;
}