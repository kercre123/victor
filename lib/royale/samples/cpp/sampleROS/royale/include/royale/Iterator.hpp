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

#include <iterator>
#include <cstddef>
#include <type_traits>

/* IMPORTANT */
/*
   Due to an MSVC compiler bug operator-> and operator* work for all types of
   iterators - which is different to the STL implementation.

   RAISED COMPILER BUG: https://connect.microsoft.com/VisualStudio/feedback/details/1882295
*/

namespace royale
{
    namespace iterator
    {
        template <bool, typename> struct enable_if { };
        template <typename T> struct enable_if<true, T>
        {
            typedef T type;
        };

        template <typename, typename> struct is_same
        {
            static const bool value = false;
        };
        template <typename T> struct is_same<T, T>
        {
            static const bool value = true;
        };

        template <typename T, typename T1, typename ... R>
        struct is_same_or_convertible
        {
            static const bool value =
                is_same<T, T1>::value ||
                std::is_convertible<T, T1>::value ||
                is_same_or_convertible<T, R...>::value;
        };

        template <typename T, typename T1>
        struct is_same_or_convertible<T, T1>
        {
            static const bool value =
                is_same<T, T1>::value ||
                std::is_convertible<T, T1>::value;
        };

        template<typename T, typename = void>
        struct is_iterator
        {
            static const bool value = false;
        };

        template<typename T>
        struct is_iterator < T, typename std::enable_if < !std::is_integral<T>::value
            &&std::is_base_of
            <std::input_iterator_tag
            , typename std::iterator_traits<T>::iterator_category
            >::value >::type >
        {
            static const bool value = true;
        };

        template< typename T >
        struct add_const : std::add_const<T> {};

        template< typename T >
        using add_const_t = typename add_const<T>::type;

        template< typename T >
        struct add_const<T &>
        {
            using type = add_const_t<T> &;
        };

        template< typename T >
        struct add_const<T *>
        {
            using type = add_const_t<T> *;
        };

        template< typename T >
        struct add_const<T const>
        {
            using type = add_const_t<T> const;
        };

        template< typename T >
        struct add_const<T volatile>
        {
            using type = add_const_t<T> volatile;
        };

        template< typename T >
        struct add_const<T[]>
        {
            using type = add_const_t<T>[];
        };

        template< typename T, size_t N >
        struct add_const<T[N]>
        {
            using type = add_const_t<T>[N];
        };

        /**
        * Iterator types (Category used during creation)
        * NOT used yet; implemented to distinguish royale iterators
        * from STL iterator
        *
        * The iterators in the container implementations follow the
        * STL and are created by using std::random_access_iterator_tag
        * making them compatible with other STL functions.
        */
        struct input_iterator_tag {};             // Input Iterator
        struct output_iterator_tag {};            // Output Iterator
        struct forward_iterator_tag {};           // Forward Iterator
        struct bidirection_iterator_tag {};       // Bidirectional Iterator
        struct random_access_iterator_tag {};     // Random - access Iterator

        /**
        * Iterator skeleton
        *
        * Holding the basic functions which are shared between all types
        * of royale iterators. Handling virtual functions for incrementing
        * and decrementing pointers.
        * This is the very basic class for handling iterators; based on the STL
        * template definitions which makes it STL compatible - all royale iterators are
        * based in this skeleton and every upcoming iterators will also be based on this
        * skeleton.
        */
        template <class Category,    // iterator::iterator_category
                  class T,                     // iterator::value_type
                  class Distance,              // iterator::difference_type
                  class Pointer,               // iterator::pointer
                  class Reference>             // iterator::reference
        class royale_iterator_skeleton
        {
        public:
            using value_type        = T;
            using difference_type   = Distance;
            using pointer           = Pointer;
            using reference         = Reference;
            using iterator_category = Category;

            royale_iterator_skeleton (typename add_const<pointer>::type ptr)
                : m_ptr (const_cast<pointer> (ptr))
            { }

            royale_iterator_skeleton (const royale_iterator_skeleton &) = default;

