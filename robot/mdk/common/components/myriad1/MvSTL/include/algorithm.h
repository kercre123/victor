#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

#include "vector.h"
#include <assert.h>

namespace mvstl
{
// Bubblesort indices by ascending value
template<typename A, typename B> inline void
bubble_sort(vector<A>& values, vector<B>& indices)
{
	int sorted = 0;
	B tmp;
	int n = values.size() - 1;
	
	do {
		sorted = 1;
		for (int i = 0; i < n; i++)
			if (values[indices[i]] > values[indices[i + 1]]) {
				sorted = 0;
				tmp = indices[i];
				indices[i] = indices[i + 1];
				indices[i + 1] = tmp;
			}
	} while (sorted == 0);
}

// Bubblesort by ascending values
template<typename T> inline void
bubble_sort(vector<T>& values)
{
	int sorted = 0;
	T tmp;
	int n = values.size() - 1;

	do {
		sorted = 1;
		for (int i = 0; i < n; i++)
			if (values[i] > values[i + 1]) {
				sorted = 0;
				tmp = values[i];
				values[i] = values[i + 1];
				values[i + 1] = tmp;				
			}
	} while (sorted == 0);
}

template<typename T> inline void
bubble_sort(vector<T>& values, u32 first, u32 last)
{
	int sorted = 0;
	T tmp;
	
	assert(first >= 0 && last > 0 && last > first);

	do {
		sorted = 1;
		for (int i = first; i < last; i++)
			if (values[i] > values[i + 1]) {
				sorted = 0;
				tmp = values[i];
				values[i] = values[i + 1];
				values[i + 1] = tmp;				
			}
	} while (sorted == 0);
}
}

#endif
