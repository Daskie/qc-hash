namespace old {



//==============================================================================
// Set /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::Set(unat minCapacity) :
    Set(minCapacity <= 4 ? 8 : detail::hash::ceil2(2 * minCapacity), PrivateTag{})
{}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(const Set<V, H, E> & other) :
    Set(other.m_capacity, PrivateTag{})
{    
    allocate();
    m_size = other.m_size;
    copyEntries(other.m_entries);
}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(Set<V, H, E> && other) :
    m_size(other.m_size),
    m_capacity(other.m_capacity),
    m_entries(other.m_entries)
{
    other.m_size = 0;
    other.m_capacity = 0;
    other.m_entries = nullptr;
}

template <typename V, typename H, typename E>
template <typename It>
Set<V, H, E>::Set(It first, It last) :
    Set(std::distance(first, last))
{
    insert(first, last);
}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(std::initializer_list<V> values) :
    Set(values.size())
{
    insert(values);
}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(unat capacity, PrivateTag) :
    m_size(0),
    m_capacity(capacity),
    m_entries(nullptr)
{}

//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::~Set() {
    if (m_entries) { // TODO: might not need this check
        clear();
        std::free(m_entries);
    }
}

//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(const Set<V, H, E> & other) {
    if (&other == this) {
        return *this;
    }

    clear();

    if (m_capacity != other.m_capacity) {
        std::free(m_entries);
        m_capacity = other.m_capacity;
        allocate();
    }
    m_size = other.m_size;

    copyEntries(other.m_entries);

    return *this;
}

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(Set<V, H, E> && other) {
    if (&other == this) {
        return *this;
    }

    if (m_entries) {
        clear();
        std::free(m_entries);
    }

    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_entries = other.m_entries;

    other.m_size = 0;
    other.m_capacity = 0;
    other.m_entries = nullptr;

    return *this;
}

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(std::initializer_list<V> values) {
    clear();
    insert(values);

    return *this;
}

//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::begin() {
    return cbegin();
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::begin() const {
    return cbegin();
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::cbegin() const {
    if (!m_size) {
        return cend();
    }

    for (unat i(0); ; ++i) {
        if (m_entries[i].dist) {
            return const_iterator(m_entries + i);
        }
    }
}

//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::end() {
    return cend();
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::end() const {
    return cend();
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::cend() const {
    return const_iterator(m_entries + m_capacity);
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket_count() const {
    return m_capacity;
}

//==============================================================================
// max_bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::max_bucket_count() const {
    return 2 * max_size();
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket_size(unat i) const {
    if (i >= m_capacity || !m_entries) {
        return 0;
    }

    Dist dist(1);
    while (m_entries[i].dist > dist) {
        ++i;
        ++dist;

        if (i >= m_capacity) i = 0;
    }
    
    unat n(0);
    while (m_entries[i].dist == dist) {
        ++i;
        ++dist;
        ++n;

        if (i >= m_capacity) i = 0;
    }

    return n;
}

//==============================================================================
// bucket
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket(const V & value) const {
    return detIndex(H()(value));
}

//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
bool Set<V, H, E>::empty() const {
    return m_size == 0;
}

//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::size() const {
    return m_size;
}

//==============================================================================
// max_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::max_size() const {
    return std::numeric_limits<unat>::max() / 2;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::clear() {
    if constexpr (std::is_trivially_destructible_v<V>) {
        if (m_size) {
            std::memset(m_entries, 0, m_capacity * sizeof(Entry)); // TODO: test if faster to clear dists individually when V is large
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) { // TODO: check if faster to just go over whole thing (and check other occurances)
            if (m_entries[i].dist) {
                m_entries[i].val.~V();
                m_entries[i].dist = 0;
                ++n;
            }
        }
    }

    m_size = 0;
}

//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::insert(const V & value) {
    return emplace(value);
}

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::insert(V && value) {
    return emplace(std::move(value));
}

template <typename V, typename H, typename E>
template <typename InputIt>
void Set<V, H, E>::insert(InputIt first, InputIt last) {
    while (first != last) {
        emplace(*first);
        ++first;
    }
}

template <typename V, typename H, typename E>
void Set<V, H, E>::insert(std::initializer_list<V> values) {
    for (const V & value : values) {
        emplace(value);
    }
}

//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <typename... Args>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace(Args &&... args) {
    V value(std::forward<Args>(args)...);
    return emplace_h(std::move(value), H()(value));
}

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace(V && value) {
    return emplace_h(std::move(value), H()(value));
}

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace_h(V && value, unat hash) {
    if (!m_entries) allocate();
    unat i(detIndex(hash));
    Dist dist(1);

    while (true) {
        Entry & entry(m_entries[i]);

        // Can be inserted
        if (entry.dist < dist) {
            if (m_size >= (m_capacity >> 1)) {
                rehash(m_capacity << 1, PrivateTag{});
                return emplace_h(std::move(value), hash);
            }

            // Value here has smaller dist, robin hood
            if (entry.dist) {
                V temp(std::move(entry.val));
                entry.val = std::move(value);
                propagate(temp, i + 1, entry.dist + 1);
            }
            // Open slot
            else {
                new (&entry.val) V(std::move(value));
            }

            entry.dist = dist;
            ++m_size;
            return { iterator(&entry), true };
        }

        // Value already exists
        if (E()(entry.val, value)) {
            return { iterator(&entry), false };
        }

        ++i;
        ++dist;

        if (i >= m_capacity) i = 0;
    }

    // Will never reach reach this return
    return { end(), false };
}

