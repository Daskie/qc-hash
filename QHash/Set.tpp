namespace qc {



//==============================================================================
// Set /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::Set(size_t minCapacity) :
    Set(PrivateTag(), minCapacity < 2 ? 2 : detail::ceil2(minCapacity))
{}

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::Set(const Set<V, H, Eq> & other) :
    Set(PrivateTag(), other.m_capacity)
{
    allocate();
    m_size = other.m_size;
    for (size_t ai(0), vi(0); vi < m_size; ++ai) {
        if (other.m_dists[ai] > 0) {
            new (m_vals + ai) V(other.m_vals[ai]);
            m_dists[ai] = other.m_dists[ai];
            ++vi;
        }
    }
}

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::Set(Set<V, H, Eq> && other) :
    m_size(other.m_size),
    m_capacity(other.m_capacity),
    m_vals(other.m_vals),
    m_dists(other.m_dists)
{
    other.m_size = 0;
    other.m_capacity = 0;
    other.m_vals = nullptr;
    other.m_dists = nullptr;
}

template <typename V, typename H, typename Eq>
template <typename It>
Set<V, H, Eq>::Set(It first, It last) :
    Set(2 * std::distance(first, last))
{
    insert(first, last);
}

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::Set(std::initializer_list<V> values) :
    Set(2 * values.size())
{
    insert(values);
}

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::Set(PrivateTag, size_t capacity) :
    m_size(0),
    m_capacity(capacity),
    m_vals(nullptr),
    m_dists(nullptr)
{}

//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
Set<V, H, Eq>::~Set() {
    if (m_vals) {
        clear<false>();
        std::free(m_vals);
        std::free(m_dists);
    }
}

//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
Set<V, H, Eq> & Set<V, H, Eq>::operator=(const Set<V, H, Eq> & other) {
    if (&other == this) {
        return *this;
    }

    clear();
    rehash(other.m_capacity);
    insert(other.cbegin(), other.cend());

    return *this;
}

template <typename V, typename H, typename Eq>
Set<V, H, Eq> & Set<V, H, Eq>::operator=(Set<V, H, Eq> && other) {
    if (&other == this) {
        return *this;
    }

    if (m_vals) {
        clear<false>();
        std::free(m_vals);
        std::free(m_dists);
    }

    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_vals = other.m_vals;
    m_dists = other.m_dists;

    other.m_size = 0;
    other.m_capacity = 0;
    other.m_vals = nullptr;
    other.m_dists = nullptr;

    return *this;
}

template <typename V, typename H, typename Eq>
Set<V, H, Eq> & Set<V, H, Eq>::operator=(std::initializer_list<V> values) {
    return *this = std::move(Set<V, H, Eq>(values));
}

//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::iterator Set<V, H, Eq>::begin() {
    return cbegin();
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::begin() const {
    return cbegin();
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::cbegin() const {
    if (!m_size) {
        return cend();
    }

    size_t i(0);
    while (!m_dists[i]) ++i;
    return const_iterator(*this, i);
}

//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::iterator Set<V, H, Eq>::end() {
    return iterator(*this, m_capacity);
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::end() const {
    return cend();
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::cend() const {
    return const_iterator(*this, m_capacity);
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::bucket_count() const {
    return m_capacity;
}

//==============================================================================
// max_bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::max_bucket_count() const {
    return 2 * max_size();
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::bucket_size(size_t i) const {
    if (i >= m_capacity || !m_vals) {
        return 0;
    }

    size_t dist(1);
    while (m_dists[i] > dist) {
        ++i;
        ++dist;
        
        if (i >= m_capacity) i = 0;
    }
    
    size_t n(0);
    while (m_dists[i] == dist) {
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

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::bucket(const V & value) const {
    return H()(value) & (m_capacity - 1);
}

//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
bool Set<V, H, Eq>::empty() const {
    return m_size == 0;
}

//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::size() const {
    return m_size;
}

//==============================================================================
// max_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::max_size() const {
    return size_t(-1) >> 1;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::clear() {    
    clear<true>();
}

//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::insert(const V & value) {
    return emplace(value);
}

template <typename V, typename H, typename Eq>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::insert(V && value) {
    return emplace(std::move(value));
}

template <typename V, typename H, typename Eq>
template <typename InputIt>
void Set<V, H, Eq>::insert(InputIt first, InputIt last) {
    while (first != last) {
        emplace(*first);
        ++first;
    }
}

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::insert(std::initializer_list<V> values) {
    for (const V & value : values) {
        emplace(value);
    }
}

//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <typename... Args>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::emplace(Args &&... args) {
    V value(std::forward<Args>(args)...);
    return emplace_h(std::move(value), H()(value));
}

template <typename V, typename H, typename Eq>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::emplace(V && value) {
    return emplace_h(std::move(value), H()(value));
}

template <typename V, typename H, typename Eq>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::emplace_h(V && value, size_t hash) {
    if (!m_vals) allocate();
    
    size_t i(hash & (m_capacity - 1)); 
    unsigned char dist(1);

    while (true) {
        // Can be inserted
        if (m_dists[i] < dist) {
            if (m_size >= (m_capacity >> 1)) {
                expand();
                return emplace_h(std::move(value), hash);
            }

            // Value here has smaller dist, robin hood
            if (m_dists[i]) {
                V temp(std::move(m_vals[i]));
                m_vals[i] = std::move(value);
                propagate(temp, i + 1, m_dists[i] + 1);
            }
            // Open slot
            else {
                new (m_vals + i) V(std::move(value));
            }

            m_dists[i] = dist;
            ++m_size;
            return { iterator(*this, i), true };
        }

        // Value already exists
        if (Eq()(m_vals[i], value)) {
            return { iterator(*this, i), false };
        }

        ++i;
        ++dist;

        if (i >= m_capacity) i = 0;
    }

    // Will never encounter this return
    return { cend(), false };
}

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::propagate(V & value, size_t i, unsigned char dist) {
    while (true) {
        if (i >= m_capacity) i = 0;

        if (!m_dists[i]) {
            new (m_vals + i) V(std::move(value));
            m_dists[i] = dist;
            return;
        }

        if (m_dists[i] < dist) {
            std::swap(value, m_vals[i]);
            std::swap(dist, m_dists[i]);
        }

        ++i;
        ++dist;
    }
}

//==============================================================================
// emplace_hint
//------------------------------------------------------------------------------

template<typename V, typename H, typename Eq>
template <typename... Args>
std::pair<typename Set<V, H, Eq>::iterator, bool> Set<V, H, Eq>::emplace_hint(const_iterator hint, Args &&... args) {
    return emplace(std::forward<Args>(args)...);
}

//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::erase(const V & value) {
    iterator it(find(value));
    if (it.m_i >= m_capacity) return 0;
    erase(it);
    return 1;
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::iterator Set<V, H, Eq>::erase(const_iterator position) {
    if (position.m_i >= m_capacity || !m_vals) {
        return cend();
    }
    
    size_t i(position.m_i), j(i + 1);
    while (true) {
        if (j >= m_capacity) j = 0;
        if (m_dists[j] <= 1) {
            break;
        }

        m_vals[i] = std::move(m_vals[j]);
        m_dists[i] = m_dists[j] - 1;
        ++i; ++j;

        if (i >= m_capacity) i = 0;
    }

    m_vals[i].~V();
    m_dists[i] = 0;
    --m_size;

    while (!m_dists[position.m_i]) {
        ++position.m_i;
        if (position.m_i >= m_capacity) {
            break;
        }
    }

    return position;
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::swap(Set<V, H, Eq> & other) {
    std::swap(m_size, other.m_size);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_vals, other.m_vals);
    std::swap(m_dists, other.m_dists);
}

//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
size_t Set<V, H, Eq>::count(const V & value) const {
    return contains(value);
}

//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::iterator Set<V, H, Eq>::find(const V & value) {
    return cfind(value);
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::find(const V & value) const {
    return cfind(value);
}

template <typename V, typename H, typename Eq>
typename Set<V, H, Eq>::const_iterator Set<V, H, Eq>::cfind(const V & value) const {
    if (!m_vals) {
        return cend();
    }
    
    size_t i(H()(value) & (m_capacity - 1));
    unsigned char dist(1);

    while (true) {
        if (Eq()(m_vals[i], value)) {
            return { *this, i };
        }

        if (m_dists[i] < dist) {
            return cend();
        }

        ++i;
        ++dist;

        if (i >= m_capacity) i = 0;
    };

    // Will never reach this return
    return cend();
}

//==============================================================================
// contains
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
bool Set<V, H, Eq>::contains(const V & value) const {
    return cfind(value).m_i != m_capacity;
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
float Set<V, H, Eq>::load_factor() const {
    return float(m_size) / float(m_capacity);
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
float Set<V, H, Eq>::max_load_factor() const {
    return 0.5f;
}

//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::rehash(size_t minCapacity) {
    minCapacity = detail::ceil2(minCapacity);
    if (minCapacity >= 2 * m_size && minCapacity != m_capacity) {
        expandTo(minCapacity);
    }
}

//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::reserve(size_t n) {
    if (2 * n > m_capacity) rehash(2 * n);
}

//==============================================================================
// hash_function
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
H Set<V, H, Eq>::hash_function() const {
    return H();
}

//==============================================================================
// key_eq
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
Eq Set<V, H, Eq>::key_eq() const {
    return Eq();
}

// Non-member Functions ////////////////////////////////////////////////////////

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
inline bool operator==(const Set<V, H, Eq> & s1, const Set<V, H, Eq> & s2) {
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

template <typename V, typename H, typename Eq>
inline bool operator!=(const Set<V, H, Eq> & s1, const Set<V, H, Eq> & s2) {
    return !(s1 == s2);
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
inline void swap(Set<V, H, Eq> & s1, Set<V, H, Eq> & s2) {
    s1.swap(s2);
}

// Private Methods /////////////////////////////////////////////////////////////

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::expandTo(size_t capacity) {
    if (!m_vals) {
        m_capacity = capacity;
        return;
    }

    Set<V, H, Eq> temp(PrivateTag(), capacity);
    for (size_t ai(0), vi(0); vi < m_size; ++ai) {
        if (m_dists[ai]) {
            temp.emplace(std::move(m_vals[ai]));
            m_vals[ai].~V();
            m_dists[ai] = 0;
            ++vi;
        }
    }

    m_size = 0;

    *this = std::move(temp);
}

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::expand() {
    expandTo(2 * m_capacity);
}

template <typename V, typename H, typename Eq>
template <bool t_clearDists>
void Set<V, H, Eq>::clear() {
    if constexpr (std::is_trivial_v<V>) {
        if constexpr (t_clearDists) {
            if (m_size) {
                std::memset(m_dists, 0, m_capacity);
            }
        }
    }
    else {
        for (size_t ai(0), vi(0); vi < m_size; ++ai) {
            if (m_dists[ai]) {
                m_vals[ai].~V();
                if constexpr (t_clearDists) {
                    m_dists[ai] = 0;
                }
                ++vi;
            }
        }
    }

    m_size = 0;
}

template <typename V, typename H, typename Eq>
void Set<V, H, Eq>::allocate() {
    m_vals = reinterpret_cast<V *>(std::malloc(m_capacity * sizeof(V)));
    m_dists = reinterpret_cast<unsigned char *>(std::calloc(m_capacity, 1));
}



//==============================================================================
// Iterator ////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
Set<V, H, Eq>::Iterator<t_const>::Iterator(const Set<V, H, Eq> & set, size_t i) :
    m_set(&set),
    m_i(i)
{}

template <typename V, typename H, typename Eq>
template <bool t_const>
template <bool t_const_>
Set<V, H, Eq>::Iterator<t_const>::Iterator(const Set<V, H, Eq>::Iterator<t_const_> & iterator) :
    m_set(iterator.m_set),
    m_i(iterator.m_i)
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
template <bool t_const_>
typename Set<V, H, Eq>::Iterator<t_const> & Set<V, H, Eq>::Iterator<t_const>::operator=(const Set<V, H, Eq>::Iterator<t_const_> & iterator) {
    m_set = iterator.m_set;
    m_i = iterator.m_i;
    return *this;
}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
typename Set<V, H, Eq>::Iterator<t_const> & Set<V, H, Eq>::Iterator<t_const>::operator++() {
    if (m_i < m_set->m_capacity) {
        do {
            ++m_i;
        } while (m_i < m_set->m_capacity && !m_set->m_dists[m_i]);
    }
    return *this;
}

//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
typename Set<V, H, Eq>::Iterator<t_const> Set<V, H, Eq>::Iterator<t_const>::operator++(int) {
    Set<V, H, Eq>::Iterator<t_const> temp(*this);
    operator++();
    return temp;
}

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, Eq>::Iterator<t_const>::operator==(const Set<V, H, Eq>::Iterator<t_const_> & o) const {
    return m_i == o.m_i && m_set == o.m_set;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, Eq>::Iterator<t_const>::operator!=(const Set<V, H, Eq>::Iterator<t_const_> & o) const {
    return m_i != o.m_i || m_set != o.m_set;
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
const V & Set<V, H, Eq>::Iterator<t_const>::operator*() const {
    return m_set->m_vals[m_i];
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H, typename Eq>
template <bool t_const>
const V * Set<V, H, Eq>::Iterator<t_const>::operator->() const {
    return m_set->m_vals + m_i;
}



}