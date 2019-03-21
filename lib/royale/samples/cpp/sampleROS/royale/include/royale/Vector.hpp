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

// disable getenv warnings under Windows
#ifdef WIN32
#pragma warning(disable:4544)
#endif

#include <royale/Definitions.hpp>
#include <royale/Iterator.hpp>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>
#include <royale/Pair.hpp>
#include <cassert>

namespace royale
{
    template<class T>
    class Vector
    {
        using V_TYPE        = char;

    public:
        /**
        * Iterator definitions
        */
        typedef royale::iterator::royale_iterator<std::random_access_iterator_tag, T> iterator;
        typedef royale::iterator::royale_const_iterator<std::random_access_iterator_tag, T> const_iterator;
        typedef royale::iterator::royale_reverse_iterator< iterator > reverse_iterator;
        typedef royale::iterator::royale_const_reverse_iterator< const_iterator > const_reverse_iterator;

        /**
        * General Vector definitions
        */
        typedef const T *const_pointer;
        typedef const T &const_reference;
        typedef ptrdiff_t pdifference_type;
        typedef T *pointer;
        typedef size_t size_type;
        typedef T value_type;
        typedef value_type &reference;

        /**
        * General constructor, which does not allocate memory and sets everything to it's default
        */
        Vector<T>();

        /**
        * Constructor which already performs an allocation of memory during execution
        * Constructs a given number of elements
        *
        * \param size Number of elements to construct
        */
        explicit Vector<T> (size_t size);

        /**
        * Constructor which already performs an allocation of memory during execution
        * Constructs the given number of elements pre-initialized with the value of item
        *
        * \param n Number of elements to construct
        * \param item The pre initialization value
        */
        explicit Vector<T> (size_t n, const T &item);

        /**
        * Copy-Constructor for royale compliant vector
        * which allows creation of a royale compliant vector from
        * another royale compliant vector - (NOTE: performs a deep copy!)
        *
        * \param v The royale vector which's memory shall be copied
        */
        Vector<T> (const Vector<T> &v);

        /**
        * Move-Constructor for royale compliant vector
        * which allows creation of a royale compliant vector by moving memory
        * (NOTE: performs a shallow copy!)
        *
        * \param v The royale vector which's memory shall be moved
        */
        Vector<T> (Vector<T> &&v);

        /**
        * Copy-Constructor for STL compliant vector (std::vector)
        * It allows creation of a royale compliant vector from
        * a STL compliant vector - (NOTE: performs a deep copy!)
        *
        * \param v The STL vector to copy
        */
        Vector<T> (const std::vector<T> &v);

        /**
        * Construct a vector with an input iterator
        *
        * \param first the First element returned by the iterator
        * \param last the Last element returned by the iterator
        */
        template <class InputIterator>
        Vector<T> (InputIterator first, InputIterator last);

        /**
        * Initializer list initialization to initialize a vector
        *
        * \param list The list of values
        */
        Vector<T> (const std::initializer_list<T> &list);

        /**
        * Destructor
        *
        * Clears the vectors allocated memory by performing deletion
        */
        virtual ~Vector<T>();

        /**
        * Returns the actual size of the vector (this is the used amount of
        * slots in the allocated area) equivalent to count()
        *
        * \return size_t The amount of the actual used memory slots within the vector
        */
        size_t size() const;

        /**
        * Returns the actual size of the vector (this is the used amount of
        * slots in the allocated area) equivalent to size()
        *
        * \return size_t The amount of the actual used memory slots within the vector
        */
        size_t count() const;

        /**
        * Checks if the vector is empty
        *
        * \return bool Returns true if the vector is empty - otherwise false
        */
        bool empty() const;

        /**
        * Returns the amount of allocated slots which are maintained by the vector
        * These allocated slots might be used or unused (refer to size() for checking the size()
        * itself)
        *
        * \return size_t The amount of allocated slots for the element type which is bound to the vector
        */
        size_t capacity() const;

        /**
        * Returns a direct pointer to the memory array used internally by the vector to store its owned elements.
        * Because elements in the vector are guaranteed to be stored in contiguous storage locations in the same order as
        * represented by the vector, the pointer retrieved can be offset to access any element in the array.
        *
        * \return T* A pointer to the first element in the array used internally by the vector.
        */
        T *data();

        /**
        * Returns a direct pointer to the memory array used internally by the vector to store its owned elements.
        * Because elements in the vector are guaranteed to be stored in contiguous storage locations in the same order as
        * represented by the vector, the pointer retrieved can be offset to access any element in the array.
        *
        * \return T* A pointer to the first element in the array used internally by the vector.
        */
        const T *data() const;

        /**
        * Returns a reference to the first element in the vector.
        * This member function returns a direct reference to the first element in the vector
        * Calling this function on an empty vector will result in a std::out_of_range exception.
        *
        * \return T& A reference to the first element in the vector.
        * \throws std::out_of_range Exception if the vector is empty
        */
        T &front();
        const T &front() const;

        /**
        * Returns a reference to the first element in the vector (see also front()).
        * This member function returns a direct reference to the first element in the vector
        * Calling this function on an empty vector will result in a std::out_of_range exception.
        *
        * \return T& A reference to the first element in the vector.
        * \throws std::out_of_range Exception if the vector is empty
        */
        T &first();
        const T &first() const;

        /**
        * Returns true if the vector starts with the given value
        *
        * \return bool True if the vector starts with the given value
        */
        bool startsWith (const T &value);
        bool startsWith (const T &value) const;

        /**
        * Returns a reference to the last element in the vector.
        * This member function returns a direct reference to the last element in the vector
        * Calling this function on an empty vector will result in a std::out_of_range exception.
        *
        * \return T& A reference to the last element in the vector.
        * \throws std::out_of_range Exception if the vector is empty
        */
        T &back();
        const T &back() const;

        /**
        * Returns a reference to the last element in the vector (see also back()).
        * This member function returns a direct reference to the last element in the vector
        * Calling this function on an empty vector will result in a std::out_of_range exception.
        *
        * \return T& A reference to the last element in the vector.
        * \throws std::out_of_range Exception if the vector is empty
        */
        T &last();
        const T &last() const;

        /**
        * Returns true if the vector ends with the given value
        *
        * \return bool True if the vector ends with the given value
        */
        bool endsWith (const T &value);
        bool endsWith (const T &value) const;

