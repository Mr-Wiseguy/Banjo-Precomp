/**********************************************************
 * n64_span.h
 **********************************************************
 * N64 Span:
 *     This class represents a span of big endian memory.
 *     Think of it as a "view" of a memory section.
 *     Does not convey any ownership of the object
 * Author: Mittenz 5/15/2020
 **********************************************************/
#pragma once
#include <type_traits>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdint>

//big endian helper struct
template<typename T> struct n64_be;

class n64_span
{
public:
	n64_span() = default;
	n64_span(const n64_span& src)
		:_begin(src._begin)
		, _end(src._end){}
	n64_span(uint8_t* const begin, const size_t size) 
		: _begin(begin)
		, _end(begin + size) {}

	inline uint8_t* const begin(void) const { return _begin; }
	inline uint8_t* const end(void) const { return _end; }
	inline const size_t size(void) const { return _end - _begin; }
	
	const uint8_t& operator[](int index) const { if (index >= size())exit(0); return _begin[index]; }
	uint8_t& operator[](int index) { if (index >= size())exit(0); return _begin[index]; }
 
	n64_span operator()(uint32_t i) const{
    	return slice(i, size() - i);
	}

	template <typename T>
	inline operator T() const{ return get<T>();}

	n64_span& operator=(const n64_span& other) {
		if (this != &other) {
			_begin = other._begin;
			_end = other._end;
		}
		return *this;
	}

    inline bool operator==(const n64_span& rhs) const {
        return (size() != rhs.size())
            ? false 
            : std::equal(begin(), end(), rhs.begin());
	}

    inline bool operator!=(const n64_span& rhs) const{
        return !operator==(rhs);
	}

	inline const n64_span slice(uint32_t offset, size_t size = -1) const{
		return n64_span(_begin + offset, size);
	}

	//return fundemental types from span
	template <typename T> 
	inline T get(uint32_t offset = 0) const {
		return n64_be<T>::get(*this, offset);
	}

	//return fundemental types from span and advances the offset to next value
	template <typename T> 
	inline T seq_get(uint32_t& offset) const {
		T ret_val = n64_be<T>::get(*this, offset);
		offset += sizeof(T);
		return ret_val;
	}

	template <typename T>
	std::vector<T> to_vector(void) const {
		size_t elem_size = sizeof(T);
		uint32_t elem_cnt = size()/elem_size;

		std::vector<uint32_t> indx(elem_cnt);
		std::iota(indx.begin(),indx.end(),0);

		std::vector<T> ret_val(elem_cnt);

		std::transform( indx.begin(), indx.end(), ret_val.begin()
		, [&](uint32_t i)-> T { return this->get<T>(i*elem_size); }
		);

		return ret_val;
	}

	template <typename T>
	inline std::vector<T> to_vector(uint32_t offset) const {
		return slice(offset, size()-offset).to_vector<T>();

	}

	template <typename T>
	inline std::vector<T> to_vector(uint32_t offset, uint32_t count) const {
		return slice(offset, count * sizeof(T)).to_vector<T>();

	}

	//write fundemental types to span
	template <typename T> void set(T value, uint32_t offset = 0) {
		if (!std::is_fundamental<T>::value)
			return;
		uint8_t size = sizeof(T);
		uint8_t* start = begin() + offset;
		T tmp = value;
		while (size) {
			size--;
			start[size] = (uint8_t) (tmp & 0x00FF);
			tmp = tmp >> 8;
		}
		return;
	}

	template <typename T> void seq_set(T value, uint32_t& offset){
		set(value, offset);
		offset += sizeof(T);
		return;
	}


private:
	uint8_t* _begin = nullptr;
	uint8_t* _end = nullptr;
};

template<typename T> 
struct n64_be{
	static T get(const n64_span& span, uint32_t offset) {
		if (std::is_fundamental<T>::value) {
			uint8_t size = sizeof(T);
			uint8_t count = 1;
			uint8_t* start = span.begin() + offset;
			T value = (T)(start[0]);
			while (count < size) {
				value = value << 8;
				value |= (T)start[count];
				count++;
			}
			return value;
		}
		return 0;
	}
};
/*
Copyright (c) 2020 Michael "Mittenz" Salino-Hugg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/