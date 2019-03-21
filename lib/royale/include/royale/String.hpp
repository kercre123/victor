/****************************************************************************\
* Copyright (C) 2015 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#pragma once

#include <royale/Definitions.hpp>
#include <royale/Iterator.hpp>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <stdint.h>

namespace royale
{
    template<typename T>
    typename std::enable_if <
    std::is_same<T, wchar_t>::value,
        size_t
        >::type royale_strlen (const wchar_t *string)
    {
        if (string == nullptr)
        {
            return 0;
        }
        return wcslen (string);
    }

    template<typename T>
    typename std::enable_if <
    std::is_same<T, char>::value,
        size_t
        >::type royale_strlen (const char *string)
    {
        if (string == nullptr)
        {
            return 0;
        }
        return strlen (string);
    }

    template<typename T>
    size_t royale_strnlen (const T *string, std::size_t len)
    {
        if (string == nullptr)
        {
            return 0;
        }
        for (auto i = 0u; i < len; i++)
        {
            if (!string [i])
            {
                return i;
            }
        }
        return len;
    }

    // should probably evaluate for more types of characters (UTF8/UTF16 and wide chars)
    template <class T>
    class basicString
    {
        // at the moment we only support char; wchar_t might work but is untested!
        static_assert ( (std::is_same<T, char>::value || std::is_same<T, wchar_t>::value), "basicString can only manage char and wchar_t!");

    public:
        /**
        * Iterator definitions
        */
        typedef royale::iterator::royale_iterator<std::random_access_iterator_tag, T> iterator;
        typedef royale::iterator::royale_const_iterator<std::random_access_iterator_tag, T> const_iterator;
        typedef royale::iterator::royale_reverse_iterator< iterator > reverse_iterator;
        typedef royale::iterator::royale_const_reverse_iterator< const_iterator > const_reverse_iterator;

        /**
        * General String definitions
        */
        typedef T value_type;
        typedef std::char_traits<T> traits_type;
        typedef T &reference;
        typedef const T &const_reference;
        typedef T *pointer;
        typedef const T *const_pointer;
        typedef ptrdiff_t difference_type;
        typedef size_t size_type;

        /**
        * Constructor
        *
        * General constructor, which does not allocate memory and sets everything to it's default
        */
        basicString<T>();

        /**
        * Copy-Constructor for royale compliant string
        *
        * Copy constructor which allows creation of a royale compliant string from
        * another royale compliant string - (NOTE: performs a deep copy!)
        *
        * \param str The royale vector which's memory shall be copied
        */
        basicString<T> (const basicString<T> &str);

        /**
        * Copy-Constructor for STL compliant string (std::string)
        *
        * Copy constructor which allows creation of a royale compliant string from
        * a STL compliant string - (NOTE: performs a deep copy!)
        *
        * \param str The STL string to copy
        */
        basicString<T> (const std::basic_string<T> &str);

        /**
        * Move-Constructor for royale compliant string
        *
        * Constructor which allows creation of a royale compliant string by moving memory
        * (NOTE: performs a shallow copy!)
        *
        * \param str The royale string which's memory shall be moved
        */
        basicString<T> (basicString<T> &&str);

        /**
        * Constructor which allows passing a royale compliant string and add it at
        * a given position - the caller might also limit the length that shall be copied
        * (NOTE: performs a deep copy!)
        *
        * If the len would exceed the length of str then all bytes till the end of
        * str are copied to the new string object.
        *
        * \param str The royale string which shall be copied
        * \param pos The position where to start to copy bytes
        * \param len The length/amount of bytes that shall be copied
        */
        basicString<T> (const basicString<T> &str, size_t pos, size_t len = npos);

        /**
        * Constructor which allows passing a STL compliant string and add it at
        * a given position - the caller might also limit the length that shall be copied
        * (NOTE: performs a deep copy!)
        *
        * If the len would exceed the length of str then all bytes till the end of
        * str are copied to the new string object.
        *
        * \param str The STL compliant string which shall be copied
        * \param pos The position where to start to copy bytes
        * \param len The length/amount of bytes that shall be copied
        */
        basicString<T> (const std::basic_string<T> &str, size_t pos, size_t len = npos);

        /**
        * Constructor which allows passing a C-Style string
        * to create a royale compliant string
        * (NOTE: performs a deep copy!)
        *
        * \param s The C-Style compliant string which shall be copied
        */
        basicString<T> (const T *s);

        /**
        * Constructor which allows passing a C-Style string
        * to create a royale compliant string by copying the first n bytes
        * (NOTE: performs a deep copy!)
        *
        * (NOTE: The next paragraph is different to std::string!)
        * If n is bigger than the length of s, then n is allocated and the
        * length of s is  used for copying - this results in a string
        * with unused allocated space and the value of where s points to.
        *
        * If n is smaller than the length of s, then n is allocated and
        * only n bytes from s are copied to the string object.
        *
        * \param s The C-Style compliant string which shall be copied
        * \param n The number of bytes to copy
        */
        basicString<T> (const T *s, size_t n);

        /**
        * Constructor which allows creating a royale compliant string with
        * n slots which are initialized by character c.
        *
        * \param n The number of slots that shall be reserved
        * \param c The character which is used for the slot's initialization
        */
        basicString<T> (size_t n, T c);

        /**
        * Initializer list initialization to initialize a string
        *
        * \param list The list of values
        */
        basicString<T> (const std::initializer_list<T> &list);

        /**
        * Destructor
        *
        * Clears the string's allocated memory by performing deletion
        */
        virtual ~basicString<T>();

        /**
        * Returns the actual size/length of the string (this is the used amount of
        * slots in the allocated area)
        *
        * \return size_t The amount of the actual used memory slots within the string
        */
        size_t size() const;

        /**
        * Returns the actual size/length of the string (this is the used amount of
        * slots in the allocated area)
        *
        * \return size_t The amount of the actual used memory slots within the string
        */
        size_t length() const;

        /**
        * Checks if the string is empty
        *
        * \return bool Returns true if the string is empty - otherwise false
        */
        bool empty() const;

        /**
        * Returns the amount of allocated slots which are maintained by the vector
        * These allocated slots might be used or unused (refer to size() for checking the size()
        * itself)
        * The implicitly added NUL termination is not counted as part of the capacity.
        *
        * \return size_t The amount of allocated slots for the element type which is bound to the string
        */
        size_t capacity() const;

        /**
        * Returns a direct pointer to the memory array used internally by the string to store its owned elements.
        *
        * Because elements in the string are guaranteed to be stored in contiguous storage locations in the same order as
        * represented by the string, the pointer retrieved can be offset to access any element in the array.
        *
        * \return const T* A pointer to the first element in the array used internally by the string.
        */
        const T *data() const;

        /**
        * Returns a reference to the first element in the string.
        * Calling this function on an empty string will result in a std::out_of_range exception.
        *
        * \return T& A reference to the first element in the string.
        * \throws std::out_of_range Exception if the string is empty
        */
        T &front();
        const T &front() const;

        /**
        * Returns an iterator to the first position
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator An iterator to the begin of the string.
        */
        iterator begin();
        const_iterator begin() const;

        /**
        * Returns an iterator to the last position
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator An iterator to the end of the string.
        */
        iterator end();
        const_iterator end() const;

        /**
        * Returns an iterator to the reverse begin (last position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator An iterator to the reverse begin of the string
        */
        reverse_iterator rbegin();
        const_reverse_iterator rbegin() const;

        /**
        * Returns an iterator to the reverse end (first position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator An iterator to the reverse end of the string
        */
        reverse_iterator rend();
        const_reverse_iterator rend() const;

        /**
        * Returns a constant iterator to the begin of the string (first position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator A constant iterator to the begin of the string
        */
        const_iterator cbegin();
        const_iterator cbegin() const;

        /**
        * Returns a constant iterator to the end of the string (last position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator A constant iterator to the end of the string
        */
        const_iterator cend();
        const_iterator cend() const;

        /**
        * Returns a constant iterator to the reverse begin of the string (last position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator A constant iterator to the reverse begin of a string
        */
        const_reverse_iterator crbegin();
        const_reverse_iterator crbegin() const;

        /**
        * Returns a constant iterator to the reverse end of the string (first position)
        * Calling this function on an empty string will result in undefined behavior
        *
        * \return iterator A constant iterator to the reverse end of a string
        */
        const_reverse_iterator crend();
        const_reverse_iterator crend() const;

        /**
        * Returns an forward iterator for the given index
        */
        iterator iteratorFromIndex (size_t index);
        const_iterator iteratorFromIndex (size_t index) const;

        /**
        * Returns an reverse iterator for the given index
        */
        reverse_iterator reverseIteratorFromIndex (size_t index);
        const_reverse_iterator reverseIteratorFromIndex (size_t index) const;

        /**
        * Returns the index position the given iterator is pointing to
        * One may receive weird results by passing an iterator of another vector object!
        * Take care that only indices from the current vector object are passed!
        *
        * \return size_t The index position
        */
        template <typename... Dummy, typename IteratorType,
                  typename = typename std::enable_if< (royale::iterator::is_same<IteratorType, reverse_iterator>::value) >::type>
        size_t indexFromIterator (IteratorType it)
        {
            return it.base() - begin();
        }

        template < typename... Dummy, typename IteratorType,
                   typename = typename std::enable_if < (royale::iterator::is_same<IteratorType, reverse_iterator>::value || royale::iterator::is_same<IteratorType, const_reverse_iterator>::value) >::type >
        size_t indexFromIterator (IteratorType it) const
        {
            return it.base() - cbegin();
        }

        /**
        * Returns the index position the given iterator is pointing to
        * One may receive weird results by passing an iterator of another vector object!
        * Take care that only indices from the current vector object are passed!
        *
        * \return size_t The index position
        */
        template <typename IteratorType,
                  typename = typename std::enable_if< (royale::iterator::is_same<IteratorType, iterator>::value) >::type>
        size_t indexFromIterator (IteratorType it)
        {
            return it - begin();
        }

        template < typename IteratorType,
                   typename = typename std::enable_if < (royale::iterator::is_same<IteratorType, iterator>::value || royale::iterator::is_same<IteratorType, const_iterator>::value) >::type >
        size_t indexFromIterator (IteratorType it) const
        {
            return it - cbegin();
        }

        /**
        * Returns a reference to the last element in the string.
        * Calling this function on an empty vector will result in a std::out_of_range exception.
        *
        * \return T& A reference to the last element in the string.
        * \throws std::out_of_range Exception if the string is empty
        */
        T &back();
        const T &back() const;

        /**
        * Adds a new element at the end of the string, after its current last element.
        * The content of str is copied to the new element (NOTE: a deep copy is performed!).
        * This effectively increases the container size, which causes an automatic reallocation
        * of the allocated storage space if -and only if- the new string size surpasses the current string capacity.
        */
        void push_back (const std::basic_string<T> &str);
        void push_back (const basicString<T> &str);
        void push_back (const std::basic_string<T> &str, size_t subpos, size_t sublen);
        void push_back (const basicString<T> &str, size_t subpos, size_t sublen);
        void push_back (const T *str);
        void push_back (const T *str, size_t n);
        void push_back (size_t n, T c);
        void push_back (const T c);

        /**
        * Adds a new element at the end of the string, after its current last element
        * and returns the actual string element.
        * The content of str is copied to the new element (NOTE: a deep copy is performed!).
        * This effectively increases the container size, which causes an automatic reallocation
        * of the allocated storage space if -and only if- the new string size surpasses the current string capacity.
        */
        basicString<T> &append (const std::basic_string<T> &str);
        basicString<T> &append (const basicString<T> &str);
        basicString<T> &append (const std::basic_string<T> &str, size_t subpos, size_t sublen);
        basicString<T> &append (const basicString<T> &str, size_t subpos, size_t sublen);
        basicString<T> &append (const T *str);
        basicString<T> &append (const T *str, size_t n);
        basicString<T> &append (size_t n, T c);
        basicString<T> &append (const T c);

        /**
        * Adds a new element at the end of the string, after its current last element
        * and returns the actual string element.
        * The content of str is copied to the new element (NOTE: a deep copy is performed!).
        * This effectively increases the container size, which causes an automatic reallocation
        * of the allocated storage space if -and only if- the new string size surpasses the current string capacity.
        */
        basicString<T> &operator+= (const std::basic_string<T> &str);
        basicString<T> &operator+= (const basicString<T> &str);
        basicString<T> &operator+= (const T *s);
        basicString<T> &operator+= (const T s);

        /**
        * Adds two stirngs and returns the result (the second string is appended)
        * The content of str is copied to the newly created element (NOTE: a deep copy is performed!).
        * This effectively increases the container size, which causes an automatic reallocation
        * of the allocated storage space if -and only if- the new string size surpasses the current string capacity.
        */
        basicString<T> operator+ (const std::basic_string<T> &str) const;
        basicString<T> operator+ (const basicString<T> &str) const;
        basicString<T> operator+ (const T *s) const;
        basicString<T> operator+ (const T s) const;

        /**
        * Inserts elements to the string
        * The content at the given position is moved backwards in the string to create space
        * for the elements which shall be inserted - given there is enough allocated space
        * If the allocation is too small to insert the bunch of data, a new block is allocated,
        * while the old data is MOVED (std::move) to the new buffer as long as the insert position is not reached.
        * When the input position is reached, the input values are completely copied.
        * At the end the original data, after the input position is moved to the new buffer and the
        * internal data buffer is swapped with the created buffer.
        */
        template < typename StringType,
                   typename = typename std::enable_if < (royale::iterator::is_same<StringType, basicString<T>>::value || royale::iterator::is_same<StringType, std::string>::value) >::type >
        basicString<T> &insert (size_t pos, const StringType &str);

        template < typename StringType,
                   typename = typename std::enable_if < (royale::iterator::is_same<StringType, basicString<T>>::value || royale::iterator::is_same<StringType, std::string>::value) >::type >
        basicString<T> &insert (size_t pos, const StringType &str, size_t subpos, size_t sublen);
        basicString<T> &insert (size_t pos, const char *s);
        basicString<T> &insert (size_t pos, const char *s, size_t n);
        basicString<T> &insert (size_t pos, size_t n, char c);
        iterator insert (const_iterator p, size_t n, char c);
        iterator insert (const_iterator p, char c);

        template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator insert (InputIterator_pos position, InputIterator_first first, InputIterator_last last);

        basicString<T> &insert (const_iterator position, std::initializer_list<value_type> il);

        /**
        * Assigns new contents to the string, replacing its current contents, and modifying its size accordingly.
        * Any elements held in the container before the call are destroyed or replaced by newly constructed elements (no assignments of elements take place).
        * This causes an automatic reallocation of the allocated storage space if - and only if - the new vector size surpasses the current string capacity.
        */
        template < typename StringType,
                   typename = typename std::enable_if < (royale::iterator::is_same<StringType, basicString<T>>::value || royale::iterator::is_same<StringType, std::string>::value) >::type >
        basicString<T> &assign (const StringType &str);

        template < typename StringType,
                   typename = typename std::enable_if < (royale::iterator::is_same<StringType, basicString<T>>::value || royale::iterator::is_same<StringType, std::string>::value) >::type >
        basicString<T> &assign (const StringType &str, size_t subpos, size_t sublen);

        basicString<T> &assign (const char *s);
        basicString<T> &assign (const char *s, size_t n);
        basicString<T> &assign (const char *str, size_t subpos, size_t sublen);
        basicString<T> &assign (basicString<T> &&str);
        basicString<T> &assign (std::initializer_list<value_type> il);
        template <typename Value,
                  typename = typename std::enable_if< (royale::iterator::is_same<Value, typename basicString<T>::value_type>::value) >::type>
        basicString<T> &assign (size_t n, Value val);

        template <typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        basicString<T> &assign (InputIterator_first first, InputIterator_last last);

        /**
        * Removes elements
        * Removes from the string either a single element (position) or a range of elements ([first,last)).
        * This effectively reduces the container size by the number of elements removed, which NOT removed.
        *
        * The removed elements remain in the string and the allocation size is not changed, the elements are
        * only moved to the end and represent free memory space, the size of the vector is reduced.
        * The destructor is not called for the removed elements! (difference to erase())
        *
        * Because strings use an array as their underlying storage, erasing elements in positions other than
        * the string end causes the container to relocate all the elements after the segment erased to their new positions.
        * This is generally an inefficient operation compared to the one performed for the same operation
        * by other kinds of sequence containers (such as list or forward_list).
        *
        * See erase() to destruct the elements in the range.
        * \return iterator The iterator position to the first element after the deleted range
        */
        iterator remove (const_iterator first, const_iterator last);
        basicString<T> &remove (size_t pos = 0, size_t len = npos);
        basicString<T> &remove (const_iterator position, size_t len = 1);

        /**
        * Erase elements
        * Erases from the string either a single element (position) or a range of elements ([first,last)).
        * This effectively reduces the container size by the number of elements removed, which are destroyed.
        *
        * Because strings use an array as their underlying storage, erasing elements in positions other than
        * the vector end causes the container to relocate all the elements after the segment erased to their new positions.
        * This is generally an inefficient operation compared to the one performed for the same operation
        * by other kinds of sequence containers (such as list or forward_list).
        *
        * A faster operation is remove()
        * \return iterator The iterator position to the first element after the deleted range
        */
        iterator erase (const_iterator first, const_iterator last);
        basicString<T> &erase (size_t pos = 0, size_t len = npos);
        basicString<T> &erase (const_iterator position, size_t len = 1);

        /**
        * Replace elements in a vector
        * This method allows replacing the elements from "start" to "end" with the element given in the
        * range from "first" to "last".
        * It only replaces as much items (starting at first) as forseen by the range from "start" to "end".
        * If the number of elements given by "first" and "last" exceeds the range from "start" to "end" the
        * exceeding elements are "dropped" and not used for replacement.
        * Given that the caller wants all elements from "first" to "last" to be replaced/appended there
        * are other signatures of replace() to provide that
        *
        * \param start The starting iterator where the replacement shall start
        * \param end The end iterator of replacement (this "hard end" will NOT be exceeded)
        * \param first The iterator to the first element taken for replacement
        * \param last The iterator to the last element taken for replacement
        * \return iterator to the first replaced item
        */
        template <typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator replace (const_iterator start, const_iterator end, InputIterator_first first, InputIterator_last last);

        /**
        * Replace elements in a vector
        * This method allows replacing vector elements with element given in the
        * range from "first" to "last".
        * It only replaces as much items as forseen by the range from "first" to "last".
        * If the number of elements given by "first" and "last" exceeds the allocation size
        * a reallocation will take place and the elements are appended to the vector.
        * Given that the caller only wants a range of elements to be replaced, even if the the
        * input range might be greater he/she shall use the signature of replace() which provides four
        * iterators as input and accepts an input range.
        *
        * \param position The iterator where the replacement shall start
        * \param first The iterator to the first element taken for replacement
        * \param last The iterator to the last element taken for replacement
        * \return iterator to the first replaced item
        */
        template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator replace (InputIterator_pos position, InputIterator_first first, InputIterator_last last);

        /**
        * Replace elements in a vector
        * This method allows replacing vector elements with the elements
        * of another input container.
        * It only replaces as much items, as there are in the input vector.
        * If the number of elements in the input vector exceeds the allocation size
        * a reallocation will take place and the elements are appended to the object.
        * Given that the caller only wants a range of elements to be replaced, even if the the
        * input range might be greater he/she shall use the signature of replace() which provides four
        * iterators as input and accepts an input range.
        *
        * \param position The starting iterator where the replacement shall start
        * \param v The container which shall be used for replacement
        * \return iterator to the first replaced item
        */
        template < typename InputIterator_pos, typename ContainerType,
                   typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                   typename = typename std::enable_if < !std::is_convertible<ContainerType, T>::value && !royale::iterator::is_iterator<ContainerType>::value >::type >
        iterator replace (InputIterator_pos position, const ContainerType &v);

        /**
        * Replace elements in a vector
        * This method allows replacing the elements from "start" to "end" with the elements in the
        * input container.
        * It only replaces as much items (starting at first) as forseen by the range from "start" to "end".
        * If the number of elements given by "first" and "last" exceeds the range from "start" to "end" the
        * exceeding elements are "dropped" and not used for replacement.
        * Given that the caller wants all elements from "first" to "last" to be replaced/appended there
        * are other signatures of replace() to provide that
        *
        * \param start The starting iterator where the replacement shall start
        * \param end The end iterator of replacement (this "hard end" will NOT be exceeded)
        * \param v The container which shall be used for replacement
        * \return iterator to the first replaced item
        */
        template < typename InputIterator_start, typename InputIterator_end, typename ContainerType,
                   typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_start>::value>::type,
                   typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_end>::value>::type,
                   typename = typename std::enable_if < !std::is_convertible<ContainerType, T>::value && !royale::iterator::is_iterator<ContainerType>::value >::type >
        iterator replace (InputIterator_start start, InputIterator_end end, const ContainerType &v);

        /**
        * Replace elements in a vector
        * This method allows replacing the elements beginning at index with
        * all items held by the container v.
        * If the number of elements exceeds the allocation size of the vector
        * a reallocation is performanced and the items are moved.
        *
        * \param index The index position where the replacement shall be started
        * \param v The container which shall be used for replacement
        * \return iterator to the first replaced item
        */
        template < typename ContainerType,
                   typename = typename std::enable_if < !std::is_convertible<ContainerType, T>::value && !royale::iterator::is_iterator<ContainerType>::value >::type >
        iterator replace (size_t index, const ContainerType &v);

        /**
        * Replace elements in a vector
        * This method allows replacing the elements starting at index with the given range
        * from "first" to "last".
        * It only replaces as much items (starting at first) as forseen by the range from "first" to "last".
        * If the number of elements given by "first" and "last" exceeds the allocation size
        * a reallocation is performed and the elements are moved to the new allocation space.
        *
        * \param index The index position where the replacement shall be started
        * \param first The iterator to the first element taken for replacement
        * \param last The iterator to the last element taken for replacement
        * \return iterator to the first replaced item
        */
        template <typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator replace (size_t index, InputIterator_first first, InputIterator_last last);

        /**
        * Find
        * Searches the string for the first occurrence of the sequence specified by its arguments.
        * When pos is specified, the search only includes characters at or after position pos,
        * ignoring any possible occurrences that include characters before pos.
        *
        * Notice whenever more than one character is being searched for, it is not enough that just one of these
        * characters match, but the entire sequence must match.
        *
        * \return The position of the first character of the first match.
        *         If no matches were found, the function returns string::npos.
        */
        template < typename StringType,
                   typename = typename std::enable_if < (royale::iterator::is_same<StringType, basicString<T>>::value || royale::iterator::is_same<StringType, std::string>::value) >::type >
        size_t find (const StringType &str, size_t pos = 0) const
        {
            const char *found = strstr (data() + pos, (str.c_str()));
            return found == nullptr ? npos : indexFromIterator (const_iterator (found));
        }

        size_t find (const char *str, size_t pos = 0) const
        {
            const char *found = strstr (data() + pos, str);
            return found == nullptr ? npos : indexFromIterator (const_iterator (found));
        }

        size_t find (const char *s, size_t pos, size_type n) const
        {
            size_t s_len;

            /* segfault here if s is not NULL terminated */
            if (0 == (s_len = strlen (s)))
            {
                return npos;
            }

            if (pos >= length())
            {
                return npos;
            }

            for (size_t i = pos; i < length(); i++)
            {
                // if found potential begin; check rest
                if ( (at (i) == s[0]) && (compare (i, n, s, 0, n) == 0))
                {
                    return i;
                }
            }
            return npos;
        }


        size_t find (char c, size_t pos = 0) const
        {
            if (pos >= length())
            {
                return npos;
            }

            for (size_t i = pos; i < length(); i++)
            {
                if (at (i) == c)
                {
                    return i;
                }
            }
            return npos;
        }

        /**
        * Compare strings
        * Compares the value of the string object (or a substring) to the sequence of characters specified by its arguments.
        * The compared string is the value of the string object or -if the signature used has a pos and a len parameters- the substring that begins at its character in position pos and spans len characters.
        * This string is compared to a comparing string, which is determined by the other arguments passed to the function.
        *
        * Parameters
        * - str
        *   Another string object, used entirely (or partially) as the comparing string.
        *
        * - pos
        *   Position of the first character in the compared string.
        *   If this is greater than the string length, it throws out_of_range.
        *   Note: The first character is denoted by a value of 0 (not 1).
        *
        * - len
        *   Length of compared string (if the string is shorter, as many characters as possible).
        *   A value of string::npos indicates all characters until the end of the string.
        *   subpos, sublen
        *   Same as pos and len above, but for the comparing string.
        *
        * - s
        *   Pointer to an array of characters.
        *   If argument n is specified (4), the first n characters in the array are used as the comparing string.
        *   Otherwise (3), a null-terminated sequence is expected: the length of the sequence with the characters to use as
        *   comparing string is determined by the first occurrence of a null character.
        * - n
        *   Number of characters to compare.
        *
        * Return Value
        * Returns a signed integral indicating the relation between the strings:
        * value relation between compared string and comparing string
        * 0 They compare equal
        * <0    Either the value of the first character that does not match is lower in the compared string, or all compared characters match but the compared string is shorter.
        * >0    Either the value of the first character that does not match is greater in the compared string, or all compared characters match but the compared string is longer.
        */
        int compare (const basicString<T> &str) const;
        int compare (size_t pos, size_t len, const basicString<T> &str) const;
        int compare (size_t pos, size_t nlen, const basicString<T> &str, size_t subpos, size_t sublen) const;
        int compare (size_t pos, size_t nlen, const char *s, size_t subpos, size_t sublen) const;
        int compare (const char *s) const;
        int compare (size_t pos, size_t nlen, const char *s, size_t n) const;

        /**
        * Creates a substring
        */
        basicString<T> substr (size_t pos, size_t len) const;

        /**
        * Removes the last element in the string, effectively reducing the container size by one.
        * The last element is destroyed by calling it's destructor; the size is reduced by one BUT
        * there is no reallocation performed to resize the string to it's contents or to reduce the
        * the string capacity by one.
        * The allocated space remains the same.
        */
        void pop_back();

        /**
        * Access an element
        *
        * Returns a reference to the element at position index in the string container.
        * A similar member function, at(), has the same behavior as this operator function,
        * except that at() is bound-checked and signals if the requested position is
        * out of range by throwing an out_of_range exception.
        *
        * \param index The index to access within the string's storage
        * \return T& The Reference to the elements at the specified position in the string
        */
        T &operator[] (size_t index);

        /**
        * Access an element
        *
        * Returns a reference to the element at position index in the string container.
        * A similar member function, at(), has the same behavior as this operator function,
        * except that at() is bound-checked and signals if the requested position is
        * out of range by throwing an out_of_range exception.
        *
        * \param index The index to access within the string's storage
        * \return T& The Reference to the elements at the specified position in the string
        */
        const T &operator[] (size_t index) const;

        /**
        * Access an element
        *
        * The function automatically checks whether index is within the bounds
        * of valid elements in the string, throwing an out_of_range exception
        * if it is not (i.e., if index is greater or equal than its size).
        * This is in contrast with member operator[], that does not check against bounds.
        *
        * \param index The index to access within the string's storage
        * \return T& The Reference to the elements at the specified position in the string
        */
        T &at (size_t index);

        /**
        * Access an element
        *
        * The function automatically checks whether index is within the bounds
        * of valid elements in the string, throwing an out_of_range exception
        * if it is not (i.e., if index is greater or equal than its size).
        * This is in contrast with member operator[], that does not check against bounds.
        *
        * \param index The index to access within the string's storage
        * \return T& The Reference to the elements at the specified position in the string
        */
        const T &at (size_t index) const;

        /**
        * Assign another royale compliant string
        *
        * Assigns new contents to the container, replacing its current contents,
        * and modifying its size accordingly.
        * This method copies all elements held by str into the container
        *
        * \param str A string of the same storage type
        * \return basicString<T>& Returns *this
        */
        basicString<T> &operator= (const basicString<T> &str);

        /**
        * Move elements to another royale compliant string
        *
        * Moves the elements of str into the container.
        * The source container is reset to it's initial state which
        * causes the allocation size to be reset (the allocated memory is moved)
        * and the size counter to be reset.
        * The data pointer is set to null.
        *
        * Leaves the source object in a valid/initial state allowing
        * somebody to reuse it for other purposes/data - like after
        * executing clear())
        *
        * \param str A string object of the same type (i.e., with the same template parameters, T and Alloc).
        * \return basicString<T>& Returns *this
        */
        basicString<T> &operator= (basicString<T> &&str);

        /**
        * Assign the contents of an STL compliant string container by
        * replacing container's current contents if necessary
        * and modifying its size accordingly.
        * This method copies all elements held by str into the container
        *
        * \param str An STL compliant string of the same storage type
        * \return basicString<T>& Returns *this
        */
        basicString<T> &operator= (const std::basic_string<T> &str);

        /**
        * Assign the contents of an C-Style compliant string container by
        * replacing container's current contents if necessary
        * and modifying its size accordingly.
        * This method copies all elements held by str into the container
        *
        * \param str An C-Style compliant string of the same storage type
        * \return basicString<T>& Returns *this
        */
        basicString<T> &operator= (const T *str);

        /**
        * Assign a character to a royale compliant string container by
        * replacing container's current contents and modifying its size accordingly.
        *
        * \param str The character to assign to the royale compliant string
        * \return basicString<T>& Returns *this
        */
        basicString<T> &operator= (const T str);

        /**
        * Checks equality with an STL compliant vector
        *
        * \param str An STL compliant string of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const std::basic_string<T> &str) const;

        /**
        * Checks equality with a royale compliant string
        *
        * \param str An royale compliant string of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const basicString<T> &str) const;

        /**
        * Checks equality with a C-Style string
        *
        * \param str An C-Style string of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const T *str) const;

        /**
        * Checks unequality with an STL compliant string
        *
        * \param str An STL compliant string of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const std::basic_string<T> &str) const;

        /**
        * Checks unequality with a royale compliant string
        *
        * \param str An royale compliant string of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const basicString<T> &str) const;

        /**
        * Checks unequality with a C-Style string
        *
        * \param str A C-style compliant string of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const T *str) const;

        /**
        * Checks less-than relation with a royale compliant string
        *
        * A comparison lhs < rhs will be true if (and only if):
        * - lhs is empty, and rhs is not
        * - lhs is a prefix of rhs and shorter than rhs
        * - lhs is not a prefix, and the first differing character in lhs is less than the one in rhs
        *   (using the operator< defined by the underlying character type T)
        *
        * \param str A royale compliant string of the same storage type
        * \return bool Returns true if less-than and false if not
        */
        bool operator< (const basicString<T> &str) const;

        /**
        * Removes all elements from the string (which are destroyed), leaving the container with a size of 0.
        *
        * A reallocation is not performed and the string's capacity is destroyed (everything is freed).
        */
        void clear();

        /**
        * Allows a data swap (convenience function to call classswap)
        * Swaps the given string data with the current one
        */
        void swap (basicString<T> &string);

        /**
        * Modifies the string to the given allocation size and allocates the buffers (it may shrink)
        *
        * Creates any amount of elements (allocates the memory already)
        * and moves the existing elements to these slots; afterwards the old space is dumped.
        *
        * If the given newSize is smaller than the already used slots, the string will shrink.
        * This means that all elements which are not covered within this capacity (the last ones) will be
        * deleted.
        *
        * \param newSize The amount of slots to remain in the string (might shrink or enlarge the string)
        */
        void resize (size_t newSize);

        /**
        * Extends the string to a higher allocation size and allocates the buffers
        *
        * Reserves any amount of free allocation slots (allocates the memory already) to be later
        * used for the element-types bound to the given string vector.
        *
        * If the given size to reserve is smaller than the already reserved space, then the function
        * return immediately; otherwise the necessary memory allocation is performed and the size is
        * extended to "size"
        *
        * \param size The size (number of element-types) of elements that should be allocated.
        */
        void reserve (size_t size);

        /**
        * Shrinks the string's allocation to it's size
        *
        * Changes the size of the allocated buffer to the string's size
        * this may result in freeing unneeded memory allocation.
        */
        void shrink_to_fit();

        /**
        * Converts the given std::basic_string (STL) to the string type used by the royale API
        *
        * \param str The STL string which should be converted to the royale API vector format
        * \return The royale API compliant string format
        */
        static basicString<T> fromStdString (const std::basic_string<T> &str);

        /**
        * Converts the given C-Style array to the string type used by the royale API
        *
        * \param str The C-Style array which should be converted to the royale API vector format
        * \return The royale API compliant string format
        */
        static basicString<T> fromCArray (const T *str);

        /**
        * Convert to String from any Type
        */
        template<typename... Dummy, typename U, typename Type = T>
        static typename std::enable_if <
        std::is_same<Type, char>::value,
            basicString<Type>
            >::type fromAny (const U value)
        {
            std::ostringstream os;
            os << value;
            std::string stdString (os.str());
            return basicString<T> (stdString);
        }

        /**
        * Convert to WString from any Type
        */
        template<typename... Dummy, typename U, typename Type = T>
        static typename std::enable_if <
        std::is_same<Type, wchar_t>::value,
            basicString<Type>
            >::type fromAny (const U value)
        {
            std::wstringstream os;
            os << value;
            std::wstring stdString (os.str());
            return basicString<Type> (stdString);
        }

        /**
        * Convert to String from Int
        */
        static basicString<T> fromInt (int value)
        {
            return basicString<T>::fromAny<int> (value);
        }

        /**
        * Convert to String from unsigned Int
        */
        static basicString<T> fromUInt (unsigned int value)
        {
            return basicString<T>::fromAny<unsigned int> (value);
        }

        /**
        * Converts the royale string object to a C-Style array
        *
        * \return T* The C-Style Array
        */
        const T *c_str() const;

        /**
        * User convenience function to allow conversion to std::basic_string
        * which might be used outside the library by the application for further processing
        *
        * \return std::basic_string containing the items of the
        */
        std::basic_string<T> toStdString() const;

        /**
        * User convenience function to allow conversion to std::basic_string
        * which might be used if the royale compliant string is a const
        *
        * \return std::basic_string containing the items of the
        */
        static std::basic_string<T> toStdString (const basicString<T> &str);

        /**
        * Returns the max_size of the string
        *
        * \return size_t The maximum length of the string
        */
        size_t max_size() const;

    public:
        static const size_t npos = ( (size_t) ~0);
        /** The NUL terminator, which is '\0' for T = char, and L'\0' for T = wchar_t */
        static const T EOS = 0;

    private:
        /**
        * Size of the NUL terminator, used instead of having "+ 1" as a magic number in multiple
        * locations. This is always 1 even when T = wchar_t, because it's the number of T that are
        * required to form the NUL terminator.
        */
        static const size_t NUL_TERMINATOR_LENGTH = 1;

        inline void preReserve (const std::basic_string<T> &str, size_t sublen = 0);
        inline void preReserve (const basicString<T> &str, size_t sublen = 0);
        inline void preReserve (const T *s, size_t len = 0);
        inline void preReserve (size_t len);

        static std::shared_ptr<T> getEmptyString()
        {
            std::shared_ptr<T> newBuffer (new T[NUL_TERMINATOR_LENGTH], std::default_delete<T[]>());
            newBuffer.get() [0] = EOS;
            return newBuffer;
        }

        template<typename U>
        friend void classswap (basicString<U> &first, basicString<U> &second);

        template<typename U>
        friend std::ostream &operator<< (std::ostream &os, const basicString<U> &str);

        size_t m_allocationSize;
        size_t m_actualSize;
        std::shared_ptr<T> m_strdata;
    };

    template<typename T>
    void classswap (basicString<T> &first, basicString<T> &second)
    {
        std::swap (first.m_actualSize, second.m_actualSize);
        std::swap (first.m_allocationSize, second.m_allocationSize);
        std::swap (first.m_strdata, second.m_strdata);
    }

    //! Template implementation
    template<typename T>
    basicString<T>::basicString() :
        m_allocationSize (NUL_TERMINATOR_LENGTH),
        m_actualSize (0),
        m_strdata (getEmptyString())
    { }

    template<typename T>
    basicString<T>::basicString (const basicString<T> &str)
        : basicString()
    {
        if (str.m_actualSize)
        {
            m_actualSize = str.m_actualSize;
            m_allocationSize = str.m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_allocationSize; ++i)
            {
                m_strdata.get() [i] = str.m_strdata.get() [i];
            }
        }
    }


    template<typename T>
    basicString<T>::basicString (const std::basic_string<T> &str)
        : basicString()
    {
        if (!str.empty())
        {
            m_actualSize = str.length();
            m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_allocationSize; ++i)
            {
                m_strdata.get() [i] = str[i];
            }
        }
    }

    template<typename T>
    basicString<T>::basicString (const basicString<T> &str, size_t pos, size_t len)
        : basicString()
    {
        if (str.length())
        {
            size_t endPos = ( (len + pos) > str.length() ? str.length() : (len + pos));

            m_actualSize = endPos - pos;
            m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                m_strdata.get() [i] = str.m_strdata.get() [pos + i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }
    }

    template<typename T>
    basicString<T>::basicString (const std::basic_string<T> &str, size_t pos, size_t len)
        : basicString()
    {
        if (str.length())
        {
            size_t endPos = ( (len + pos) > str.length() ? str.length() : (len + pos));

            m_actualSize = endPos - pos;
            m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                m_strdata.get() [i] = str[pos + i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }
    }

    template<typename T>
    basicString<T>::basicString (const T *s)
        : basicString()
    {
        if (s != nullptr)
        {
            m_actualSize = royale_strlen<T> (s);
            if (m_actualSize)
            {
                m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
                m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

                for (size_t i = 0; i < m_actualSize; ++i)
                {
                    m_strdata.get() [i] = s[i];
                }
                m_strdata.get() [m_actualSize] = EOS;
            }
        }
    }

    template<typename T>
    basicString<T>::basicString (const T *s, size_t n)
        : basicString()
    {
        if (n > 0 && s != nullptr)
        {
            m_actualSize = royale_strnlen<T> (s, n);
            m_allocationSize = n + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            size_t i = 0;
            for (i = 0; i < m_actualSize; ++i)
            {
                m_strdata.get() [i] = s[i];
            }
            m_strdata.get() [i] = EOS;
        }
    }

    template<typename T>
    basicString<T>::basicString (size_t n, T c)
        : basicString()
    {
        if (n > 0)
        {
            m_actualSize = n;
            m_allocationSize = n + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            size_t i;
            for (i = 0; i < n; ++i)
            {
                m_strdata.get() [i] = c;
            }
            m_strdata.get() [i] = EOS;
        }
    }

    template<typename T>
    basicString<T>::basicString (const std::initializer_list<T> &list) :
        m_allocationSize (NUL_TERMINATOR_LENGTH),
        m_actualSize (0),
        m_strdata (getEmptyString())
    {
        preReserve (list.size() + 1);
        for (auto itm : list)
        {
            append (itm);
        }
    }

    template<typename T>
    basicString<T>::basicString (basicString<T> &&str) :
        m_allocationSize (NUL_TERMINATOR_LENGTH),
        m_actualSize (0),
        m_strdata (getEmptyString())
    {
        *this = std::move (str);
    }

    template<class T>
    basicString<T>::~basicString()
    { }

    template<typename T>
    const T *basicString<T>::data() const
    {
        return m_strdata.get();
    }

    template<typename T>
    size_t basicString<T>::size() const
    {
        return m_actualSize;
    }

    template<typename T>
    size_t basicString<T>::length() const
    {
        return m_actualSize;
    }

    template<typename T>
    bool basicString<T>::empty() const
    {
        return m_actualSize == 0;
    }

    template<typename T>
    size_t basicString<T>::capacity() const
    {
        return m_allocationSize ? m_allocationSize - 1 : 0;
    }

    template<class T>
    T &basicString<T>::front()
    {
        if (m_strdata == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return m_strdata.get() [0];
    }

    template<class T>
    const T &basicString<T>::front() const
    {
        if (m_strdata == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return m_strdata.get() [0];
    }

    template<class T>
    T &basicString<T>::back()
    {
        if (m_strdata == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return m_strdata.get() [m_actualSize - 1];
    }

    template<class T>
    const T &basicString<T>::back() const
    {
        if (m_strdata == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return m_strdata.get() [m_actualSize - 1];
    }

    template<class T>
    size_t basicString<T>::max_size() const
    {
        return npos;
    }

    template<class T>
    void basicString<T>::reserve (size_t capacity)
    {
        if ( (capacity + NUL_TERMINATOR_LENGTH) > m_allocationSize)
        {
            std::shared_ptr<T> newBuffer (new T[capacity + NUL_TERMINATOR_LENGTH], std::default_delete<T[]>());
            for (size_t i = 0; i < m_actualSize; ++i)
            {
                newBuffer.get() [i] = m_strdata.get() [i];
            }
            newBuffer.get() [m_actualSize] = EOS;

            m_allocationSize = capacity + NUL_TERMINATOR_LENGTH;
            m_strdata.reset();
            m_strdata = newBuffer;
        }
    }

    template<class T>
    void basicString<T>::resize (size_t newSize)
    {
        if (newSize != m_actualSize)
        {
            std::shared_ptr<T> newBuffer (new T[newSize + NUL_TERMINATOR_LENGTH], std::default_delete<T[]>());
            memset (newBuffer.get(), 0, sizeof (* (newBuffer.get())));

            size_t l_Size = newSize < m_allocationSize ? newSize : m_allocationSize;
            for (size_t i = 0; i < l_Size; ++i)
            {
                newBuffer.get() [i] = m_strdata.get() [i];
            }
            newBuffer.get() [l_Size] = EOS;

            m_actualSize = newSize;
            m_allocationSize = newSize + NUL_TERMINATOR_LENGTH;

            m_strdata.reset();
            m_strdata = newBuffer;
        }
    }

    template<class T>
    void basicString<T>::shrink_to_fit()
    {
        if (m_strdata == nullptr || m_allocationSize == 0 || m_actualSize == 0)
        {
            return;
        }

        std::shared_ptr<T> newBuffer (new T[m_actualSize + NUL_TERMINATOR_LENGTH], std::default_delete<T[]>());
        for (size_t i = 0; i < (m_actualSize + NUL_TERMINATOR_LENGTH); ++i)
        {
            newBuffer.get() [i] = m_strdata.get() [i];
        }

        m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;

        m_strdata.reset();
        m_strdata = newBuffer;
        return;
    }

    template<class T>
    inline void basicString<T>::preReserve (const std::basic_string<T> &str, size_t sublen)
    {
        if (capacity() < size() + NUL_TERMINATOR_LENGTH + (sublen == 0 ? str.length() : sublen))
        {
            reserve (size_t (capacity() + (capacity() / 2) + (sublen == 0 ? str.length() : sublen)));
        }
    }

    template<class T>
    inline void basicString<T>::preReserve (const basicString<T> &str, size_t sublen)
    {
        if (capacity() < size() + NUL_TERMINATOR_LENGTH + (sublen == 0 ? str.length() : sublen))
        {
            reserve (size_t (capacity() + (capacity() / 2) + (sublen == 0 ? str.length() : sublen)));
        }
    }

    template<class T>
    inline void basicString<T>::preReserve (const T *s, size_t len)
    {
        if (capacity() < (size() + NUL_TERMINATOR_LENGTH + (len == 0 ? royale_strlen<T> (s) : len)))
        {
            reserve (size_t (capacity() + (capacity() / 2) + (len == 0 ? royale_strlen<T> (s) : len)));
        }
    }

    template<class T>
    inline void basicString<T>::preReserve (size_t len)
    {
        if (capacity() < (size() + NUL_TERMINATOR_LENGTH + len))
        {
            reserve (size_t (capacity() + (capacity() / 2) + len));
        }
    }

    template<class T>
    basicString<T> &basicString<T>::append (const std::basic_string<T> &str)
    {
        preReserve (str);

        for (size_t i = 0; i < str.length(); ++i)
        {
            m_strdata.get() [m_actualSize++] = str[i];
        }
        m_strdata.get() [m_actualSize] = EOS;

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const basicString<T> &str)
    {
        if (str.length())
        {
            preReserve (str);

            for (size_t i = 0; i < str.length(); ++i)
            {
                m_strdata.get() [m_actualSize++] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const std::basic_string<T> &str, size_t subpos, size_t sublen)
    {
        size_t endPos = (subpos + sublen) < str.length() ? subpos + sublen : str.length();

        if (endPos - subpos > 0)
        {
            preReserve (str, endPos - subpos);

            for (size_t i = subpos; i < endPos; ++i)
            {
                m_strdata.get() [m_actualSize++] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const basicString<T> &str, size_t subpos, size_t sublen)
    {
        size_t endPos = (subpos + sublen) < str.length() ? subpos + sublen : str.length();

        if (endPos - subpos > 0)
        {
            preReserve (str, endPos - subpos);

            for (size_t i = subpos; i < endPos; ++i)
            {
                m_strdata.get() [m_actualSize++] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const T *str)
    {
        if (royale_strlen<T> (str) > 0)
        {
            preReserve (royale_strlen<T> (str));

            for (size_t i = 0; i < royale_strlen<T> (str); ++i)
            {
                m_strdata.get() [m_actualSize++] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const T *str, size_t n)
    {
        if (!n)
        {
            return *this;
        }

        size_t copy_n = royale_strnlen<T> (str, n);
        if (copy_n)
        {
            preReserve (str, copy_n);

            for (size_t i = 0; i < copy_n; ++i)
            {
                m_strdata.get() [m_actualSize++] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (size_t n, T c)
    {
        if (n)
        {
            preReserve (n);

            for (size_t i = 0; i < n; ++i)
            {
                m_strdata.get() [m_actualSize++] = c;
            }
            m_strdata.get() [m_actualSize] = EOS;
        }

        return *this;
    }

    template<class T>
    basicString<T> &basicString<T>::append (const T c)
    {
        preReserve (m_actualSize + 1);

        m_strdata.get() [m_actualSize++] = c;
        m_strdata.get() [m_actualSize] = EOS;

        return *this;
    }

    template<typename T>
    basicString<T> &basicString<T>::operator+= (const std::basic_string<T> &str)
    {
        return append (str);
    }

    template<typename T>
    basicString<T> &basicString<T>::operator+= (const basicString<T> &str)
    {
        return append (str);
    }

    template<typename T>
    basicString<T> &basicString<T>::operator+= (const T *s)
    {
        return append (s);
    }

    template<typename T>
    basicString<T> &basicString<T>::operator+= (const T s)
    {
        return append (s);
    }

    template<typename T>
    basicString<T> basicString<T>::operator+ (const std::basic_string<T> &str) const
    {
        basicString<T> bString (*this);
        bString += str;
        return std::move (bString);
    }

    template<typename T>
    basicString<T> basicString<T>::operator+ (const basicString<T> &str) const
    {
        basicString<T> bString (*this);
        bString += str;
        return std::move (bString);
    }

    template<typename T>
    basicString<T> basicString<T>::operator+ (const T *s) const
    {
        basicString<T> bString (*this);
        bString += s;
        return std::move (bString);
    }

    template<typename T>
    basicString<T> basicString<T>::operator+ (const T s) const
    {
        basicString<T> bString (*this);
        bString += s;
        return std::move (bString);
    }

    template<class T>
    void basicString<T>::pop_back()
    {
        if (m_actualSize > 0)
        {
            m_actualSize--;
            m_strdata.get() [m_actualSize] = EOS;
        }
    }

    template<class T>
    void basicString<T>::push_back (const std::basic_string<T> &str)
    {
        append (str);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const basicString<T> &str)
    {
        append (str);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const std::basic_string<T> &str, size_t subpos, size_t sublen)
    {
        append (str, subpos, sublen);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const basicString<T> &str, size_t subpos, size_t sublen)
    {
        append (str, subpos, sublen);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const T *str)
    {
        append (str);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const T *str, size_t n)
    {
        append (str, n);
        return;
    }

    template<class T>
    void basicString<T>::push_back (size_t n, T c)
    {
        append (n, c);
        return;
    }

    template<class T>
    void basicString<T>::push_back (const T c)
    {
        append (c);
        return;
    }

    template<class T>
    template <typename StringType,
              typename>
    basicString<T> &basicString<T>::assign (const StringType &str)
    {
        return assign (const_iterator (str.data()), const_iterator (str.data() + str.length()));
    }

    template<class T>
    template <typename StringType,
              typename>
    basicString<T> &basicString<T>::assign (const StringType &str, size_t subpos, size_t sublen)
    {
        if (subpos >= str.length())
        {
            throw std::out_of_range ("assign: subpos out of container range");
        }

        if (sublen == npos)
        {
            return assign (const_iterator (str.data() + subpos), const_iterator (str.data() + str.length()));
        }
        return assign (const_iterator (str.data() + subpos), const_iterator (str.data() + subpos + sublen));
    }

    template<class T>
    basicString<T> &basicString<T>::assign (const char *str, size_t subpos, size_t sublen)
    {
        if (subpos >= royale_strlen<T> (str))
        {
            throw std::out_of_range ("assign: subpos out of container range");
        }

        if (sublen == npos)
        {
            return assign (const_iterator (str + subpos), const_iterator (str + royale_strlen<T> (str)));
        }
        return assign (const_iterator (str + subpos), const_iterator (str + subpos + sublen));
    }

    template <class T>
    basicString<T> &basicString<T>::assign (const char *s)
    {
        return assign (const_iterator (s), const_iterator (s + strlen (s)));
    }

    template <class T>
    basicString<T> &basicString<T>::assign (const char *s, size_t n)
    {
        return assign (const_iterator (s), const_iterator (s + n));
    }

    template <class T>
    basicString<T> &basicString<T>::assign (basicString<T> &&str)
    {
        swap (str);
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::assign (std::initializer_list<typename basicString<T>::value_type> il)
    {
        if (il.size() >= max_size())
        {
            throw std::length_error ("assign: String too long exception, exceeds npos!");
        }

        clear();
        preReserve (il.size());
        for (auto itm : il)
        {
            append (itm);
        }
        m_strdata.get() [m_actualSize] = EOS;
        return *this;
    }

    template <class T>
    template <typename Value,
              typename>
    basicString<T> &basicString<T>::assign (size_t n, Value val)
    {
        if (n >= max_size())
        {
            throw std::length_error ("assign: String too long exception, exceeds npos!");
        }

        // realloc is needed
        if (m_allocationSize < (n + NUL_TERMINATOR_LENGTH))
        {
            // allocate a bigger block and move data
            m_actualSize = 0;
            m_allocationSize = n + (n / 2) + NUL_TERMINATOR_LENGTH;

            std::shared_ptr<T> newBuffer (new T[m_allocationSize], std::default_delete<T[]>());

            // insert given data
            for (size_t i = 0; i < n; ++i)
            {
                newBuffer.get() [i] = val;
            }

            m_actualSize = n;
            m_strdata.reset();
            m_strdata = newBuffer;
            m_strdata.get() [m_actualSize] = EOS;
        }

        // realloc is not needed
        else
        {
            iterator beginPos = begin();
            auto p = m_strdata.get();

            // now we have as much space as needed in between
            for (const_iterator insertIterator (beginPos); insertIterator != (beginPos + n); insertIterator++)
            {
                p [insertIterator - beginPos] = val;
            }

            // erase unused elements at the end
            if (n < m_actualSize)
            {
                erase (iteratorFromIndex (n), iteratorFromIndex (m_actualSize));
            }

            m_actualSize = n;
            p [m_actualSize] = EOS;
        }

        return *this;
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename>
    basicString<T> &basicString<T>::assign (InputIterator_first first, InputIterator_last last)
    {
        // return position
        size_t sizeLength = (last - first) < 0 ? first - last : last - first;

        if (static_cast<uint64_t> ( (last - first) < 0 ? first - last : last - first) >= npos)
        {
            throw std::length_error ("assign: String too long exception, exceeds npos!");
        }

        // realloc is needed
        if (m_allocationSize < (sizeLength + NUL_TERMINATOR_LENGTH))
        {
            // allocate a bigger block and move data
            m_actualSize = 0;
            m_allocationSize = sizeLength + (sizeLength / 2) + NUL_TERMINATOR_LENGTH;

            std::shared_ptr<T> newBuffer (new T[m_allocationSize], std::default_delete<T[]>());

            // insert given data
            for (const_iterator iter (first); iter != last; ++iter)
            {
                newBuffer.get() [m_actualSize] = *iter;
                m_actualSize++;
            }

            m_strdata.reset();
            m_strdata = newBuffer;
            m_strdata.get() [m_actualSize] = EOS;
        }

        // realloc is not needed
        else
        {
            auto p = m_strdata.get();
            for (size_t i = 0; i < sizeLength; i++)
            {
                p[i] = * (first + i);
            }

            // erase unused elements at the end
            if (sizeLength < m_actualSize)
            {
                erase (iteratorFromIndex (sizeLength), iteratorFromIndex (m_actualSize));
            }

            m_actualSize = sizeLength;
            p [m_actualSize] = EOS;
        }

        return *this;
    }

    template <class T>
    template <typename StringType,
              typename>
    basicString<T> &basicString<T>::insert (size_t pos, const StringType &str)
    {
        insert (iteratorFromIndex (pos), str.begin(), str.end());
        return *this;
    }

    template <class T>
    template <typename StringType,
              typename>
    basicString<T> &basicString<T>::insert (size_t pos, const StringType &str, size_t subpos, size_t sublen)
    {
        insert (iteratorFromIndex (pos), iterator (str.data() + subpos), iterator (str.data() + subpos + sublen));
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::insert (size_t pos, const char *s)
    {
        insert (iteratorFromIndex (pos), iterator (s), iterator (s + strlen (s)));
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::insert (size_t pos, const char *s, size_t n)
    {
        insert (iteratorFromIndex (pos), iterator (s), iterator (s + n));
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::insert (size_t pos, size_t n, char c)
    {
        insert (pos, basicString<T> (n, c));
        return *this;
    }

    template <class T>
    typename basicString<T>::iterator basicString<T>::insert (const_iterator p, size_t n, char c)
    {
        basicString<T> a (n, c);
        return insert (p, a.begin(), a.end());
    }

    template <class T>
    typename basicString<T>::iterator basicString<T>::insert (const_iterator p, char c)
    {
        basicString<T> tempString (1, c);
        return insert (p, tempString.begin(), tempString.end());
    }

    template <class T>
    template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
              typename, typename, typename>
    typename basicString<T>::iterator basicString<T>::insert (InputIterator_pos position, InputIterator_first first, InputIterator_last last)
    {
        assert (position <= end());

        // return position
        size_t index;
        size_t sizeLength = (last - first) < 0 ? first - last : last - first;

        // realloc is needed
        if (m_allocationSize < (sizeLength + size() + NUL_TERMINATOR_LENGTH))
        {
            // allocate a bigger block and move data
            size_t oldSize = m_actualSize;
            m_actualSize = 0;
            m_allocationSize = (sizeLength + capacity()) + (sizeLength / 2) + NUL_TERMINATOR_LENGTH;

            std::shared_ptr<T> newBuffer (new T[m_allocationSize], std::default_delete<T[]>());

            // copy old data, till the insert point is reached
            index = 0;
            for (index = 0; index < indexFromIterator (position); ++index)
            {
                newBuffer.get() [index] = std::move (operator[] (index));
                m_actualSize++;
            }

            // insert given data
            for (InputIterator_first iter = first; iter != last; ++iter)
            {
                newBuffer.get() [m_actualSize] = *iter;
                m_actualSize++;
            }

            // insert old data at the end if there was any
            for (size_t iNew = index; iNew < oldSize; ++iNew)
            {
                newBuffer.get() [m_actualSize] = std::move (operator[] (iNew));
                m_actualSize++;
            }

            m_strdata.reset();
            m_strdata = newBuffer;
            m_strdata.get() [m_actualSize] = EOS;
        }

        // realloc is not needed
        else
        {
            size_t iteratingTimes = end() - position;
            size_t lastIndex      = m_actualSize - 1;

            // take the originally last item and place it at the new end
            const_reverse_iterator posChanger;
            auto p = m_strdata.get();
            for (posChanger = rbegin(); iteratingTimes > 0; posChanger++)
            {
                size_t offset = sizeLength + (posChanger - rbegin());

                // if we place items behind the actualSize, we need to use placement new
                // otherwise we have to use "="; memAssign manages these cases.
                p [lastIndex + offset] = *posChanger;

                iteratingTimes--;
            }

            // now we have as much space as needed in between
            index = indexFromIterator (position);
            for (size_t offset = 0; offset < sizeLength; offset++)
            {
                // if we place items behind the actualSize, we need to use placement new
                // otherwise we have to use "="; memAssign manages these cases.
                p [index + offset] = * (first + offset);
            }

            m_actualSize += sizeLength;
            p [m_actualSize] = EOS;
        }
        return basicString<T>::iterator (data() + index);
    }

    template <class T>
    basicString<T> &basicString<T>::insert (const_iterator position, std::initializer_list<typename basicString<T>::value_type> il)
    {
        insert (position, il.begin(), il.end());
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::erase (size_t pos, size_t len)
    {
        erase (iteratorFromIndex (pos), iteratorFromIndex (pos) + len);
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::erase (const_iterator position, size_t len)
    {
        erase (position, position + len);
        return *this;
    }

    template <class T>
    typename basicString<T>::iterator basicString<T>::erase (const_iterator first, const_iterator last)
    {
        if (m_actualSize > 0)
        {
            size_t oldSize = m_actualSize;
            const_iterator realFirst (first);
            const_iterator realLast (last);

            if (realLast > end())
            {
                realLast = end();
            }

            assert (realFirst < end() && realLast <= end());

            auto p = m_strdata.get();
            for (size_t i = indexFromIterator (remove (realFirst, realLast)); i < oldSize; i++)
            {
                p [i] = EOS;
            }

            if (m_actualSize == 0)
            {
                return begin();
            }
            return iterator (data() + indexFromIterator (realFirst) + 1);
        }
        return begin();
    }

    template <class T>
    basicString<T> &basicString<T>::remove (size_t pos, size_t len)
    {
        remove (iteratorFromIndex (pos), iteratorFromIndex (pos) + len);
        return *this;
    }

    template <class T>
    basicString<T> &basicString<T>::remove (const_iterator position, size_t len)
    {
        remove (position, position + len);
        return *this;
    }

    template <class T>
    typename basicString<T>::iterator basicString<T>::remove (const_iterator first, const_iterator last)
    {
        if (m_actualSize > 0)
        {
            const_iterator realFirst (first);
            const_iterator realLast (last);

            if (realLast > end())
            {
                realLast = end();
            }

            assert (realFirst < end() && realLast <= end());

            size_t noDelItems = (realLast - realFirst);
            if (noDelItems > m_actualSize)
            {
                noDelItems = m_actualSize;
            }

            size_t insertIndex      = indexFromIterator (realFirst);
            size_t iteratingTimes   = m_actualSize - (insertIndex + noDelItems);

            size_t i = 0;
            auto p = m_strdata.get();

            for (i = 0; i < iteratingTimes; i++)
            {
                if ( (realFirst + i + noDelItems) < end()) // we are at the end
                {
                    // relocate items as long as we do not reach the end
                    p[insertIndex + i] = p[insertIndex + i + noDelItems];
                }
            }

            m_actualSize -= noDelItems;
            p[m_actualSize] = EOS;
            return iterator (data() + insertIndex + i);
        }
        return begin();
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (const_iterator start, const_iterator end, InputIterator_first first, InputIterator_last last)
    {
        size_t corrInput = (last - first);

        if (last < first)
        {
            corrInput = (first - last);
        }

        size_t numItems = (corrInput < static_cast<size_t> (end - start)) ? corrInput : static_cast<size_t> (end - start);

        return replace (start, first, first + numItems);
    }

    template <class T>
    template <typename InputIterator_start, typename InputIterator_end, typename ContainerType,
              typename, typename, typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (InputIterator_start start, InputIterator_end end, const ContainerType &v)
    {
        return replace (start, end, v.begin(), v.end());
    }

    template <class T>
    template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
              typename, typename, typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (InputIterator_pos position, InputIterator_first first, InputIterator_last last)
    {
        assert (position <= end());

        // return position
        size_t index = 0;
        size_t numItems = (last - first) < 0 ? first - last : last - first;

        // realloc is needed
        if (m_allocationSize < (indexFromIterator (position) + numItems + NUL_TERMINATOR_LENGTH))
        {
            // allocate a bigger block and move data
            m_actualSize = 0;
            m_allocationSize = (indexFromIterator (position) + numItems) + ( (indexFromIterator (position) + numItems) / 2) + NUL_TERMINATOR_LENGTH;

            std::shared_ptr<T> newBuffer (new T[m_allocationSize], std::default_delete<T[]>());

            // copy old data, till the replace point is reached
            size_t i = 0;
            for (i = 0; i < indexFromIterator (position); ++i)
            {
                newBuffer.get() [i] = std::move (operator[] (i));
                m_actualSize++;
            }

            index = m_actualSize;

            // insert given data
            for (InputIterator_first iter = first; numItems > 0; ++iter, numItems--)
            {
                newBuffer.get() [m_actualSize] = *iter;
                m_actualSize++;
            }

            // end string
            newBuffer.get() [m_actualSize] = EOS;

            m_strdata.reset();
            m_strdata = newBuffer;
        }

        // realloc is not needed
        else
        {
            // overwrite data
            index = indexFromIterator (position);
            auto p = m_strdata.get();
            for (size_t offset = 0; offset < numItems; offset++)
            {
                p [index + offset] = * (first + offset);
            }

            m_actualSize = (index + numItems > m_actualSize) ? index + numItems : m_actualSize;
            p [m_actualSize] = EOS;
        }
        return basicString<T>::iterator (data() + index);
    }

    template <class T>
    template <typename InputIterator_pos, typename ContainerType,
              typename, typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (InputIterator_pos position, const ContainerType &v)
    {
        return replace (position, v.begin(), v.end());
    }

    template <class T>
    template <typename ContainerType,
              typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (size_t index, const ContainerType &v)
    {
        return replace (iteratorFromIndex (index), v);
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    typename basicString<T>::iterator basicString<T>::replace (size_t index, InputIterator_first first, InputIterator_last last)
    {
        return replace (iteratorFromIndex (index), first, last);
    }

    template <class T>
    int basicString<T>::compare (const basicString<T> &str) const
    {
        return compare (0, length(), str.c_str(), 0, str.length());
    }

    template <class T>
    int basicString<T>::compare (size_t pos, size_t len, const basicString<T> &str) const
    {
        return compare (pos, len, str.c_str(), 0, str.length());
    }

    template <class T>
    int basicString<T>::compare (size_t pos, size_t nlen, const basicString<T> &str, size_t subpos, size_t sublen) const
    {
        return compare (pos, nlen, str.c_str(), subpos, sublen);
    }

    template <class T>
    int basicString<T>::compare (size_t pos, size_t nlen, const char *s, size_t subpos, size_t sublen) const
    {
        // if both strings are empty, return equal
        if (nlen == 0 && sublen == 0)
        {
            return 0;
        }
        else if (length() == 0 && sublen > 0)
        {
            return static_cast<int> (sublen);
        }
        else if (sublen == 0 && length() > 0)
        {
            return -static_cast<int> (nlen);
        }

        if (pos >= length() || subpos >= royale_strlen<T> (s))
        {
            throw std::out_of_range ("Index out of range!");
        }

        size_t s_len = 0;
        if (0 == (s_len = strlen (s)))
        {
            return -1;
        }

        if (nlen == 0 || sublen > nlen || pos > length() || pos + nlen > length() || (subpos + sublen > s_len))
        {
            return -1;
        }

        if (nlen > sublen)
        {
            return 1;
        }

        size_t i = 0;
        while (* (data() + pos + i) == * (s + subpos + i) && (i < sublen))
        {
            i++;
        }

        if (i == sublen)
        {
            return 0;
        }

        return * (s + subpos + i) - * (data() + pos + i);
    }

    template <class T>
    int basicString<T>::compare (const char *s) const
    {
        size_t s_len = 0;
        if (0 == (s_len = strlen (s)))
        {
            return -1;
        }
        return compare (0, length(), s, 0, s_len);
    }

    template <class T>
    int basicString<T>::compare (size_t pos, size_t nlen, const char *s, size_t n) const
    {
        return compare (pos, nlen, s, 0, n);
    }

    template <class T>
    basicString<T> basicString<T>::substr (size_t pos, size_t len) const
    {
        if (pos >= m_actualSize)
        {
            throw std::out_of_range ("Index out of range!");
        }

        size_t subLen = (len + pos > length() ? length() - pos : len);

        return basicString<T> (this->data() + pos, subLen);
    }

    template<class T>
    T &basicString<T>::at (size_t index)
    {
        if (index < (m_actualSize + NUL_TERMINATOR_LENGTH))
        {
            return m_strdata.get() [index];
        }
        throw std::out_of_range ("index out of range");
    }

    template<class T>
    const T &basicString<T>::at (size_t index) const
    {
        if (index < (m_actualSize + NUL_TERMINATOR_LENGTH))
        {
            return m_strdata.get() [index];
        }
        throw std::out_of_range ("index out of range");
    }

    template<class T>
    T &basicString<T>::operator[] (size_t index)
    {
        return m_strdata.get() [index];
    }

    template<class T>
    const T &basicString<T>::operator[] (size_t index) const
    {
        return m_strdata.get() [index];
    }

    template<typename T>
    basicString<T> &basicString<T>::operator= (const basicString<T> &str)
    {
        if (this != &str)
        {
            // Free the existing resource.
            m_strdata.reset();

            m_actualSize = str.m_actualSize;
            if (m_actualSize)
            {
                m_allocationSize = str.m_actualSize + NUL_TERMINATOR_LENGTH;
                m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

                for (size_t i = 0; i < str.m_actualSize; ++i)
                {
                    m_strdata.get() [i] = str.m_strdata.get() [i];
                }
                m_strdata.get() [str.m_actualSize] = EOS;
            }
            else
            {
                m_allocationSize = NUL_TERMINATOR_LENGTH;
                m_strdata = getEmptyString();
            }
        }
        return *this;
    }

    template<typename T>
    basicString<T> &basicString<T>::operator= (const std::basic_string<T> &str)
    {
        // Free the existing resource.
        m_strdata.reset();

        m_actualSize = str.length();
        if (m_actualSize)
        {
            m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                m_strdata.get() [i] = str[i];
            }
            m_strdata.get() [m_actualSize] = EOS;
        }
        else
        {
            m_allocationSize = NUL_TERMINATOR_LENGTH;
            m_strdata = getEmptyString();
        }

        return *this;
    }

    template<typename T>
    basicString<T> &basicString<T>::operator= (const T *str)
    {
        // Free the existing resource.
        m_strdata.reset();

        m_actualSize = royale_strlen<T> (str);
        if (m_actualSize)
        {
            m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
            m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                m_strdata.get() [i] = * (str + i);
            }
            m_strdata.get() [m_actualSize] = EOS;
        }
        else
        {
            m_allocationSize = NUL_TERMINATOR_LENGTH;
            m_strdata = getEmptyString();
        }

        return *this;
    }

    template<typename T>
    basicString<T> &basicString<T>::operator= (const T str)
    {
        // Free the existing resource.
        m_strdata.reset();

        m_actualSize = 1;
        m_allocationSize = m_actualSize + NUL_TERMINATOR_LENGTH;
        m_strdata = std::shared_ptr<T> (new T[m_allocationSize], std::default_delete<T[]>());

        m_strdata.get() [0] = str;
        m_strdata.get() [m_actualSize] = EOS;

        return *this;
    }

    template<typename T>
    basicString<T> &basicString<T>::operator= (basicString<T> &&str)
    {
        if (this != &str)
        {
            classswap (*this, str);
        }
        return *this;
    }

    template<typename T>
    bool basicString<T>::operator== (const std::basic_string<T> &str) const
    {
        if (length() != str.length())
        {
            return false;
        }

        for (size_t i = 0; i < str.length(); ++i)
        {
            if (at (i) != str[i])
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    bool basicString<T>::operator== (const basicString<T> &str) const
    {
        if (length() != str.length())
        {
            return false;
        }

        for (size_t i = 0; i < str.length(); ++i)
        {
            if (at (i) != str.at (i))
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    bool basicString<T>::operator== (const T *str) const
    {
        if (length() != royale_strlen<T> (str))
        {
            return false;
        }

        for (size_t i = 0; i < royale_strlen<T> (str); ++i)
        {
            if (at (i) != str[i])
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    bool basicString<T>::operator!= (const std::basic_string<T> &str) const
    {
        return !operator== (str);
    }

    template<typename T>
    bool basicString<T>::operator!= (const basicString<T> &str) const
    {
        return !operator== (str);
    }

    template<typename T>
    bool basicString<T>::operator!= (const T *str) const
    {
        return !operator== (str);
    }

    template<typename T>
    bool basicString<T>::operator< (const basicString<T> &str) const
    {
        auto l = begin();
        auto r = str.begin();
        auto le = end();
        auto re = str.end();

        while ( (l != le) && (r != re))
        {
            if (*l < *r)
            {
                return true;
            }
            if (*r < *l)
            {
                return false;
            }
            ++l;
            ++r;
        }
        if (r == re)
        {
            return false;
        }
        return true;
    }

    template <class T>
    void basicString<T>::clear()
    {
        m_strdata = getEmptyString();
        m_allocationSize = NUL_TERMINATOR_LENGTH;
        m_actualSize = 0;
    }

    template<class T>
    void basicString<T>::swap (basicString<T> &string)
    {
        classswap (*this, string);
    }

    template<class T>
    const T *basicString<T>::c_str() const
    {
        return data();
    }

    template<class T>
    std::basic_string<T> basicString<T>::toStdString() const
    {
        return basicString<T>::toStdString (*this);
    }

    template<class T>
    std::basic_string<T> basicString<T>::toStdString (const basicString<T> &str)
    {
        if (str.empty())
        {
            return std::basic_string<T>();
        }
        return std::basic_string<T> (str.c_str());
    }

    template<class T>
    basicString<T> basicString<T>::fromStdString (const std::basic_string<T> &str)
    {
        return basicString<T> (str);    // C'tor handles str.empty() == true case
    }

    template<class T>
    basicString<T> basicString<T>::fromCArray (const T *str)
    {
        return basicString<T> (str);    // C'tor handles str.empty() == true case
    }

    template<typename T>
    std::ostream &operator<< (std::ostream &os, const basicString<T> &str)
    {
        if (!str.empty())
        {
            os << str.c_str();
        }
        return os;
    }

    template<typename T>
    typename basicString<T>::iterator basicString<T>::begin()
    {
        return iterator (m_strdata.get());
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::begin() const
    {
        return const_iterator (m_strdata.get());
    }

    template<typename T>
    typename basicString<T>::iterator basicString<T>::end()
    {
        return iterator (m_strdata.get() + m_actualSize);
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::end() const
    {
        return const_iterator (m_strdata.get() + m_actualSize);
    }

    template<typename T>
    typename basicString<T>::reverse_iterator basicString<T>::rbegin()
    {
        return reverse_iterator (m_strdata.get() + m_actualSize - 1);
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::rbegin() const
    {
        return const_reverse_iterator (m_strdata.get() + m_actualSize - 1);
    }

    template<typename T>
    typename basicString<T>::reverse_iterator basicString<T>::rend()
    {
        return reverse_iterator (m_strdata.get() - 1);
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::rend() const
    {
        return const_reverse_iterator (m_strdata.get() - 1);
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::cbegin()
    {
        return const_iterator (begin());
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::cbegin() const
    {
        return const_iterator (begin());
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::cend()
    {
        return const_iterator (end());
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::cend() const
    {
        return const_iterator (end());
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::crbegin()
    {
        return const_reverse_iterator (rbegin());
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::crbegin() const
    {
        return const_reverse_iterator (rbegin());
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::crend()
    {
        return const_reverse_iterator (rend());
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::crend() const
    {
        return const_reverse_iterator (rend());
    }

    template<typename T>
    typename basicString<T>::iterator basicString<T>::iteratorFromIndex (size_t index)
    {
        return iterator (data() + index);
    }

    template<typename T>
    typename basicString<T>::const_iterator basicString<T>::iteratorFromIndex (size_t index) const
    {
        return const_iterator (data() + index);
    }

    template<typename T>
    typename basicString<T>::reverse_iterator basicString<T>::reverseIteratorFromIndex (size_t index)
    {
        return reverse_iterator (data() + index);
    }

    template<typename T>
    typename basicString<T>::const_reverse_iterator basicString<T>::reverseIteratorFromIndex (size_t index) const
    {
        return const_reverse_iterator (data() + index);
    }

    typedef basicString<char> String;
    typedef basicString<wchar_t> WString;

    /* Declare static members */
    template<typename T>
    const size_t basicString<T>::npos;

    template<typename T>
    const T basicString<T>::EOS;
}