template <typename V, typename H, typename E>
void Set<V, H, E>::propagate(V & value, unat i, Dist dist) {
    while (true) {
        if (i >= m_capacity) i = 0;
        Entry & entry(m_entries[i]);

        if (!entry.dist) {
            new (&entry.val) V(std::move(value));
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

//==============================================================================
// emplace_hint
//------------------------------------------------------------------------------

template<typename V, typename H, typename E>
template <typename... Args>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace_hint(const_iterator hint, Args &&... args) {
    return emplace(std::forward<Args>(args)...);
}

//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::erase(const V & value) {
    iterator it(find(value));
    if (it == end()) return 0;
    erase(it);
    return 1;
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::erase(const_iterator position) {
    const_iterator endIt(cend());
    if (position == endIt || !m_entries) {
        return endIt;
    }

    unat i(position.m_entry - m_entries), j(i + 1);

    while (true) {
        if (j >= m_capacity) j = 0;

        if (m_entries[j].dist <= 1) {
            break;
        }

        m_entries[i].val = std::move(m_entries[j].val);
        m_entries[i].dist = m_entries[j].dist - 1;

        ++i; ++j;
        if (i >= m_capacity) i = 0;
    }

    m_entries[i].val.~V();
    m_entries[i].dist = 0;
    --m_size;

    // TODO: find a way to improve seeking when extremely empty
    while (!position.m_entry->dist) {
        ++position;
        if (position == endIt) {
            break;
        }
    }

    return position;
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::swap(Set<V, H, E> & other) {
    std::swap(m_size, other.m_size);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_entries, other.m_entries);
}

//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::count(const V & value) const {
    return contains(value);
}

//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::find(const V & value) {
    return cfind(value);
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::find(const V & value) const {
    return cfind(value);
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::const_iterator Set<V, H, E>::cfind(const V & value) const {
    if (!m_entries) {
        return cend();
    }
    
    unat i(detIndex(H()(value)));
    Dist dist(1);

    while (true) {
        const Entry & entry(m_entries[i]);

        if (E()(entry.val, value)) {
            return const_iterator(&entry);
        }

        if (entry.dist < dist) {
            return cend();
        }

        ++i;
        ++dist;

        if (i >= m_capacity) i = 0;
    };

    // Will never reach reach this return
    return cend();
}

//==============================================================================
// contains
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
bool Set<V, H, E>::contains(const V & value) const {
    return find(value) != cend();
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
float Set<V, H, E>::load_factor() const {
    return float(m_size) / float(m_capacity);
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
float Set<V, H, E>::max_load_factor() const {
    return 0.5f; // TODO: play with larger load factors
}

//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::rehash(unat minCapacity) {
    if (minCapacity <= 8) minCapacity = 8;
    else minCapacity = detail::hash::ceil2(minCapacity);

    if (minCapacity >= 2 * m_size && minCapacity != m_capacity) {
        if (m_entries) {
            rehash(minCapacity, PrivateTag{});
        }
        else {
            m_capacity = minCapacity;
        }
    }
}

//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::reserve(unat n) {
    n *= 2;
    if (n > m_capacity) rehash(n);
}

//==============================================================================
// hash_function
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
typename Set<V, H, E>::hasher Set<V, H, E>::hash_function() const {
    return hasher();
}

//==============================================================================
// key_eq
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
typename Set<V, H, E>::key_equal Set<V, H, E>::key_eq() const {
    return key_equal();
}

// Non-member Functions ////////////////////////////////////////////////////////

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
inline bool operator==(const Set<V, H, E> & s1, const Set<V, H, E> & s2) {
    if (s1.m_size != s2.m_size) {
        return false;
    }

    if (&s1 == &s2) {
        return true;
    }

    for (const auto & v : s1) {
        if (!s2.contains(v)) {
            return false;
        }
    }

    return true;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
inline bool operator!=(const Set<V, H, E> & s1, const Set<V, H, E> & s2) {
    return !(s1 == s2);
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
inline void swap(Set<V, H, E> & s1, Set<V, H, E> & s2) {
    s1.swap(s2);
}

// Private Methods /////////////////////////////////////////////////////////////

template <typename V, typename H, typename E>
void Set<V, H, E>::rehash(unat capacity, PrivateTag) {
    unat oldSize(m_size);
    Entry * oldEntries(m_entries);

    m_size = 0;
    m_capacity = capacity;
    allocate();

    for (unat i(0), n(0); n < oldSize; ++i) {
        Entry & entry(oldEntries[i]);
        if (entry.dist) {
            emplace(std::move(entry.val));
            entry.val.~V();
            ++n;
        }
    }

    std::free(oldEntries);
}

template <typename V, typename H, typename E>
void Set<V, H, E>::allocate() {
    m_entries = reinterpret_cast<Entry *>(std::calloc(m_capacity + 1, sizeof(Entry)));
    m_entries[m_capacity].dist = std::numeric_limits<Dist>::max();
}

// TODO: maybe inline this?
template <typename V, typename H, typename E>
void Set<V, H, E>::copyEntries(const Entry * entries) {
    if constexpr (std::is_trivially_copyable_v<V>) {
        if (m_size) {
            std::memcpy(m_entries, entries, m_capacity * sizeof(Entry));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_entries[i].dist = entries[i].dist) {
                new (&m_entries[i].val) V(entries[i].val);
                ++n;
            }
        }
    }
}



//==============================================================================
// Iterator ////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
Set<V, H, E>::Iterator<t_const>::Iterator(const Entry * entry) :
    m_entry(entry)
{}

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
Set<V, H, E>::Iterator<t_const>::Iterator(const Set<V, H, E>::Iterator<t_const_> & other) :
    m_entry(other.m_entry)
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator=(const Set<V, H, E>::Iterator<t_const_> & other) {
    m_entry = other.m_entry;

    return *this;
}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator++() {
    do {
        ++m_entry;
    } while (!m_entry->dist);

    return *this;
}

//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
typename Set<V, H, E>::Iterator<t_const> Set<V, H, E>::Iterator<t_const>::operator++(int) {
    Set<V, H, E>::Iterator<t_const> temp(*this);
    operator++();
    return temp;
}

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E>::Iterator<t_const>::operator==(const Set<V, H, E>::Iterator<t_const_> & o) const {
    return m_entry == o.m_entry;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E>::Iterator<t_const>::operator!=(const Set<V, H, E>::Iterator<t_const_> & o) const {
    return m_entry != o.m_entry; // TODO: test comparing const with non const
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V & Set<V, H, E>::Iterator<t_const>::operator*() const {
    return m_entry->val;
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V * Set<V, H, E>::Iterator<t_const>::operator->() const {
    return &m_entry->val;
}



}