        /**
        * Returns an iterator to the first position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the begin of the vector.
        */
        iterator begin();
        const_iterator begin() const;

        /**
        * Returns an iterator to the last position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the end of the vector.
        */
        iterator end();
        const_iterator end() const;

        /**
        * Returns an iterator to the last position (reverse begin)
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the reverse begin of a vector (!= end())
        */
        reverse_iterator rbegin();
        const_reverse_iterator rbegin() const;

        /**
        * Returns an iterator to the last position (reverse end)
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the reverse end of a vector (!= begin())
        */
        reverse_iterator rend();
        const_reverse_iterator rend() const;

        /**
        * Returns an constant iterator to the first position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the begin of the vector.
        */
        const_iterator cbegin();
        const_iterator cbegin() const;

        /**
        * Returns an constant iterator to the last position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator An iterator pointing to the end of the vector.
        */
        const_iterator cend();
        const_iterator cend() const;

        /**
        * Returns a constant reverse iterator to the first position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator A constant iterator pointing to the reverse begin of the vector.
        */
        const_reverse_iterator crbegin();
        const_reverse_iterator crbegin() const;

        /**
        * Returns a constant reverse iterator to the last position
        * Calling this function on an empty vector will result in undefined behavior
        *
        * \return iterator A constant iterator pointing to the reverse end of the vector.
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
        * Adds a new element at the end of the vector, after its current last element.
        * The content of v is copied to the new element (NOTE: a deep copy is performed!).
        * This effectively increases the container size by one, which causes an automatic reallocation
        * of the allocated storage space if -and only if- the new vector size surpasses the current vector capacity.
        */
        void push_back (const T &v);
        void push_back (T &&v);

        /**
        * Assigns new contents to the vector, replacing its current contents, and modifying its size accordingly.
        * Any elements held in the container before the call are destroyed or replaced by newly constructed elements(no assignments of elements take place).
        * This causes an automatic reallocation of the allocated storage space if - and only if - the new vector size surpasses the current vector capacity.
        */
        void assign (size_t n, const T &val);
        void assign (std::initializer_list<value_type> il);

        template <typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        void assign (InputIterator_first first, InputIterator_last last);

        /**
        * Inserts elements to the given vector
        * The content at the given position is moved backwards in the vector to create space
        * for the elements which shall be inserted - given there is enough allocated space
        * If the allocation is too small to insert the bunch of data, a new block is allocated,
        * while the old data is MOVED (std::move) to the new buffer as long as the insert position is not reached.
        * When the input position is reached, the input values are completely copied.
        * At the end the original data, after the input position is moved to the new buffer and the
        * internal data buffer is swapped with the created buffer.
        */
        template < typename InputIterator_pos, typename VectorType,
                   typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                   typename = typename std::enable_if < !std::is_convertible<VectorType, T>::value >::type >
        iterator insert (InputIterator_pos position, const VectorType &v);

        template < typename VectorType,
                   typename = typename std::enable_if < !std::is_convertible<VectorType, T>::value >::type >
        iterator insert (size_t index, const VectorType &v);

        template <typename InputIterator_pos, typename ItemType,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                  typename = typename std::enable_if<std::is_convertible<ItemType, T>::value>::type,
                  typename = void> // dummy to make the functions different templates
        iterator insert (InputIterator_pos position, const ItemType &item);

        template <typename ItemType,
                  typename = typename std::enable_if<std::is_convertible<ItemType, T>::value>::type,
                  typename = void> // dummy to make the functions different templates
        iterator insert (size_t index, const ItemType &item);

        template <typename InputIterator_pos, typename ItemType,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                  typename = typename std::enable_if<std::is_convertible<ItemType, T>::value>::type,
                  typename = void> // dummy to make the functions different templates
        iterator insert (InputIterator_pos position, size_type n, const ItemType &item);

        template <typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator insert (size_t index, InputIterator_first first, InputIterator_last last);

        template <typename InputIterator_pos,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type>
        iterator insert (InputIterator_pos position, std::initializer_list<value_type> il);

        template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_first>::value>::type,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_last>::value>::type>
        iterator insert (InputIterator_pos position, InputIterator_first first, InputIterator_last last);

        /**
        * Construct and insert element
        * The container is extended by inserting a new element at position.
        * This new element is constructed in place using args as the arguments for its construction.
        * This effectively increases the container size by one.
        * An automatic reallocation of the allocated storage space happens if -and only if- the
        * new vector size surpasses the current vector capacity.
        *
        * Because vectors use an array as their underlying storage, inserting elements in positions other than the vector end
        * causes the container to shift all the elements that were after position by one to their new positions.
        * This is generally an inefficient operation compared to the one performed by other kinds of sequence containers (such as list or forward_list).
        * See emplace_back for a member function that extends the container directly at the end.
        *
        * The element is constructed in-place by calling allocator_traits::construct with args forwarded.
        *
        * A similar member function exists, insert, which either copies or moves existing objects into the container.
        *
        * \param position The position where the arguments shall be inserted
        * \param ts The arguments which shall be appended to the end
        * \return void
        */
        template <class... Args>
        iterator emplace (const_iterator position, Args &&... ts);

        /**
        * Construct and insert element at the end
        * Inserts a new element at the end of the vector, right after its current last element.
        * This new element is constructed in place using args as the arguments for its constructor.
        *
        * This effectively increases the container size by one, which causes an automatic reallocation of the allocated storage
        * space if - and only if - the new vector size surpasses the current vector capacity.
        * The element is constructed in - place by calling placement new with args forwarded.
        * A similar member function exists, push_back, which either copies or moves an existing object into the container.
        * see also insert() and emplace()
        *
        * \param ts The arguments which shall be appended to the end
        * \return void
        */
        template <class... Args>
        void emplace_back (Args &&... ts);

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
                   typename = typename std::enable_if < (std::is_same<ContainerType, std::vector<T>>::value || std::is_same<ContainerType, Vector<T>>::value) >::type >
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
                   typename = typename std::enable_if < (std::is_same<ContainerType, std::vector<T>>::value || std::is_same<ContainerType, Vector<T>>::value) >::type >
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
                   typename = typename std::enable_if < (std::is_same<ContainerType, std::vector<T>>::value || std::is_same<ContainerType, Vector<T>>::value) >::type >
        iterator replace (size_t index, const ContainerType &v);