            royale_iterator_skeleton &operator= (const royale_iterator_skeleton &) = default;

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_iterator_skeleton &
                >::type operator+= (const difference_type rhs)
            {
                m_ptr = increment (rhs);
                return *this;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_iterator_skeleton &
                >::type operator-= (const difference_type rhs)
            {
                m_ptr = decrement (rhs);
                return *this;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                difference_type
                >::type operator+ (const royale_iterator_skeleton &rhs) const
            {
                return m_ptr + rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                difference_type
                >::type operator- (const royale_iterator_skeleton &rhs) const
            {
                return m_ptr - rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            (
                std::is_same<CurrIteratorCategory, std::input_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::forward_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::bidirectional_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value
            ),
            bool
            >::type operator== (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr == rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            (
                std::is_same<CurrIteratorCategory, std::input_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::forward_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::bidirectional_iterator_tag>::value ||
                std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value
            ),
            bool
            >::type operator!= (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr != rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                bool
                >::type operator< (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr < rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                bool
                >::type operator> (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr > rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                bool
                >::type operator<= (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr <= rhs.m_ptr;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                bool
                >::type operator>= (const royale_iterator_skeleton<CurrIteratorCategory, T, Distance, Pointer, Reference> &rhs) const
            {
                return m_ptr >= rhs.m_ptr;
            }

            /**
            * Returns the current item and increments the internal
            * pointer by n steps - default 1.
            */
            T nextItem (const Distance stepItems = 1)
            {
                Pointer tmp (m_ptr);
                m_ptr = increment (stepItems);
                return *tmp;
            }

            /**
            * Returns the current item and decrements the internal
            * pointer by n steps - default 1.
            */
            template < typename = typename std::enable_if < (
                           std::is_same<Category, std::bidirectional_iterator_tag>::value ||
                           std::is_same<Category, std::random_access_iterator_tag>::value)
                       >::type >
            T prevItem (const Distance stepItems = 1)
            {
                Pointer tmp (m_ptr);
                m_ptr = decrement (stepItems);
                return *tmp;
            }

            /**
            * Increment function - virtual!
            *
            * Used to increment the internal pointer by X steps; this function
            * might be overridden by other iterators (i.e. reverse iterator)
            * \param steps If nothing else was specified the returned pointer
            *        is incremented by "1"; however the function allows stepping
            *        more items within the same execution.
            * \return pointer Returns the pointer to the calculated position
            */
            virtual Pointer increment (const Distance steps = 1) const
            {
                return m_ptr + steps;
            }

            /**
            * Decrement function - virtual!
            *
            * Used to increment the internal pointer by X steps; this function
            * might be overridden by other iterators (i.e. reverse iterator)
            * \param steps If nothing else was specified the returned pointer
            *        is incremented by "1"; however the function allows stepping
            *        more items within the same execution.
            * \return pointer Returns the pointer to the calculated position
            */
            virtual Pointer decrement (const Distance steps = 1) const
            {
                return m_ptr - steps;
            }

            virtual Pointer internalPointer() const
            {
                return m_ptr;
            }
        protected:
            Pointer m_ptr;
        };

        /**
        * Royale base iterator class (basis for all non-const iterators)
        *
        * Holding the basic functions for a non-const iterator.
        * Other iterators can be based on this (if they follow the non-const
        * approach), while this class itself is directly based on the skeleton.
        *
        * The royale non-const forward iterator and the
        * royale non-const reverse iterator are based on this class.
        */
        template <class Category,    // iterator::iterator_category
                  class T,                     // iterator::value_type
                  class Distance,              // iterator::difference_type
                  class Pointer,               // iterator::pointer
                  class Reference>             // iterator::reference
        class royale_base_iterator : public royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>
        {
        public:
            royale_base_iterator (typename add_const<Pointer>::type ptr = nullptr) :
                royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> (ptr)
            { }

            royale_base_iterator (const royale_base_iterator<Category, T, Distance, Pointer, Reference> &base) :
                royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> (base)
            { }

            T *operator->()
            {
                return this->m_ptr;
            }

            T &operator*()
            {
                return * (this->m_ptr);
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                T &
                >::type operator[] (const Distance n)
            {
                Pointer tmp = this->increment (n);
                return *tmp;
            }

            royale_base_iterator<Category, T, Distance, Pointer, Reference> &operator= (const royale_base_iterator<Category, T, Distance, Pointer, Reference> &rhs)
            {
                this->m_ptr = rhs.m_ptr;
                return *this;
            }

            royale_base_iterator<Category, T, Distance, Pointer, Reference> &operator-- ()
            {
                this->m_ptr = this->decrement();
                return *this;
            }

            royale_base_iterator<Category, T, Distance, Pointer, Reference> operator-- (int b)
            {
                royale_base_iterator<Category, T, Distance, Pointer, Reference> i (*this);
                this->m_ptr = this->decrement();
                return i;
            }

            royale_base_iterator<Category, T, Distance, Pointer, Reference> &operator++ ()
            {
                this->m_ptr = this->increment();
                return *this;
            }

            royale_base_iterator<Category, T, Distance, Pointer, Reference> operator++ (int b)
            {
                royale_base_iterator<Category, T, Distance, Pointer, Reference> i (*this);
                this->m_ptr = this->increment();
                return i;
            }
        };

        /**
        * Royale base iterator class (basis for all const iterators)
        *
        * Holding the basic functions for a const iterator.
        * Other iterators can be based on this (if they follow the const
        * approach), while this class itself is directly based on the skeleton.
        *
        * The royale const forward iterator and the
        * royale const reverse iterator are based on this class.
        */
        template <class Category,    // iterator::iterator_category
                  class T,                     // iterator::value_type
                  class Distance,              // iterator::difference_type
                  class Pointer,               // iterator::pointer
                  class Reference>             // iterator::reference
        class royale_base_const_iterator : public royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>
        {
        public:
            royale_base_const_iterator (typename add_const<Pointer>::type ptr = nullptr) :
                royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> (ptr)
            { }

            royale_base_const_iterator (const royale_base_const_iterator<Category, T, Distance, Pointer, Reference> &base) :
                royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> (base)
            { }

            royale_base_const_iterator (const royale_base_iterator<Category, T, Distance, Pointer, Reference> &base) :
                royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> (base)
            { }

            const T *operator->() const
            {
                return this->m_ptr;
            }

            const T &operator*() const
            {
                return * (this->m_ptr);
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                const T &
                >::type operator[] (const Distance n)
            {
                Pointer tmp = this->increment (n);
                return *tmp;
            }

            royale_base_const_iterator<Category, T, Distance, Pointer, Reference> &operator= (const royale_iterator_skeleton<Category, T, Distance, Pointer, Reference> &rhs)
            {
                this->m_ptr = rhs.internalPointer();
                return *this;
            }

            royale_base_const_iterator<Category, T, Distance, Pointer, Reference> &operator-- ()
            {
                this->m_ptr = this->decrement();
                return *this;
            }

            royale_base_const_iterator<Category, T, Distance, Pointer, Reference> operator-- (int b)
            {
                royale_base_const_iterator<Category, T, Distance, Pointer, Reference> i (*this);
                this->m_ptr = this->decrement();
                return i;
            }

            royale_base_const_iterator<Category, T, Distance, Pointer, Reference> &operator++ ()
            {
                this->m_ptr = this->increment();
                return *this;
            }

            royale_base_const_iterator<Category, T, Distance, Pointer, Reference> operator++ (int b)
            {
                royale_base_const_iterator<Category, T, Distance, Pointer, Reference> i (*this);
                this->m_ptr = this->increment();
                return i;
            }
        };

        /**
        * Royale const forward iterator class (used by royale containers)
        *
        * Holding the specific functions for the const forward iterator.
        * Other specific iterators shall not be based on this final iterator
        * anymore.
        *
        * The constant forward iterator is used by the royale container classes
        * and is compatible with the rest of the STL implementation concerning iterators
        */
        template <class Category,    // iterator::iterator_category
                  class T,                     // iterator::value_type
                  class Distance = ptrdiff_t,  // iterator::difference_type
                  class Pointer = T *,         // iterator::pointer
                  class Reference = T &>       // iterator::reference
        class royale_const_iterator : public royale_base_const_iterator <Category, T, Distance, Pointer, Reference>
        {
        public:
            using royale_base_const_iterator<Category, T, Distance, Pointer, Reference>::operator=;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator+;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator-;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator>;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator>=;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator<;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator<=;

            royale_const_iterator (typename add_const<Pointer>::type ptr = nullptr) :
                royale_base_const_iterator<Category, T, Distance, Pointer, Reference> (ptr)
            { }

            royale_const_iterator (const royale_base_iterator<Category, T, Distance, Pointer, Reference> &base) :
                royale_base_const_iterator<Category, T, Distance, Pointer, Reference> (base)
            { }

            royale_const_iterator next (const Distance stepItems = 1)
            {
                royale_const_iterator tmp (*this);
                tmp.m_ptr = tmp.increment (stepItems);
                return tmp;
            }

            template < typename = typename std::enable_if < (
                           std::is_same<Category, std::bidirectional_iterator_tag>::value ||
                           std::is_same<Category, std::random_access_iterator_tag>::value)
                       >::type >
            royale_const_iterator prev (const Distance stepItems = 1)
            {
                royale_const_iterator tmp (*this);
                tmp.m_ptr = tmp.decrement (stepItems);
                return tmp;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_const_iterator<CurrIteratorCategory, T, Distance, Pointer, Reference>
                >::type operator + (const Distance value)
            {
                return royale_const_iterator<Category, T, Distance, Pointer, Reference> (this->increment (value));
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_const_iterator<CurrIteratorCategory, T, Distance, Pointer, Reference>
                >::type operator - (const Distance value)
            {
                return royale_const_iterator<Category, T, Distance, Pointer, Reference> (this->decrement (value));
            }
        };

        /**
        * Royale const reverse iterator class (used by royale containers)
        *
        * Holding the specific functions for the const reverse iterator.
        * Other specific iterators shall not be based on this final iterator
        * anymore.
        *
        * The constant reverse iterator is used by the royale container classes
        * and is compatible with the rest of the STL implementation concerning iterators
        */
        template <class Iter>
        class royale_const_reverse_iterator : public royale_base_const_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>
        {
        public:
            using royale_base_const_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator=;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator+;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator-;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator>;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator>=;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator<;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator<=;

            royale_const_reverse_iterator (typename Iter::pointer ptr = nullptr) :
                royale_base_const_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> (ptr)
            { }

            royale_const_reverse_iterator (const royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> &base) :
                royale_base_const_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> (base)
            { }

            royale_const_reverse_iterator next (const typename Iter::difference_type stepItems = 1)
            {
                royale_const_reverse_iterator tmp (*this);
                tmp.m_ptr = tmp.increment (stepItems);
                return tmp;
            }

            template < typename = typename std::enable_if < (
                           std::is_same<typename Iter::iterator_category, std::bidirectional_iterator_tag>::value ||
                           std::is_same<typename Iter::iterator_category, std::random_access_iterator_tag>::value)
                       >::type >
            royale_const_reverse_iterator prev (const typename Iter::difference_type stepItems = 1)
            {
                royale_const_reverse_iterator tmp (*this);
                tmp.m_ptr = tmp.decrement (stepItems);
                return tmp;
            }

            template<typename... Dummy, typename CurrIteratorCategory = typename Iter::iterator_category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_const_reverse_iterator<Iter>
                >::type operator + (const typename Iter::difference_type value)
            {
                return royale_const_reverse_iterator<Iter> (increment (value));
            }

            template<typename... Dummy, typename CurrIteratorCategory = typename Iter::iterator_category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_const_reverse_iterator<Iter>
                >::type operator - (const typename Iter::difference_type value)
            {
                return royale_const_reverse_iterator<Iter> (decrement (value));
            }

            virtual typename Iter::pointer increment (const typename Iter::difference_type steps = 1) const override
            {
                return this->m_ptr - steps;
            }

            virtual typename Iter::pointer decrement (const typename Iter::difference_type steps = 1) const override
            {
                return this->m_ptr + steps;
            }

            Iter base()
            {
                return Iter (this->m_ptr);
            }
        };

        /**
        * Royale non-const forward iterator class (used by royale containers)
        *
        * Holding the specific functions for the non-const forward iterator.
        * Other specific iterators shall not be based on this final iterator
        * anymore.
        *
        * The non-const forward iterator is used by the royale container classes
        * and is compatible with the rest of the STL implementation concerning iterators
        */
        template <class Category,    // iterator::iterator_category
                  class T,                     // iterator::value_type
                  class Distance = ptrdiff_t,  // iterator::difference_type
                  class Pointer = T *,         // iterator::pointer
                  class Reference = T &>       // iterator::reference
        class royale_iterator : public royale_base_iterator<Category, T, Distance, Pointer, Reference>
        {
        public:
            using royale_base_iterator<Category, T, Distance, Pointer, Reference>::operator=;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator+;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator-;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator>;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator>=;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator<;
            using royale_iterator_skeleton<Category, T, Distance, Pointer, Reference>::operator<=;

            royale_iterator (typename add_const<Pointer>::type ptr = nullptr) :
                royale_base_iterator<Category, T, Distance, Pointer, Reference> (ptr)
            { }

            royale_iterator (const royale_base_iterator<Category, T, Distance, Pointer, Reference> &base) :
                royale_base_iterator<Category, T, Distance, Pointer, Reference> (base)
            { }

            royale_iterator next (const Distance stepItems = 1)
            {
                royale_iterator tmp (*this);
                tmp.m_ptr = tmp.increment (stepItems);
                return tmp;
            }

            template < typename = typename std::enable_if < (
                           std::is_same<Category, std::bidirectional_iterator_tag>::value ||
                           std::is_same<Category, std::random_access_iterator_tag>::value)
                       >::type >
            royale_iterator prev (const Distance stepItems = 1)
            {
                royale_iterator tmp (*this);
                tmp.m_ptr = tmp.decrement (stepItems);
                return tmp;
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_iterator<CurrIteratorCategory, T, Distance, Pointer, Reference>
                >::type operator + (const Distance value)
            {
                return royale_iterator<Category, T, Distance, Pointer, Reference> (this->increment (value));
            }

            template<typename... Dummy, typename CurrIteratorCategory = Category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_iterator<CurrIteratorCategory, T, Distance, Pointer, Reference>
                >::type operator - (const Distance value)
            {
                return royale_iterator<Category, T, Distance, Pointer, Reference> (this->decrement (value));
            }
        };

        /**
        * Royale non-const reverse iterator class (used by royale containers)
        *
        * Holding the specific functions for the non-const reverse iterator.
        * Other specific iterators shall not be based on this final iterator
        * anymore.
        *
        * The non-const reverse iterator is used by the royale container classes
        * and is compatible with the rest of the STL implementation concerning iterators
        */
        template <class Iter>
        class royale_reverse_iterator : public royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>
        {
        public:
            using royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator=;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator+;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator-;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator>;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator>=;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator<;
            using royale_iterator_skeleton<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference>::operator<=;

            royale_reverse_iterator (typename Iter::pointer ptr = nullptr) :
                royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> (ptr)
            { }

            royale_reverse_iterator (const royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> &base) :
                royale_base_iterator<typename Iter::iterator_category, typename Iter::value_type, typename Iter::difference_type, typename Iter::pointer, typename Iter::reference> (base)
            { }

            royale_reverse_iterator next (const typename Iter::difference_type stepItems = 1)
            {
                royale_reverse_iterator tmp (*this);
                tmp.m_ptr = tmp.increment (stepItems);
                return tmp;
            }

            template < typename = typename std::enable_if < (
                           std::is_same<typename Iter::iterator_category, std::bidirectional_iterator_tag>::value ||
                           std::is_same<typename Iter::iterator_category, std::random_access_iterator_tag>::value)
                       >::type >
            royale_reverse_iterator prev (const typename Iter::difference_type stepItems = 1)
            {
                royale_reverse_iterator tmp (*this);
                tmp.m_ptr = tmp.decrement (stepItems);
                return tmp;
            }

            template<typename... Dummy, typename CurrIteratorCategory = typename Iter::iterator_category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_reverse_iterator<Iter>
                >::type operator + (const typename Iter::difference_type value)
            {
                return royale_reverse_iterator<Iter> (increment (value));
            }

            template<typename... Dummy, typename CurrIteratorCategory = typename Iter::iterator_category>
            inline typename std::enable_if <
            std::is_same<CurrIteratorCategory, std::random_access_iterator_tag>::value,
                royale_reverse_iterator<Iter>
                >::type operator - (const typename Iter::difference_type value)
            {
                return royale_reverse_iterator<Iter> (decrement (value));
            }

            virtual typename Iter::pointer increment (const typename Iter::difference_type steps = 1) const override
            {
                return this->m_ptr - steps;
            }

            virtual typename Iter::pointer decrement (const typename Iter::difference_type steps = 1) const override
            {
                return this->m_ptr + steps;
            }

            Iter base()
            {
                return Iter (this->m_ptr);
            }
        };
    }
}
