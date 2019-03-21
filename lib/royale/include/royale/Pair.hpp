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
#include <cstddef>
#include <utility>
#include <ostream>
#include <type_traits>

namespace royale
{
    template<typename first_type, typename second_type>
    class Pair
    {
    public:
        /**
        * General constructor, which does not allocate memory and sets everything to it's default
        */
        Pair<first_type, second_type>();

        /**
        * Copy constructor which allows creation of a royale compliant pair from
        * another royale compliant pair - (NOTE: performs a deep copy!)
        *
        * \param pair The royale pair which's memory shall be copied
        */
        Pair<first_type, second_type> (const Pair<first_type, second_type> &pair);

        /**
        * Move constructor which allows moving a royale compliant pair from
        * another royale compliant pair - (NOTE: performs a shallow copy!)
        *
        * \param pair The royale pair which is moved
        */
        Pair<first_type, second_type> (Pair<first_type, second_type> &&pair);

        /**
        * Copy constructor which allows creation of a royale compliant pair from
        * an STL compliant pair - (NOTE: performs a deep copy!)
        *
        * \param pair The STL pair which's memory shall be copied
        */
        Pair<first_type, second_type> (const std::pair<first_type, second_type> &pair);

        /**
        * Constructor for creation of the royale compliant pair by providing
        * the two pair values directly
        *
        * \param first The first value for the pair
        * \param second The second value for the pair
        */
        Pair<first_type, second_type> (const first_type &first, const second_type &second);
        Pair<first_type, second_type> (const first_type &first_val, second_type &&second_val);
        Pair<first_type, second_type> (first_type &&first_val, const second_type &second_val);
        Pair<first_type, second_type> (first_type &&first_val, second_type &&second_val);

        /**
        * Destructor
        * Does nothing special; there is no internal memory to be cleared
        */
        virtual ~Pair<first_type, second_type>();

        /**
        * Assigns new contents to the container, replacing its current contents.
        * This method copies all elements held by pair into the container
        *
        * \param pair A royale compliant pair of the same storage type
        * \return Pair<first_type, second_type>& Returns *this
        */
        Pair<first_type, second_type> &operator= (Pair<first_type, second_type> &&pair);

        /**
        * Assigns new contents to the container, replacing its current contents.
        * This method copies all elements held by pair into the container
        *
        * \param pair A royale compliant pair of the same storage type
        * \return Pair<first_type, second_type>& Returns *this
        */
        Pair<first_type, second_type> &operator= (const Pair<first_type, second_type> &pair);

        /**
        * Assign the contents of an STL compliant pair container by
        * replacing container's current contents
        * This method copies the elements held by pair into the royale
        * compliant pair container
        *
        * \param pair An STL compliant pair of the same storage type
        * \return Pair<first_type, second_type>& Returns *this
        */
        Pair<first_type, second_type> &operator= (const std::pair<first_type, second_type> &pair);

        /**
        * Checks equality with an STL compliant pair
        *
        * \param pair An STL compliant pair of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const std::pair<first_type, second_type> &pair) const;

        /**
        * Checks equality with a royale compliant pair
        *
        * \param pair A royale compliant pair of the same storage type
        * \return bool Returns true if they are equal and false is they are not
        */
        bool operator== (const Pair<first_type, second_type> &pair) const;

        /**
        * Checks unequality with an STL compliant pair
        *
        * \param pair An STL compliant pair of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const std::pair<first_type, second_type> &pair) const;

        /**
        * Checks unequality with a royale compliant pair
        *
        * \param pair A royale compliant pair of the same storage type
        * \return bool Returns false if they are equal and true is they are not
        */
        bool operator!= (const Pair<first_type, second_type> &pair) const;

        /**
        * User convenience function to allow conversion to std::pair
        * which might be used outside the library by the application for further processing
        *
        * \return std::pair containing the items of the royale compliant pair
        */
        std::pair<first_type, second_type> toStdPair();