        /**
        * Replace elements in a vector
        * This method allows replacing the elements starting at index with the given range
        * from "first" to "last".
        * It only replaces as much items (starting at first) as forseen by the range from "first" to "last".
        * If the number of elements given by "first" and "last" exceeds the allocation size
        * a realoocation is performed and the elements are moved to the new allocation space.
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
        * Erase elements
        * Erases from the vector either a single element (position) or a range of elements ([first,last)).
        * This effectively reduces the container size by the number of elements removed, which are destroyed.
        *
        * Because vectors use an array as their underlying storage, erasing elements in positions other than
        * the vector end causes the container to relocate all the elements after the segment erased to their new positions.
        * This is generally an inefficient operation compared to the one performed for the same operation
        * by other kinds of sequence containers (such as list or forward_list).
        *
        * A faster operation is remove()
        * \return iterator The iterator position to the first element after the deleted range
        */
        iterator erase (const_iterator first, const_iterator last);
        iterator erase (size_t index, size_t numElements = 1);
        iterator erase (const_iterator position, size_t numElements = 1);

        /**
        * Takes a range of items out of a vector (relocates the other elements to fill the gap)
        * The element which was taken out is copied and returned, while
        * the original element in the original vector is erased using erase() - which calls the destructor.
        */
        template <typename InputIterator_pos,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type>
        T takeItem (InputIterator_pos position);
        T takeItem (size_t index);

        /**
        * Takes a range of items out of a vector (relocates the other elements to fill the gap)
        * The elements which were take out are MOVED to another vector storage and returned, while
        * they are erased (with erase()) in the original vector - which calls the destructor.
        */
        template <typename InputIterator_pos,
                  typename = typename std::enable_if<royale::iterator::is_iterator<InputIterator_pos>::value>::type>
        Vector<T> takeItems (InputIterator_pos position, size_t numElements);
        Vector<T> takeItems (size_t index, size_t numElements);

        /**
        * Looks up a specific item in the vector and returns the index
        *
        * \param item The item to search for
        * \param occurance Return the n'th occurance of the item (everything below 2 means first occurance)
        * \return position The position at which the Item was found or npos if it wasn't found
        */
        size_t indexOf (const value_type &item, size_t occurance = 1) const;

        /**
        * Looks up a specific item in the vector and returns true or false
        *
        * \param item The item to search for
        * \return bool Returns true if the vector contains an equal item or false otherwise
        */
        bool contains (const value_type &item) const;

        /**
        * Removes the last element in the vector, effectively reducing the container size by one.
        * The last element is destroyed by calling it's destructor; the size is reduced by one BUT
        * there is no reallocation performed to resize the vector to it's contents or to reduce the
        * the vectors capacity by one.
        * The allocated space remains the same.
        */
        void pop_back();

        /**
        * Returns a reference to the element at position index in the vector container.
        * A similar member function, at(), has the same behavior as this operator function,
        * except that at() is bound-checked and signals if the requested position is
        * out of range by throwing an out_of_range exception.
        *
        * \param index The index to access within the vectors storage
        * \return T& The Reference to the elements at the specified position in the vector
        */
        T &operator[] (size_t index);

        /**
        * Returns a reference to the element at position index in the vector container.
        * A similar member function, at(), has the same behavior as this operator function,
        * except that at() is bound-checked and signals if the requested position is
        * out of range by throwing an out_of_range exception.
        *
        * \param index The index to access within the vectors storage
        * \return T& The Reference to the elements at the specified position in the vector
        */
        const T &operator[] (size_t index) const;

        /**
        * The function automatically checks whether index is within the bounds
        * of valid elements in the vector, throwing an out_of_range exception
        * if it is not (i.e., if index is greater or equal than its size).
        * This is in contrast with member operator[], that does not check against bounds.
        *
        * \param index The index to access within the vectors storage
        * \return T& The Reference to the elements at the specified position in the vector
        */
        T &at (size_t index);

        /**
        * The function automatically checks whether index is within the bounds
        * of valid elements in the vector, throwing an out_of_range exception
        * if it is not (i.e., if index is greater or equal than its size).
        * This is in contrast with member operator[], that does not check against bounds.
        *
        * \param index The index to access within the vectors storage
        * \return T& The Reference to the elements at the specified position in the vector
        */
        const T &at (size_t index) const;

        /**
        * Assign another royale compliant vector.
        * Assigns new contents to the container, replacing its current contents,
        * and modifying its size accordingly.
        * This method copies all elements held by v into the container
        *
        * \param v A vector of the same storage type
        * \return Vector<T>& Returns *this
        */
        Vector<T> &operator= (Vector<T> v);

        /**
        * Assign the contents of an STL compliant vector container by
        * replacing container's current contents if necessary
        * and modifying its size accordingly.
        * This method copies all elements held by v into the container
        *
        * \param v An STL compliant vector of the same storage type
        * \return Vector<T>& Returns *this
        */
        Vector<T> &operator= (const std::vector<T> &v);

        /**
        * Checks equality with an STL compliant vector
        *
        * \param v An STL compliant vector of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const std::vector<T> &v) const;

        /**
        * Checks equality with a royale compliant vector
        *
        * \param v An royale compliant vector of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const Vector<T> &v) const;

        /**
        * Checks unequality with an STL compliant vector
        *
        * \param v An STL compliant vector of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const std::vector<T> &v) const;

        /**
        * Checks unequality with a royale compliant vector
        *
        * \param v An royale compliant vector of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const Vector<T> &v) const;

        /**
        * Removes all elements from the vector (which are destroyed), leaving the container with a size of 0.
        * A reallocation is not performed and the vector's capacity is destroyed (everything is freed).
        */
        void clear();

        /**
        * Modifies the vector to the given allocation size and initializes the elements (it may shrink)
        *
        * Creates any amount of elements (allocates the memory already)
        * and moves the existing elements to these slots; afterwards the old space is dumped.
        *
        * If the given newSize is smaller than the already used slots, the vector will shrink.
        * This means that all elements which are not covered within this capacity (the last ones) will be
        * deleted.
        *
        * \param newSize The amount of slots to remain in the vector (might shrink or enlarge the vector)
        */
        void resize (size_t newSize);

