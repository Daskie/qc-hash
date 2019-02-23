namespace qc {



//==============================================================================
// Set /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::Set(unat minCapacity) :
    Set(minCapacity <= config::set::minCapacity ?
        config::set::minBucketCount :
        detail::hash::ceil2(minCapacity << 1), PrivateTag{})
{}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(const Set<V, H, E> & other) :
    Set(other.m_bucketCount, PrivateTag{})
{    
    allocate();
    m_size = other.m_size;
    copyBuckets(other.m_buckets);
}

template <typename V, typename H, typename E>
Set<V, H, E>::Set(Set<V, H, E> && other) :
    m_size(other.m_size),
    m_bucketCount(other.m_bucketCount),
    m_buckets(other.m_buckets)
{
    other.m_size = 0;
    other.m_bucketCount = 0;
    other.m_buckets = nullptr;
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
Set<V, H, E>::Set(unat bucketCount, PrivateTag) :
    m_size(0),
    m_bucketCount(bucketCount),
    m_buckets(nullptr)
{}

//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
Set<V, H, E>::~Set() {
    if (m_buckets) {
        if constexpr (!std::is_trivially_destructible_v<V>) {
            for (unat i(0), n(0); n < m_size; ++i) {
                if (m_buckets[i].dist) {
                    m_buckets[i].val.~V();
                    ++n;
                }
            }
        }
        std::free(m_buckets);
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

    if (m_bucketCount != other.m_bucketCount) {
        std::free(m_buckets);
        m_bucketCount = other.m_bucketCount;
        allocate();
    }
    m_size = other.m_size;

    copyBuckets(other.m_buckets);

    return *this;
}

template <typename V, typename H, typename E>
Set<V, H, E> & Set<V, H, E>::operator=(Set<V, H, E> && other) {
    if (&other == this) {
        return *this;
    }

    if (m_buckets) {
        clear();
        std::free(m_buckets);
    }

    m_size = other.m_size;
    m_bucketCount = other.m_bucketCount;
    m_buckets = other.m_buckets;

    other.m_size = 0;
    other.m_bucketCount = 0;
    other.m_buckets = nullptr;

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
        if (m_buckets[i].dist) {
            return const_iterator(m_buckets + i);
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
    return const_iterator(m_buckets + m_bucketCount);
}

//==============================================================================
// capacity
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::capacity() const {
    return m_bucketCount >> 1;
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket_count() const {
    return m_bucketCount;
}

//==============================================================================
// max_bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::max_bucket_count() const {
    return std::numeric_limits<unat>::max() - 1;
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
unat Set<V, H, E>::bucket_size(unat i) const {
    if (i >= m_bucketCount || !m_buckets) {
        return 0;
    }

    Dist dist(1);
    while (m_buckets[i].dist > dist) {
        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
    }
    
    unat n(0);
    while (m_buckets[i].dist == dist) {
        ++i;
        ++dist;
        ++n;

        if (i >= m_bucketCount) i = 0;
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
    return max_bucket_count() >> 1;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::clear() {
    if constexpr (std::is_trivially_destructible_v<V>) {
        if (m_size) {
            std::memset(m_buckets, 0, m_bucketCount * sizeof(Bucket));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist) {
                m_buckets[i].val.~V();
                m_buckets[i].dist = 0;
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
    return emplace_private(std::move(value), H()(value));
}

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace(V && value) {
    return emplace_private(std::move(value), H()(value));
}

template <typename V, typename H, typename E>
std::pair<typename Set<V, H, E>::iterator, bool> Set<V, H, E>::emplace_private(V && value, unat hash) {
    if (!m_buckets) allocate();
    unat i(detIndex(hash));
    Dist dist(1);

    while (true) {
        Bucket & bucket(m_buckets[i]);

        // Can be inserted
        if (bucket.dist < dist) {
            if (m_size >= (m_bucketCount >> 1)) {
                rehash_private(m_bucketCount << 1);
                return emplace_private(std::move(value), hash);
            }

            // Value here has smaller dist, robin hood
            if (bucket.dist) {
                V temp(std::move(bucket.val));
                bucket.val = std::move(value);
                propagate(temp, i + 1, bucket.dist + 1);
            }
            // Open slot
            else {
                new (&bucket.val) V(std::move(value));
            }

            bucket.dist = dist;
            ++m_size;
            return { iterator(&bucket), true };
        }

        // Value already exists
        if (E()(bucket.val, value)) {
            return { iterator(&bucket), false };
        }

        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
    }

    // Will never reach reach this return
    return { end(), false };
}

template <typename V, typename H, typename E>
void Set<V, H, E>::propagate(V & value, unat i, Dist dist) {
    while (true) {
        if (i >= m_bucketCount) i = 0;
        Bucket & bucket(m_buckets[i]);

        if (!bucket.dist) {
            new (&bucket.val) V(std::move(value));
            bucket.dist = dist;
            return;
        }

        if (bucket.dist < dist) {
            std::swap(value, bucket.val);
            std::swap(dist, bucket.dist);
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
    if (it == end()) {
        return 0;
    }
    erase_private(it);
    if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::set::minBucketCount) {
        rehash_private(m_bucketCount >> 1);
    }
    return 1;
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::erase(const_iterator position) {
    iterator endIt(end());
    if (position != endIt) {
        erase_private(position);
        if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::set::minBucketCount) {
            rehash_private(m_bucketCount >> 1);
            endIt = end();
        }
    }
    return endIt;
}

template <typename V, typename H, typename E>
typename Set<V, H, E>::iterator Set<V, H, E>::erase(const_iterator first, const_iterator last) {
    while (first != last) {
        erase_private(first);
        ++first;
    }
    reserve(m_size);
    return end();
}

template <typename V, typename H, typename E>
void Set<V, H, E>::erase_private(const_iterator position) {
    unat i(position.m_bucket - m_buckets), j(i + 1);

    while (true) {
        if (j >= m_bucketCount) j = 0;

        if (m_buckets[j].dist <= 1) {
            break;
        }

        m_buckets[i].val = std::move(m_buckets[j].val);
        m_buckets[i].dist = m_buckets[j].dist - 1;

        ++i; ++j;
        if (i >= m_bucketCount) i = 0;
    }

    m_buckets[i].val.~V();
    m_buckets[i].dist = 0;
    --m_size;
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::swap(Set<V, H, E> & other) {
    std::swap(m_size, other.m_size);
    std::swap(m_bucketCount, other.m_bucketCount);
    std::swap(m_buckets, other.m_buckets);
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
    if (!m_buckets) {
        return cend();
    }
    
    unat i(detIndex(H()(value)));
    Dist dist(1);

    while (true) {
        const Bucket & bucket(m_buckets[i]);

        if (E()(bucket.val, value)) {
            return const_iterator(&bucket);
        }

        if (bucket.dist < dist) {
            return cend();
        }

        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
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
    return float(m_size) / float(m_bucketCount);
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
void Set<V, H, E>::rehash(unat bucketCount) {
    bucketCount = detail::hash::ceil2(bucketCount);
    if (bucketCount < config::set::minBucketCount) bucketCount = config::set::minBucketCount;
    else if (bucketCount < (m_size << 1)) bucketCount = m_size << 1;

    if (bucketCount != m_bucketCount) {
        if (m_buckets) {
            rehash_private(bucketCount);
        }
        else {
            m_bucketCount = bucketCount;
        }
    }
}

//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
void Set<V, H, E>::reserve(unat capacity) {
    rehash(capacity << 1);
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
void Set<V, H, E>::rehash_private(unat bucketCount) {
    unat oldSize(m_size);
    Bucket * oldBuckets(m_buckets);

    m_size = 0;
    m_bucketCount = bucketCount;
    allocate();

    for (unat i(0), n(0); n < oldSize; ++i) {
        Bucket & bucket(oldBuckets[i]);
        if (bucket.dist) {
            emplace(std::move(bucket.val));
            bucket.val.~V();
            ++n;
        }
    }

    std::free(oldBuckets);
}

template <typename V, typename H, typename E>
void Set<V, H, E>::allocate() {
    m_buckets = reinterpret_cast<Bucket *>(std::calloc(m_bucketCount + 1, sizeof(Bucket)));
    m_buckets[m_bucketCount].dist = std::numeric_limits<Dist>::max();
}

template <typename V, typename H, typename E>
void Set<V, H, E>::copyBuckets(const Bucket * buckets) {
    if constexpr (std::is_trivially_copyable_v<V>) {
        if (m_size) {
            std::memcpy(m_buckets, buckets, m_bucketCount * sizeof(Bucket));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist = buckets[i].dist) {
                new (&m_buckets[i].val) V(buckets[i].val);
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
Set<V, H, E>::Iterator<t_const>::Iterator(const Bucket * bucket) :
    m_bucket(bucket)
{}

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
Set<V, H, E>::Iterator<t_const>::Iterator(const Set<V, H, E>::Iterator<t_const_> & other) :
    m_bucket(other.m_bucket)
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator=(const Set<V, H, E>::Iterator<t_const_> & other) {
    m_bucket = other.m_bucket;

    return *this;
}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
typename Set<V, H, E>::Iterator<t_const> & Set<V, H, E>::Iterator<t_const>::operator++() {
    do {
        ++m_bucket;
    } while (!m_bucket->dist);

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
    return m_bucket == o.m_bucket;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E>::Iterator<t_const>::operator!=(const Set<V, H, E>::Iterator<t_const_> & o) const {
    return m_bucket != o.m_bucket;
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V & Set<V, H, E>::Iterator<t_const>::operator*() const {
    return m_bucket->val;
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H, typename E>
template <bool t_const>
const V * Set<V, H, E>::Iterator<t_const>::operator->() const {
    return &m_bucket->val;
}



}