        /**
        * User convenience function to allow conversion from std::map
        * which might be used inside the library for further processing
        *
        * \param pair The STL compliant pair which shall be converted to the API style pair
        * \return Pair<first_type, second_type> Returned is a royale compliant pair
        */
        static Pair<first_type, second_type> fromStdPair (const std::pair<first_type, second_type> &pair);

        first_type first;
        second_type second;

    private:
        template<typename first_type_, typename second_type_>
        friend std::ostream &operator<< (std::ostream &os, const Pair<first_type_, second_type_> &pair);
    };

    //! Template implementation
    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair()
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (const Pair<first_type, second_type> &pair) :
        first (pair.first),
        second (pair.second)
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (Pair<first_type, second_type> &&pair) :
        first (std::move (pair.first)),
        second (std::move (pair.second))
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (const std::pair<first_type, second_type> &pair) :
        first (pair.first),
        second (pair.second)
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (const first_type &first_val, const second_type &second_val) :
        first (first_val),
        second (second_val)
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (first_type &&first_val, second_type &&second_val) :
        first (std::move (first_val)),
        second (std::move (second_val))
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (first_type &&first_val, const second_type &second_val) :
        first (std::move (first_val)),
        second (second_val)
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::Pair (const first_type &first_val, second_type &&second_val) :
        first (first_val),
        second (std::move (second_val))
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type>::~Pair()
    { }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type> &Pair<first_type, second_type>::operator= (const Pair<first_type, second_type> &pair)
    {
        if (this != &pair)
        {
            first = pair.first;
            second = pair.second;
        }
        return *this;
    }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type> &Pair<first_type, second_type>::operator= (Pair<first_type, second_type> &&pair)
    {
        if (this != &pair)
        {
            first = std::move (pair.first);
            second = std::move (pair.second);
        }
        return *this;
    }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type> &Pair<first_type, second_type>::operator= (const std::pair<first_type, second_type> &pair)
    {
        first = pair.first;
        second = pair.second;
        return *this;
    }

    template<typename first_type, typename second_type>
    bool Pair<first_type, second_type>::operator== (const Pair<first_type, second_type> &pair) const
    {

        if (first == pair.first && second == pair.second)
        {
            return true;
        }
        return false;
    }

    template<typename first_type, typename second_type>
    bool Pair<first_type, second_type>::operator== (const std::pair<first_type, second_type> &pair) const
    {
        if (first == pair.first && second == pair.second)
        {
            return true;
        }
        return false;
    }

    template<typename first_type, typename second_type>
    bool Pair<first_type, second_type>::operator!= (const Pair<first_type, second_type> &pair) const
    {
        return !operator== (pair);
    }

    template<typename first_type, typename second_type>
    bool Pair<first_type, second_type>::operator!= (const std::pair<first_type, second_type> &pair) const
    {
        return !operator== (pair);
    }

    template<typename first_type, typename second_type>
    Pair<first_type, second_type> Pair<first_type, second_type>::fromStdPair (const std::pair<first_type, second_type> &pair)
    {
        return Pair (pair);
    }

    template<typename first_type, typename second_type>
    std::pair<first_type, second_type> Pair<first_type, second_type>::toStdPair()
    {
        return std::make_pair (first, second);
    }

    template<typename first_type, typename second_type>
    std::ostream &operator<< (std::ostream &os, const Pair<first_type, second_type> &pair)
    {
        os << "( ";
        os << pair.first;
        os << ", ";
        os << pair.second;
        os << " )";
        return os;
    }

    /**
    * User convenience function to construct a pair
    * calls the alternative constructor directly
    *
    * \return Pair<first_type, second_type> The royale compliant constructed pair
    */
    template <typename T1, typename T2>
    Pair<T1, T2> royale_pair (T1 &&x, T2 &&y)
    {
        return Pair<T1, T2> (std::forward<T1> (x), std::forward<T2> (y));
    }
}