        /**
        * Modifies the vector to the given allocation size and initializes the elements (it may shrink)
        *
        * Creates any amount of elements (allocates the memory already)
        * and moves the existing elements to these slots; afterwards the old space is dumped.
        *
        * If the given newSize is smaller than the already used slots, the vector will shrink.
        * This means that all elements which are not covered within this capacity (the last ones) will be
        * deleted.
        *
        * \param newSize The amount of slots to remain in the vector (might shrink or enlarge the vector)
        * \param initVal The initializer value by which the vector shall be initialized
        */
        void resize (size_t newSize, T initVal);

        /**
        * Extends the vector to a higher allocation size and allocates the buffers
        *
        * Reserves any amount of free allocation slots (allocates the memory already) to be later
        * used for the element-types bound to the given royale vector.
        *
        * If the given size to reserve is smaller than the already reserved space, then the function
        * return immediately; otherwise the necessary memory allocation is performed and the size is
        * extended to "size"
        +
        * \param size The size (number of element-types) of elements that should be allocated.
        */
        void reserve (size_t size);

        /**
        * Shrinks the vector's allocation to it's size
        * Changes the size of the allocated buffer to the vector's size
        * this may result in freeing unneeded memory allocation.
        */
        void shrink_to_fit();

        /**
        * Converts the given std::vector (STL) to the vector type used by the royale API
        *
        * \param v The STL vector which should be converted to the royale API vector format
        * \return The royale API compliant vector format
        */
        static Vector<T> fromStdVector (const std::vector<T> &v);

        /**
        * User convenience function to allow conversion to std::vector
        * which might be used outside the library by the application for further processing
        *
        * \return std::vector containing the items of the
        */
        std::vector<T> toStdVector();
        const std::vector<T> toStdVector() const;

        /**
        * User convenience function to allow conversion to std::vector
        * which might be used if the royale compliant Vector is a const
        *
        * \return std::vector containing the items of the
        */
        static std::vector<T> toStdVector (const Vector<T> &v);

        /**
        * Converts the given String (based on char or wchar_t) to a vector
        *
        * \param string The string to convert to a vector
        * \return The royale API compliant vector format
        */
        template<typename U>
        static typename std::enable_if <
        (std::is_same<typename U::value_type, char>::value || std::is_same<typename U::value_type, wchar_t>::value), Vector<T> >::type
        fromString (const U &string)
        {
            return Vector<T> (string.begin(), string.end());
        }

        /**
        * User convenience function to allow conversion from std::map
        * which might be used inside the library for further processing
        *
        * \param stdMap The STL compliant map which shall be converted to the API style vector
        * \return Vector<pair> Returned is a vector of pairs - each pair is holding one entry of the
        *                      converted std::map
        */
        template<typename X, typename Y>
        static Vector< Pair<X, Y> > fromStdMap (const std::map<X, Y> &stdMap);

        /**
        * User convenience function to allow conversion to std::map
        * which might be used outside the library by the application for further processing
        *
        * \return std::map containing the items of the Vector<pairs>
        */
        template<typename X, typename Y>
        std::map<X, Y> toStdMap();

        /**
        * User convenience function to allow conversion to std::map
        * which might be used if the element is of type const
        *
        * \return std::map containing the items of the Vector<pairs>
        */
        template<typename X, typename Y>
        static std::map<X, Y> toStdMap (const Vector< Pair<X, Y> > &v);

        /**
        * Allows a data swap (convenience function to call classswap)
        * Swaps the given vector data with the current one
        */
        void swap (Vector<T> &&vector);

        /**
        * Returns the max_size of the string
        *
        * \return size_t The maximum length of the string
        */
        size_t max_size() const;

    public:
        static const size_t npos = ( (size_t) ~0);

    private:
        template<typename U>
        friend std::ostream &operator<< (std::ostream &os, const Vector<U> &v);

        template<typename U>
        friend void classswap (Vector<U> &first, Vector<U> &second);

        inline void freeAllocation();

        void memAssign (size_t potentialIndex, const T &value);

        std::shared_ptr<V_TYPE> m_data;
        size_t m_allocationSize;
        size_t m_actualSize;

        const int GROWTH_FACTOR = 2;
    };

    template<class T>
    size_t Vector<T>::max_size() const
    {
        return npos;
    }

    // private method for memory management!
    template <class T>
    void Vector<T>::memAssign (size_t potentialIndex, const T &value)
    {
        if (potentialIndex >= m_allocationSize)
        {
            throw std::out_of_range ("memAssign(): potentialIndex exceeds allocationSize, unhandled case; reallocate larger block!");
        }

        if (potentialIndex >= m_actualSize)
        {
            new (data() + potentialIndex) T (value);
        }
        else
        {
            * (data() + potentialIndex) = value;
        }
    }

    template<typename T>
    void classswap (Vector<T> &first, Vector<T> &second)
    {
        std::swap (first.m_actualSize, second.m_actualSize);
        std::swap (first.m_allocationSize, second.m_allocationSize);
        std::swap (first.m_data, second.m_data);
    }

    //! Template implementation
    template<class T>
    Vector<T>::Vector() :
        m_data (nullptr),
        m_allocationSize (0),
        m_actualSize (0)
    { }

