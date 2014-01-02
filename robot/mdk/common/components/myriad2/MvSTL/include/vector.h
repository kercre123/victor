#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "mvstl_types.h"
#include "mvstl_utils.h"
#include "alloc.h" //Mircea's malloc and mv_free

#ifdef __MOVICOMPILE__
#include <stddef.h>
#endif

namespace mvstl
{

//#define vector sve_vector

#define DEFAULT_CAPACITY (10)

template<typename T>
class vector 
{
private:
	u32 capacity;
	T* _data;
	size_t last_idx;

public:
	vector();
	vector(size_t _capacity);
    vector(size_t _capacity, T initVal);
	~vector();
	size_t size() const;
	void push_back(T* val);
	void push_back(T val);
	T& pop_back();
	void erase(size_t idx);
	void erase(u32 first, u32 last);
	void assign(vector<T>& src, u32 first, u32 last);
	void shrink_to_fit();
	u32 begin() const;
	u32 end() const;
	void clear();
	bool empty() const;
	T* data() const;
	T& at(size_t idx);
    void resize(size_t n = 0);
    void resize(size_t n, int x);
	T& operator[] (size_t idx);

private:
	void expand();
};

template<typename T> inline
	vector<T>::vector() : capacity(DEFAULT_CAPACITY)
{
	_data = (T*)mv_malloc(sizeof(T) * capacity);
	assert(_data != NULL);
	last_idx = 0;
}

template<typename T> inline
	vector<T>::vector(size_t _capacity) : capacity(_capacity)
{
	_data = (T*)mv_malloc(sizeof(T) * capacity);
	assert(_data != NULL);
	last_idx = 0;
}

template<typename T> inline
	vector<T>::vector(size_t _capacity, T initVal) : capacity(_capacity)
{
	_data = (T*)mv_malloc(sizeof(T) * capacity);
	assert(_data != NULL);
	last_idx = 0;

	for (; last_idx < capacity; last_idx++)
		_data[last_idx] = initVal;
}

template<typename T> inline
	vector<T>::~vector()
{
	//for (int i = 0; i < last_idx; i++)
		mv_free(_data);		
	last_idx = 0;
}

template<typename T> inline size_t
	vector<T>::size() const 
{
	return last_idx;
}

template<typename T> inline void
	vector<T>::expand()
{
	u32 current_size = sizeof(T) * capacity;
	T* tmp = (T*)mv_realloc(_data, current_size + current_size * 1/2);
	assert(tmp != NULL);
	_data = tmp;
	capacity = capacity + capacity * 1/2;
}

template<typename T> void 
	vector<T>::push_back(T* val) 
{		
	if (last_idx == capacity)
		expand();

	_data[last_idx] = *val;
	last_idx++;
}

template<typename T> inline void
	vector<T>::push_back(T val) 
{		
	if (last_idx == capacity)
		expand();

	_data[last_idx] = val;
	last_idx++;
}

template<typename T> inline T&
	vector<T>::pop_back() 
{		
	if (last_idx > 0)
		return _data[--last_idx];
}

template<typename T> void 
	vector<T>::assign(vector<T>& src, u32 first, u32 last)
{
	mv_free(_data);
	size_t n = last - first;
	_data = (T*)mv_malloc(sizeof(T) * n);
	assert(_data != NULL);
	last_idx = 0;

	for (; last_idx < n; last_idx++, first++)
		_data[last_idx] = src[first];
}

template<typename T> void 
	vector<T>::erase(size_t idx)
{
	assert(idx >= 0 && idx < last_idx);
	for (u32 i = idx; i < last_idx - 1; i++)
		_data[i] = _data[i + 1];
	last_idx--;
}

template<typename T> void
	vector<T>::erase(u32 first, u32 last)
{
	assert(first <= last && first >= 0 && last >= 0 && first <= capacity && last <= capacity);
	int step = last - first;
	for (u32 i = first; i < last_idx - step; i++)
		_data[i] = _data[i + step];
	last_idx -= step;
}

template<typename T> void
	vector<T>::shrink_to_fit()
{
	//TBD: reallocate a smaller array and copy over all elements
	//Note: not necessary yet
}

template<typename T> void
    vector<T>::resize(size_t n)
{
    if (n < capacity)
        erase(begin() + n, end());
    else if (n > capacity)
        expand();
}

template<typename T> void
    vector<T>::resize(size_t n, int x)
{
    //TODO:
   
}

template<typename T> inline void 
	vector<T>::clear()
{
	last_idx = 0;
}

template<typename T> inline u32
	vector<T>::begin() const
{
	return 0;
}

template<typename T> inline u32
	vector<T>::end() const
{
	return last_idx;
}

template<typename T> inline bool
	vector<T>::empty() const
{
	return (last_idx == 0);
}

template<typename T> inline T*
	vector<T>::data() const
{
	return _data;
}

template<typename T> inline T& 
	vector<T>::at(size_t idx)
{
	assert(idx >= 0 && idx < last_idx);
	return _data[idx];
}

template<typename T> inline T& 
	vector<T>::operator[] ( size_t idx)
{
	assert(idx >= 0 && idx < last_idx);
	return _data[idx];
}

}

#endif
