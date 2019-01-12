#pragma once

#include "Hash.hpp"



class FastSet {

    public:

    using V = size_t;
    using uchar = unsigned char;

    struct Entry {
        V val;
        uchar dist;
    };

    template <bool t_const> class Iterator {

        public:

        const FastSet * m_set;
        size_t m_i;

        Iterator(const FastSet & set, size_t i) :
            m_set(&set),
            m_i(i)
        {};

        template <bool t_const_>
        Iterator(const Iterator<t_const_> & iterator) :
            m_set(iterator.m_set),
            m_i(iterator.m_i)
        {};

        ~Iterator() = default;

        template <bool t_const_>
        Iterator<t_const> & operator=(const Iterator<t_const_> & iterator) {
            m_set = iterator.m_set;
            m_i = iterator.m_i;
            return *this;
        }
    };

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    
    struct PrivateTag {};

    FastSet(size_t minCapacity = 32) :
        FastSet(PrivateTag(), minCapacity < 2 ? 2 : qc::detail::ceil2(minCapacity))
    {}

    FastSet(FastSet && other) :
        m_size(other.m_size),
        m_capacity(other.m_capacity),
        m_arr(other.m_arr)
    {
        other.m_size = 0;
        other.m_capacity = 0;
        other.m_arr = nullptr;
    }

    FastSet(PrivateTag, size_t capacity) :
        m_size(0),
        m_capacity(capacity),
        m_arr(reinterpret_cast<Entry *>(std::calloc(m_capacity, sizeof(Entry)))) // TODO: a way around this calloc for copying
    {}

    ~FastSet() {
        if (m_capacity) {
            std::free(m_arr);
        }
    }

    FastSet & operator=(FastSet && other) {
        if (&other == this) {
            return *this;
        }

        if (m_capacity) {
            std::free(m_arr);
        }

        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_arr = other.m_arr;

        other.m_size = 0;
        other.m_capacity = 0;
        other.m_arr = nullptr;

        return *this;
    }
    
    std::pair<iterator, bool> emplace(V v) {
        return emplace_h(v, qc::Hash<V>()(v));
    }

    std::pair<iterator, bool> emplace_h(V value, size_t hash) {        
        size_t i(hash & (m_capacity - 1)); 
        uchar dist(1);

        while (true) {
            Entry & entry(m_arr[i]);

            // Can be inserted
            if (entry.dist < dist) {
                if (m_size >= (m_capacity >> 1)) {
                    expand();
                    return emplace_h(value, hash);
                }

                // Value here has smaller dist, robin hood
                if (entry.dist) {
                    propagate(entry.val, i + 1, entry.dist + 1);
                    entry.val = value;
                }
                // Open slot
                else {
                    entry.val = value;
                }

                entry.dist = dist;
                ++m_size;
                return { iterator(*this, i), true };
            }

            // Value already exists
            if (entry.val == value) {
                return { iterator(*this, i), false };
            }

            ++i;
            ++dist;

            if (i >= m_capacity) i = 0;
        }

        // Will never encounter this return
        return { cend(), false };
    }

    void propagate(V value, size_t i, uchar dist) {
        while (true) {
            if (i >= m_capacity) i = 0;
            Entry & entry(m_arr[i]);

            if (!entry.dist) {
                entry.val = value;
                entry.dist = dist;
                return;
            }

            if (entry.dist < dist) {
                std::swap(value, entry.val);
                std::swap(dist, entry.dist);
            }

            ++i;
            ++dist;
        }
    }

    void expandTo(size_t capacity) {
        FastSet temp(capacity);
        for (size_t ai(0), vi(0); vi < m_size; ++ai) {
            if (m_arr[ai].dist) {
                temp.emplace(m_arr[ai].val);
                ++vi;
            }
        }
        m_size = 0;

        *this = std::move(temp);
    }

    void expand() {
        expandTo(2 * m_capacity);
    }
    
    void clear() {
        for (size_t ai(0), vi(0); vi < m_size; ++ai) {
            if (m_arr[ai].dist) {
                m_arr[ai].dist = 0;
                ++vi;
            }
        }

        m_size = 0;
    }

    iterator end() {
        return iterator(*this, m_capacity);
    }

    const_iterator end() const {
        return cend();
    }

    const_iterator cend() const {
        return const_iterator(*this, m_capacity);
    }

    size_t m_size;
    size_t m_capacity;
    Entry * m_arr;

};