    template<class T>
    Vector<T>::Vector (size_t size) :
        m_data (std::shared_ptr<V_TYPE> (new V_TYPE[sizeof (T) * size], std::default_delete<V_TYPE[]>())),
        m_allocationSize (size),
        m_actualSize (size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            new (data() + i) T();
        }
    }

    template<class T>
    Vector<T>::Vector (size_t n, const T &item) :
        m_data (std::shared_ptr<V_TYPE> (new V_TYPE[sizeof (T) * n], std::default_delete<V_TYPE[]>())),
        m_allocationSize (n),
        m_actualSize (n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            new (data() + i) T (item);
        }
    }

    template<class T>
    Vector<T>::Vector (const Vector<T> &v) :
        m_data (std::shared_ptr<V_TYPE> (new V_TYPE[sizeof (T) * v.m_allocationSize], std::default_delete<V_TYPE[]>())),
        m_allocationSize (v.m_allocationSize),
        m_actualSize (v.m_actualSize)
    {
        for (size_t i = 0; i < m_actualSize; ++i)
        {
            new (data() + i) T (v[i]);
        }
    }

    template<class T>
    Vector<T>::Vector (const std::vector<T> &v) :
        m_data (std::shared_ptr<V_TYPE> (new V_TYPE[sizeof (T) * v.size()], std::default_delete<V_TYPE[]>())),
        m_allocationSize (v.size()),
        m_actualSize (v.size())
    {
        for (size_t i = 0; i < m_actualSize; ++i)
        {
            new (data() + i) T (v[i]);
        }
    }

    template<typename T>
    Vector<T>::Vector (Vector<T> &&v) :
        Vector<T>()
    {
        classswap (*this, v);
    }

    template <typename T>
    template <class InputIterator>
    Vector<T>::Vector (InputIterator first, InputIterator last) :
        m_data (std::shared_ptr<V_TYPE> (new V_TYPE[sizeof (T) * (last - first)], std::default_delete<V_TYPE[]>())),
        m_allocationSize ( (last - first)),
        m_actualSize ( (last - first))
    {
        for (InputIterator i = first; i != last; ++i)
        {
            new (data() + (m_allocationSize - (last - i))) T (*i);
        }
    }

    template<typename T>
    Vector<T>::Vector (const std::initializer_list<T> &list) :
        m_data (nullptr),
        m_allocationSize (0),
        m_actualSize (0)
    {
        reserve (list.size());
        for (auto itm : list)
        {
            push_back (itm);
        }
    }

    template<class T>
    Vector<T>::~Vector()
    {
        clear();
        freeAllocation();
    }

    template<typename T>
    T *Vector<T>::data()
    {
        return reinterpret_cast<T *> (m_data.get());
    }

    template<typename T>
    const T *Vector<T>::data() const
    {
        return reinterpret_cast<T *> (m_data.get());
    }

    template<typename T>
    size_t Vector<T>::size() const
    {
        return m_actualSize;
    }

    template<typename T>
    size_t Vector<T>::count() const
    {
        return m_actualSize;
    }

    template<typename T>
    bool Vector<T>::empty() const
    {
        return m_actualSize == 0;
    }

    template<typename T>
    size_t Vector<T>::capacity() const
    {
        return m_allocationSize;
    }

    template<class T>
    T &Vector<T>::front()
    {
        if (m_data == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return data() [0];
    }

    template<class T>
    const T &Vector<T>::front() const
    {
        if (m_data == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return data() [0];
    }

    template<class T>
    T &Vector<T>::first()
    {
        return front();
    }

    template<class T>
    const T &Vector<T>::first() const
    {
        return front();
    }

    template<class T>
    bool Vector<T>::startsWith (const T &value)
    {
        return front() == value;
    }

    template<class T>
    bool Vector<T>::startsWith (const T &value) const
    {
        return front() == value;
    }

    template<class T>
    T &Vector<T>::back()
    {
        if (m_data == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return data() [m_actualSize - 1];
    }

    template<class T>
    const T &Vector<T>::back() const
    {
        if (m_data == nullptr || empty() == true)
        {
            throw std::out_of_range ("index out of range");
        }
        return data() [m_actualSize - 1];
    }

    template<class T>
    T &Vector<T>::last()
    {
        return back();
    }

    template<class T>
    const T &Vector<T>::last() const
    {
        return back();
    }

    template<class T>
    bool Vector<T>::endsWith (const T &value)
    {
        return back() == value;
    }

    template<class T>
    bool Vector<T>::endsWith (const T &value) const
    {
        return back() == value;
    }

    template<class T>
    size_t Vector<T>::indexOf (const typename Vector<T>::value_type &item, size_t occurance) const
    {
        size_t nOccurance = (occurance < 2 ? 1 : occurance);
        size_t occuranceCount = 0;

        for (size_t i = 0; i < m_actualSize; ++i)
        {
            if (operator[] (i) == item)
            {
                occuranceCount++;
            }
            if (occuranceCount == nOccurance)
            {
                return i;
            }
        }
        return npos;
    }

    template<class T>
    bool Vector<T>::contains (const typename Vector<T>::value_type &item) const
    {
        return indexOf (item) != npos;
    }

    template<class T>
    void Vector<T>::pop_back()
    {
        if (m_actualSize > 0)
        {
            (data() [m_actualSize - 1]).~T();
            --m_actualSize;

            if (m_actualSize == 0)
            {
                freeAllocation();
            }
        }
    }

    template<class T>
    void Vector<T>::push_back (const T &v)
    {
        if (m_actualSize >= m_allocationSize)
        {
            reserve (m_allocationSize == 0 ? 1 : (m_allocationSize * GROWTH_FACTOR));
        }
        new (data() + m_actualSize) T (v);
        m_actualSize++;
    }

    template<class T>
    void Vector<T>::push_back (T &&v)
    {
        if (m_actualSize >= m_allocationSize)
        {
            reserve (m_allocationSize == 0 ? 1 : (m_allocationSize * GROWTH_FACTOR));
        }
        new (data() + m_actualSize) T (std::move (v));
        m_actualSize++;
    }

    template<class T>
    template <class... Args>
    typename Vector<T>::iterator Vector<T>::emplace (const_iterator position, Args &&... ts)
    {
        return insert (position, T (std::forward<Args> (ts)...));
    }

    template<class T>
    template <class... Args>
    void Vector<T>::emplace_back (Args &&... ts)
    {
        insert (end(), T (std::forward<Args> (ts)...));
        return;
    }

    template <class T>
    template <typename InputIterator_pos, typename VectorType,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (InputIterator_pos position, const VectorType &v)
    {
        return insert (position, v.begin(), v.end());
    }

    template <class T>
    template <typename VectorType,
              typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (size_t index, const VectorType &v)
    {
        return insert (iterator (data() + index), v);
    }

    template <class T>
    template <typename InputIterator_pos, typename ItemType,
              typename, typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (InputIterator_pos position, const ItemType &item)
    {
        return insert (position, Vector<T> { item });
    }

    template <class T>
    template <typename ItemType,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (size_t index, const ItemType &item)
    {
        return insert (iterator (data() + index), item);
    }

    template <class T>
    template <typename InputIterator_pos, typename ItemType,
              typename, typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (InputIterator_pos position, size_t n, const ItemType &item)
    {
        return insert (position, Vector<T> (n, item));
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (size_t index, InputIterator_first first, InputIterator_last last)
    {
        return insert (iterator (data() + index), first, last);
    }

    template <class T>
    template <typename InputIterator_pos,
              typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (InputIterator_pos position, std::initializer_list<typename Vector<T>::value_type> il)
    {
        return insert (position, il.begin(), il.end());
    }

    template <class T>
    template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
              typename, typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::insert (InputIterator_pos position, InputIterator_first first, InputIterator_last last)
    {
        assert (position <= end());

        // return position
        size_t index = 0;
        size_t numItems = (last - first) < 0 ? first - last : last - first;

        // realloc is needed
        if (m_allocationSize < (numItems + size()))
        {
            // allocate a bigger block and move data
            size_t oldSize = m_actualSize;
            m_actualSize = 0;
            m_allocationSize = (numItems + (capacity() * GROWTH_FACTOR));

            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * m_allocationSize], std::default_delete<V_TYPE[]>());

            // copy old data, till the insert point is reached
            size_t i = 0;
            for (i = 0; i < indexFromIterator (position); ++i)
            {
                new (newBuffer.get() + sizeof (T) * i) T (std::move (operator[] (i)));
                (data() [i]).~T();
                m_actualSize++;
            }

            index = m_actualSize;

            // insert given data
            for (InputIterator_first iter = first; iter != last; ++iter)
            {
                new (newBuffer.get() + sizeof (T) * m_actualSize) T (*iter);
                m_actualSize++;
            }

            // insert old data at the end if there was any
            for (size_t iNew = i; iNew < oldSize; ++iNew)
            {
                new (newBuffer.get() + sizeof (T) * m_actualSize) T (std::move (operator[] (iNew)));
                (data() [iNew]).~T();
                m_actualSize++;
            }

            m_data.reset();
            m_data = newBuffer;
        }

        // realloc is not needed
        else
        {
            size_t iteratingTimes   = end() - position;
            size_t lastIndex        = m_actualSize - 1;

            // take the originally last item and place it at the new end
            const_reverse_iterator posChanger;
            for (posChanger = rbegin(); iteratingTimes > 0; posChanger++)
            {
                size_t offset = numItems + (posChanger - rbegin());

                // if we place items behind the actualSize, we need to use placement new
                // otherwise we have to use "="; memAssign manages these cases.
                memAssign (lastIndex + offset, *posChanger);

                iteratingTimes--;
            }

            // now we have as much space as needed in between
            index = indexFromIterator (position);
            for (size_t offset = 0; offset < numItems; offset++)
            {
                // if we place items behind the actualSize, we need to use placement new
                // otherwise we have to use "="; memAssign manages these cases.
                memAssign (index + offset, * (first + offset));
            }

            m_actualSize += numItems;
        }
        return Vector<T>::iterator (data() + index);
    }

    template <class T>
    void Vector<T>::assign (size_t n, const T &val)
    {
        // realloc is needed
        if (m_allocationSize < n)
        {
            // clear everything - needed to avoid memory leaks
            clear();
            freeAllocation();

            // allocate a bigger block and move data
            m_allocationSize = (n * GROWTH_FACTOR);

            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * m_allocationSize], std::default_delete<V_TYPE[]>());
            m_data = newBuffer;

            // insert given data
            for (size_t i = 0; i < n; ++i)
            {
                new (data() + i) T (val);
            }

            m_actualSize = n;
        }

        // realloc is not needed
        else
        {
            iterator beginPos (begin());
            iterator insertIterator (beginPos);

            // we use the available space
            while (insertIterator != (beginPos + n))
            {
                memAssign ( (insertIterator - beginPos), val);
                insertIterator++;
            }
            if (insertIterator < end())
            {
                erase (insertIterator, end());
            }

            m_actualSize = n;
        }
    }

    template <class T>
    void Vector<T>::assign (std::initializer_list<typename Vector<T>::value_type> il)
    {
        return assign (il.begin(), il.end());
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    void Vector<T>::assign (InputIterator_first first, InputIterator_last last)
    {
        // return position
        size_t numItems = (last - first) < 0 ? first - last : last - first;

        // realloc is needed
        if (m_allocationSize < numItems)
        {
            // clear everything - needed to avoid memory leaks
            clear();
            freeAllocation();

            // allocate a bigger block and move data
            m_allocationSize = (numItems * GROWTH_FACTOR);

            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * m_allocationSize], std::default_delete<V_TYPE[]>());
            m_data = newBuffer;

            // insert given data
            for (InputIterator_first iter = first; iter != last; ++iter)
            {
                new (data() + m_actualSize) T (*iter);
                m_actualSize++;
            }
        }

        // realloc is not needed
        else
        {
            for (size_t i = 0; i < numItems; i++)
            {
                memAssign (i, * (first + i));
            }

            // erase unused elements at the end
            if (numItems < m_actualSize)
            {
                erase (iteratorFromIndex (numItems), iteratorFromIndex (m_actualSize));
            }

            m_actualSize = numItems;
        }
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::replace (const_iterator start, const_iterator end, InputIterator_first first, InputIterator_last last)
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
    typename Vector<T>::iterator Vector<T>::replace (InputIterator_start start, InputIterator_end end, const ContainerType &v)
    {
        return replace (start, end, v.begin(), v.end());
    }

    template <class T>
    template <typename InputIterator_pos, typename InputIterator_first, typename InputIterator_last,
              typename, typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::replace (InputIterator_pos position, InputIterator_first first, InputIterator_last last)
    {
        assert (position <= end());

        // return position
        size_t index = 0;
        size_t numItems = last - first < 0 ? first - last : last - first;

        // realloc is needed
        if (m_allocationSize < (indexFromIterator (position) + numItems + 1))
        {
            // allocate a bigger block and move data
            m_allocationSize = (indexFromIterator (position) + numItems) * GROWTH_FACTOR;

            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * m_allocationSize], std::default_delete<V_TYPE[]>());

            // copy old data, till the insert point is reached
            size_t newSize = 0;
            for (newSize = 0; newSize < indexFromIterator (position); ++newSize)
            {
                new (newBuffer.get() + sizeof (T) * newSize) T (std::move (operator[] (newSize)));
                (data() [newSize]).~T();
            }

            index = newSize;

            // insert given data
            for (InputIterator_first iter = first; iter != last; ++iter)
            {
                new (newBuffer.get() + sizeof (T) * newSize) T (*iter);
                newSize++;
            }

            // need to delete unneeded items at the end to avoid memory leaks
            // There are elements left!
            if (iteratorFromIndex (index) < end())
            {
                erase (iteratorFromIndex (index), end());
            }

            m_data.reset();
            m_data = newBuffer;
            m_actualSize = newSize;
        }

        // realloc is not needed
        else
        {
            // overwrite data
            index = indexFromIterator (position);
            for (size_t offset = 0; offset < numItems; offset++)
            {
                // if we place items behind the actualSize, we need to use placement new
                // otherwise we have to use "="; memAssign manages these cases.
                memAssign (index + offset, * (first + offset));
            }

            m_actualSize = (index + numItems) > m_actualSize ? index + numItems : m_actualSize;
        }
        return Vector<T>::iterator (data() + index);
    }

    template <class T>
    template <typename InputIterator_pos, typename ContainerType,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::replace (InputIterator_pos position, const ContainerType &v)
    {
        return replace (position, v.begin(), v.end());
    }

    template <class T>
    template <typename ContainerType,
              typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::replace (size_t index, const ContainerType &v)
    {
        return replace (iteratorFromIndex (index), v);
    }

    template <class T>
    template <typename InputIterator_first, typename InputIterator_last,
              typename, typename> // evaluation in definition
    typename Vector<T>::iterator Vector<T>::replace (size_t index, InputIterator_first first, InputIterator_last last)
    {
        return replace (iteratorFromIndex (index), first, last);
    }

    template <class T>
    typename Vector<T>::iterator Vector<T>::erase (size_t index, size_t numElements)
    {
        return erase (iteratorFromIndex (index), iteratorFromIndex (index) + numElements);
    }

    template <class T>
    typename Vector<T>::iterator Vector<T>::erase (const_iterator position, size_t numElements)
    {
        return erase (position, position + numElements);
    }

    template <class T>
    typename Vector<T>::iterator Vector<T>::erase (const_iterator first, const_iterator last)
    {
        if (m_actualSize > 0)
        {
            assert (first < last && last <= end());

            size_t noDelItems = (last - first);
            size_t startIndex = indexFromIterator (first);
            for (size_t i = startIndex; i < m_actualSize; i++)
            {
                if (i + noDelItems >= m_actualSize)  // we are at the end
                {
                    (data() [i]).~T();
                }
                else
                {
                    // take the originally last item and place it at the new end
                    data() [i] = std::move (data() [i + noDelItems]);
                }
            }

            m_actualSize -= noDelItems;
            return iterator (data() + startIndex);
        }
        return begin();
    }

    template <class T>
    template <typename InputIterator_pos,
              typename> // evaluation in definition
    T Vector<T>::takeItem (InputIterator_pos position)
    {
        if (position >= end())
        {
            throw std::out_of_range ("position out of range");
        }

        T takenElement = *position;
        for (iterator posChanger (position); posChanger != end() - 1; posChanger++)
        {
            // take the originally last item and place it at the new end
            *posChanger = * (posChanger + 1);
        }
        pop_back();

        return takenElement;
    }

    template <class T>
    T Vector<T>::takeItem (size_t index)
    {
        return takeItem (iteratorFromIndex (index));
    }

    template <class T>
    template <typename InputIterator_pos,
              typename> // evaluation in definition
    Vector<T> Vector<T>::takeItems (InputIterator_pos position, size_t numElements)
    {
        if (position >= end())
        {
            throw std::out_of_range ("position out of range");
        }

        size_t numElementsCorrected = (position + numElements > end() ? end() - position : numElements);

        // Take Elements out
        Vector<T> takenElements;
        takenElements.reserve (numElementsCorrected);
        for (size_t index = 0; index < numElementsCorrected; index++)
        {
            takenElements.push_back (std::move (operator[] (position - begin() + index)));
        }

        // close gaps
        erase (position, position + numElements);

        return takenElements;
    }

    template<class T>
    Vector<T> Vector<T>::takeItems (size_t index, size_t numElements)
    {
        return takeItems (iteratorFromIndex (index), numElements);
    }

    template<class T>
    T &Vector<T>::operator[] (size_t index)
    {
        return data() [index];
    }

    template<class T>
    const T &Vector<T>::operator[] (size_t index) const
    {
        return data() [index];
    }

    template<class T>
    T &Vector<T>::at (size_t index)
    {
        if (index < m_actualSize)
        {
            return operator[] (index);
        }
        throw std::out_of_range ("index out of range");
    }

    template<class T>
    const T &Vector<T>::at (size_t index) const
    {
        if (index < m_actualSize)
        {
            return operator[] (index);
        }
        throw std::out_of_range ("index out of range");
    }

    template<class T>
    Vector<T> &Vector<T>::operator= (Vector<T> v)
    {
        if (this != &v)
        {
            classswap (*this, v);
        }
        return *this;
    }

    template<class T>
    Vector<T> &Vector<T>::operator= (const std::vector<T> &v)
    {
        // Create a non-const copy, so we can swap elements
        Vector<T> copy (v);
        classswap (*this, copy);
        return *this;
    }

    template<class T>
    bool Vector<T>::operator== (const std::vector<T> &v) const
    {
        if (size() != v.size())
        {
            return false;
        }

        for (size_t i = 0; i < v.size(); ++i)
        {
            if (at (i) != v.at (i))
            {
                return false;
            }
        }

        return true;
    }

    template<class T>
    bool Vector<T>::operator== (const Vector<T> &v) const
    {
        if (size() != v.size())
        {
            return false;
        }

        for (size_t i = 0; i < v.size(); ++i)
        {
            if (at (i) != v.at (i))
            {
                return false;
            }
        }

        return true;
    }

    template<class T>
    bool Vector<T>::operator!= (const std::vector<T> &v) const
    {
        return !operator== (v);
    }

    template<class T>
    bool Vector<T>::operator!= (const Vector<T> &v) const
    {
        return !operator== (v);
    }

    template <class T>
    inline void Vector<T>::freeAllocation()
    {
        if (m_data != nullptr)
        {
            m_data.reset();
            m_data = nullptr;
            m_allocationSize = 0;
        }
    }

    template <class T>
    void Vector<T>::clear()
    {
        while (m_actualSize)
        {
            pop_back();
        }
        m_allocationSize = 0;
    }

    template<class T>
    void Vector<T>::reserve (size_t capacity)
    {
        if (capacity > m_allocationSize)
        {
            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * capacity], std::default_delete<V_TYPE[]>());
            for (size_t i = 0; i < m_actualSize; ++i)
            {
                new (newBuffer.get() + sizeof (T) * i) T (std::move (operator[] (i)));
                (data() [i]).~T();
            }

            m_allocationSize = capacity;

            m_data.reset();
            m_data = newBuffer;
        }
    }

    template<class T>
    void Vector<T>::resize (size_t newSize)
    {
        if (newSize != m_actualSize)
        {
            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * newSize], std::default_delete<V_TYPE[]>());
            for (size_t i = 0; i < newSize; ++i)
            {
                if (i < m_actualSize)
                {
                    new (newBuffer.get() + sizeof (T) * i) T (std::move (operator[] (i)));
                }
                else
                {
                    new (newBuffer.get() + sizeof (T) * i) T();
                }
            }

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                (data() [i]).~T();
            }

            m_actualSize = newSize;
            m_allocationSize = newSize;

            m_data.reset();
            m_data = newBuffer;
        }
    }

    template<class T>
    void Vector<T>::resize (size_t newSize, T initVal)
    {
        if (newSize != m_actualSize)
        {
            std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * newSize], std::default_delete<V_TYPE[]>());
            for (size_t i = 0; i < newSize; ++i)
            {
                if (i < m_actualSize)
                {
                    new (newBuffer.get() + sizeof (T) * i) T (std::move (operator[] (i)));
                }
                else
                {
                    new (newBuffer.get() + sizeof (T) * i) T (initVal);
                }
            }

            for (size_t i = 0; i < m_actualSize; ++i)
            {
                (data() [i]).~T();
            }

            m_actualSize = newSize;
            m_allocationSize = newSize;

            m_data.reset();
            m_data = newBuffer;
        }
    }

    template<class T>
    void Vector<T>::shrink_to_fit()
    {
        if (m_data == nullptr || empty())
        {
            return;
        }

        std::shared_ptr<V_TYPE> newBuffer (new V_TYPE[sizeof (T) * m_actualSize], std::default_delete<V_TYPE[]>());
        for (size_t i = 0; i < m_actualSize; ++i)
        {
            new (newBuffer.get() + sizeof (T) * i) T (std::move (operator[] (i)));
            (data() [i]).~T();
        }

        m_allocationSize = m_actualSize;

        m_data.reset();
        m_data = newBuffer;
    }

    template<class T>
    void Vector<T>::swap (Vector<T> &&vector)
    {
        classswap (*this, vector);
    }

    template<class T>
    std::vector<T> Vector<T>::toStdVector()
    {
        return Vector<T>::toStdVector (*this);
    }

    template<class T>
    const std::vector<T> Vector<T>::toStdVector() const
    {
        return Vector<T>::toStdVector (*this);
    }

    template<class T>
    std::vector<T> Vector<T>::toStdVector (const Vector<T> &v)
    {
        return std::vector<T> (v.begin(), v.end());
    }

    template<class T>
    Vector<T> Vector<T>::fromStdVector (const std::vector<T> &v)
    {
        return Vector<T> (v);
    }

    template<class T>
    template<typename X, typename Y>
    Vector< Pair<X, Y> > Vector<T>::fromStdMap (const std::map<X, Y> &stdMap)
    {
        Vector< Pair<X, Y> > tempVector;
        tempVector.reserve (stdMap.size());
        for (auto &mapItem : stdMap)
        {
            tempVector.push_back (std::move (Pair<X, Y> (mapItem.first, mapItem.second)));
        }
        return tempVector;
    }

    template<class T>
    template<typename X, typename Y>
    std::map<X, Y> Vector<T>::toStdMap()
    {
        return Vector<T>::toStdMap (*this);
    }

    template<class T>
    template<typename X, typename Y>
    std::map<X, Y> Vector<T>::toStdMap (const Vector< Pair<X, Y> > &v)
    {
        std::map<X, Y> tempMap;
        for (size_t i = 0; i < v.size(); ++i)
        {
            tempMap.emplace (v.at (i).first, v.at (i).second);
        }
        return tempMap;
    }

    template<typename T>
    std::ostream &operator<< (std::ostream &os, const Vector<T> &v)
    {
        os << "{ ";
        for (size_t i = 0; i < v.size(); ++i)
        {
            os << v.at (i);
            if (i + 1 < v.size())
            {
                os << ",\n";
            }
        }
        os << " }";
        endl (os);
        return os;
    }

    /* ------------- Iterators -------------- */
    template<typename T>
    typename Vector<T>::iterator Vector<T>::begin()
    {
        return iterator (data());
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::begin() const
    {
        return const_iterator (data());
    }

    template<typename T>
    typename Vector<T>::iterator Vector<T>::end()
    {
        return iterator (data() + m_actualSize);
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::end() const
    {
        return const_iterator (data() + m_actualSize);
    }

    template<typename T>
    typename Vector<T>::reverse_iterator Vector<T>::rbegin()
    {
        return reverse_iterator (data() + m_actualSize - 1);
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::rbegin() const
    {
        return const_reverse_iterator (data() + m_actualSize - 1);
    }

    template<typename T>
    typename Vector<T>::reverse_iterator Vector<T>::rend()
    {
        return reverse_iterator (data() - 1);
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::rend() const
    {
        return const_reverse_iterator (data() - 1);
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::cbegin()
    {
        return const_iterator (begin());
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::cbegin() const
    {
        return const_iterator (begin());
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::cend()
    {
        return const_iterator (end());
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::cend() const
    {
        return const_iterator (end());
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::crbegin()
    {
        return const_reverse_iterator (rbegin());
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::crbegin() const
    {
        return const_reverse_iterator (rbegin());
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::crend()
    {
        return const_reverse_iterator (rend());
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::crend() const
    {
        return const_reverse_iterator (rend());
    }

    template<typename T>
    typename Vector<T>::iterator Vector<T>::iteratorFromIndex (size_t index)
    {
        return iterator (data() + index);
    }

    template<typename T>
    typename Vector<T>::const_iterator Vector<T>::iteratorFromIndex (size_t index) const
    {
        return const_iterator (data() + index);
    }

    template<typename T>
    typename Vector<T>::reverse_iterator Vector<T>::reverseIteratorFromIndex (size_t index)
    {
        return reverse_iterator (data() + index);
    }

    template<typename T>
    typename Vector<T>::const_reverse_iterator Vector<T>::reverseIteratorFromIndex (size_t index) const
    {
        return const_reverse_iterator (data() + index);
    }

    /* Declare static members */
    template<typename T>
    const size_t Vector<T>::npos;
}
