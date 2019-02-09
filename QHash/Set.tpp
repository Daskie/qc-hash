namespace qc {



//==============================================================================
// Set /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::Set(unat minCapacity) :
    Set(PrivateTag(), minCapacity <= 4 ? 8 : detail::ceil2(2 * minCapacity))
{}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(const Set<V, H, E> & other) :
    Set(PrivateTag(), other.m_capacity)
{    
    allocate();
    m_size = other.m_size;
    copyChunks(other.m_chunks);
}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(Set<V, H, E> && other) :
    m_size(other.m_size),
    m_capacity(other.m_capacity),
    m_chunks(other.m_chunks)
{
    other.m_size = 0;
    other.m_capacity = 0;
    other.m_chunks = nullptr;
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
Set<V, H, E>::Set(PrivateTag, unat capacity) :
    m_size(0),
    m_capacity(capacity),
    m_chunks(nullptr)
{}

//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::~Set() {
    if (m_chunks) {
        clear();
        std::free(m_chunks);
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
        std::free(m_chunks);
        m_capacity = other.m_capacity;
        allocate();
    }
    m_size = other.m_size;

    copyChunks(other.m_chunks);

    return *this;
}

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(Set<V, H, E> && other) {
    if (&other == this) {
        return *this;
    }

    if (m_chunks) {
        clear();
        std::free(m_chunks);
    }

    m_size = other.m_size;
    m_capacity = other.m_capacity;
    m_chunks = other.m_chunks;

    other.m_size = 0;
    other.m_capacity = 0;
    other.m_chunks = nullptr;

    return *this;
}

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(std::initializer_list<V> values) {
    return *this = std::move(Set<V, H, E>(values));
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

    for (unat ci(0); ; ++ci) {
        for (unat cj(0); cj < 8; ++cj) {
            if (m_chunks[ci].dists[cj]) {
                return const_iterator(m_chunks[ci].dists + cj);
            }
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
    return const_iterator(reinterpret_cast<unat>(m_chunks) + m_capacity * (sizeof(Chunk) >> 3));
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
    if (i >= m_capacity || !m_chunks) {
        return 0;
    }

    ChunkIndex ci(toChunkIndex(i));
    unat dist(1);
    while (distAt(ci) > dist) {
        increment(ci);
        ++dist;
    }
    
    unat n(0);
    while (distAt(ci) == dist) {
        increment(ci);
        ++dist;
        ++n;
    }

    return n;
}

//==============================================================================
// bucket
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket(const V & value) const {
    return detIndex(value);
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
    return std::numeric_limits<unat>::max() >> 1;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::clear() {
    if constexpr (std::is_trivial_v<V>) {
        if (m_size) {
            std::memset(m_chunks, 0, m_capacity * (sizeof(Chunk) >> 3));
        }
    }
    else {
        for (unat ci(0), vi(0); vi < m_size; ++ci) {
            Chunk & chunk(m_chunks[ci]);
            for (unat cj(0); cj < 8; ++cj) {
                if (chunk.dists[cj]) {
                    chunk.vals[cj].~V();
                    ++vi;
                }
            }
            chunk.distsVal = 0;
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
    if (!m_chunks) allocate();
    
    ChunkIndex ci(toChunkIndex(detIndex(hash)));
    uchar dist(1);

    while (true) {
        // Can be inserted
        if (distAt(ci) < dist) {
            if (m_size >= (m_capacity >> 1)) {
                rehash(PrivateTag(), m_capacity << 1);
                return emplace_h(std::move(value), hash);
            }

            // Value here has smaller dist, robin hood
            if (distAt(ci)) {
                V temp(std::move(valAt(ci)));
                valAt(ci) = std::move(value);
                propagate(temp, ci, distAt(ci) + uchar(1));
            }
            // Open slot
            else {
                new (&valAt(ci)) V(std::move(value));
            }

            distAt(ci) = dist;
            ++m_size;
            return { iterator(&distAt(ci)), true };
        }

        // Value already exists
        if (E()(valAt(ci), value)) {
            return { iterator(&distAt(ci)), false };
        }

        increment(ci);
        ++dist;
    }

    // Will never reach reach this return
    return { end(), false };
}

template <typename V, typename H, typename E>
void Set<V, H, E>::propagate(V & value, ChunkIndex ci, uchar dist) {
    increment(ci);

    while (true) {
        if (!distAt(ci)) {
            new (&valAt(ci)) V(std::move(value));
            distAt(ci) = dist;
            return;
        }

        if (distAt(ci) < dist) {
            std::swap(value, valAt(ci));
            std::swap(dist, distAt(ci));
        }

        increment(ci);
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
    if (position == endIt || !m_chunks) {
        return endIt;
    }

    ChunkIndex ci(toChunkIndex(position));
    ChunkIndex cj(ci);
    increment(cj);

    while (true) {
        if (distAt(cj) <= uchar(1)) {
            break;
        }

        valAt(ci) = std::move(valAt(cj));
        distAt(ci) = distAt(cj) - uchar(1);

        increment(ci);
        increment(cj);
    }

    valAt(ci).~V();
    distAt(ci) = uchar(0);
    --m_size;

    while (!*position.m_dist.ptr) {
        ++position; // TODO: consider adding total zero check
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
    std::swap(m_chunks, other.m_chunks);
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
    if (!m_chunks) {
        return cend();
    }
    
    ChunkIndex ci(toChunkIndex(detIndex(value)));
    uchar dist(1);

    while (true) {
        if (E()(valAt(ci), value)) {
            return const_iterator(&distAt(ci));
        }

        if (distAt(ci) < dist) {
            return cend();
        }

        increment(ci);
        ++dist;
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
    return 0.5f;
}

//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::rehash(unat minCapacity) {
    if (minCapacity <= 8) minCapacity = 8;
    else minCapacity = detail::ceil2(minCapacity);

    if (minCapacity >= 2 * m_size && minCapacity != m_capacity) {
        if (m_chunks) {
            rehash(PrivateTag(), minCapacity);
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
void Set<V, H, E>::rehash(PrivateTag, unat capacity) {
    unat oldSize(m_size);
    Chunk * oldChunks(m_chunks);

    m_size = 0;
    m_capacity = capacity;
    allocate();

    for (unat ci(0), vi(0); vi < oldSize; ++ci) {
        Chunk & chunk(oldChunks[ci]);
        for (unat cj(0); cj < 8; ++cj) {
            // TODO: maybe skip all?
            if (chunk.dists[cj]) {
                emplace(std::move(chunk.vals[cj]));
                chunk.vals[cj].~V();
                ++vi;
            }
        }
    }

    std::free(oldChunks);
}

template <typename V, typename H, typename E>
void Set<V, H, E>::allocate() {
    m_chunks = reinterpret_cast<Chunk *>(std::calloc(m_capacity >> 3, sizeof(Chunk)));
}

template <typename V, typename H, typename E>
void Set<V, H, E>::copyChunks(const Chunk * chunks) {
    if constexpr (std::is_trivial_v<V>) {
        if (m_size) {
            std::memcpy(m_chunks, chunks, m_capacity * (sizeof(Chunk) >> 3));
        }
    }
    else {
        for (unat ci(0), vi(0); vi < m_size; ++ci) {
            m_chunks[ci].distsVal = chunks[ci].distsVal;
            for (unat cj(0); cj < 8; ++cj) {
                if (chunks[ci].dists[cj] > uchar(0)) {
                    new (m_chunks[ci].vals + cj) V(chunks[ci].vals[cj]);
                    ++vi;
                }
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
Set<V, H, E>::Iterator<t_const>::Iterator(const uchar * distPtr) :
    m_dist{ distPtr }
{}

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
Set<V, H, E>::Iterator<t_const>::Iterator(const Set<V, H, E>::Iterator<t_const_> & other) :
    m_dist{ other.m_dist.ptr }
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator=(const Set<V, H, E>::Iterator<t_const_> & other) {
    m_dist.ptr = other.m_dist.ptr;

    return *this;
}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator++() {
    do {
        ++m_dist.addr;
        m_dist.addr += !(m_dist.addr & 7) * (sizeof(Chunk) - 8); // TODO: compare performance
        //if (!(m_dist & 0b111)) m_dist += sizeof(Chunk) - 8;
        // TODO: consider skipping if 0 dist, maybe dynamically
    } while (!*m_dist.ptr);

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
    return m_dist.addr == o.m_dist.addr;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E>::Iterator<t_const>::operator!=(const Set<V, H, E>::Iterator<t_const_> & o) const {
    return m_dist.addr != o.m_dist.addr;
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V & Set<V, H, E>::Iterator<t_const>::operator*() const {
    return *operator->();
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V * Set<V, H, E>::Iterator<t_const>::operator->() const {
    constexpr uintptr_t k_mask(0b111);
    return reinterpret_cast<const V *>((m_dist.addr & ~k_mask) + 8 + (m_dist.addr & k_mask) * sizeof(V));
}



}