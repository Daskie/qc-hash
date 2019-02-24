namespace qc {



//==============================================================================
// Set /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    m_size(0),
    m_bucketCount(minCapacity <= config::set::minCapacity ? config::set::minBucketCount : detail::hash::ceil2(minCapacity << 1)),
    m_buckets(nullptr),
    m_hash(hash),
    m_equal(equal),
    m_alloc(alloc)
{}
template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(unat minCapacity, const A & alloc) :
    Set(minCapacity, H(), E(), alloc)
{}
template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(unat minCapacity, const H & hash, const A & alloc) :
    Set(minCapacity, hash, E(), alloc)
{}
template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(const A & alloc) :
    Set(config::set::minCapacity, H(), E(), alloc)
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(const Set & other) :
    Set(other, std::allocator_traits<A>::select_on_container_copy_construction(other.m_alloc))
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(const Set & other, const A & alloc) :
    m_size(other.m_size),
    m_bucketCount(other.m_bucketCount),
    m_buckets(nullptr),
    m_hash(other.m_hash),
    m_equal(other.m_equal),
    m_alloc(alloc)
{    
    allocate();
    copyBuckets(other.m_buckets);
}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(Set && other) :
    Set(std::move(other), std::move(other.m_alloc))
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(Set && other, const A & alloc) :
    Set(std::move(other), A(alloc))
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(Set && other, A && alloc) :
    m_size(other.m_size),
    m_bucketCount(other.m_bucketCount),
    m_buckets(other.m_buckets),
    m_hash(std::move(other.m_hash)),
    m_equal(std::move(other.m_equal)),
    m_alloc(std::move(alloc))
{
    other.m_size = 0;
    other.m_bucketCount = 0;
    other.m_buckets = nullptr;
}

template <typename V, typename H, typename E, typename A>
template <typename It>
Set<V, H, E, A>::Set(It first, It last, unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    Set(minCapacity ? minCapacity : std::distance(first, last), hash, equal, alloc)
{
    insert(first, last);
}

template <typename V, typename H, typename E, typename A>
template <typename It>
Set<V, H, E, A>::Set(It first, It last, unat minCapacity, const A & alloc) :
    Set(first, last, minCapacity, H(), E(), alloc)
{}

template <typename V, typename H, typename E, typename A>
template <typename It>
Set<V, H, E, A>::Set(It first, It last, unat minCapacity, const H & hash, const A & alloc) :
    Set(first, last, minCapacity, hash, E(), alloc)
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(std::initializer_list<V> values, unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    Set(minCapacity ? minCapacity : values.size(), hash, equal, alloc)
{
    insert(values);
}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(std::initializer_list<V> values, unat minCapacity, const A & alloc) :
    Set(values, minCapacity, H(), E(), alloc)
{}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::Set(std::initializer_list<V> values, unat minCapacity, const H & hash, const A & alloc) :
    Set(values, minCapacity, hash, E(), alloc)
{}

//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A>::~Set() {
    if (m_buckets) {
        clear_private<false>();
        deallocate();
    }
}

//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A> & Set<V, H, E, A>::operator=(const Set & other) {
    if (&other == this) {
        return *this;
    }

    if (m_buckets) {
        clear_private<false>();
        if (m_bucketCount != other.m_bucketCount || m_alloc != other.m_alloc) {
            deallocate();
        }
    }

    m_size = other.m_size;
    m_bucketCount = other.m_bucketCount;
    m_hash = other.m_hash;
    m_equal = other.m_equal;
    if constexpr (AllocatorTraits::propagate_on_container_copy_assignment::value) {
        m_alloc = std::allocator_traits<A>::select_on_container_copy_construction(other.m_alloc);
    }

    if (other.m_buckets) {
        if (!m_buckets) {
            allocate();
        }
        copyBuckets(other.m_buckets);
    }

    return *this;
}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A> & Set<V, H, E, A>::operator=(Set && other) noexcept {
    if (&other == this) {
        return *this;
    }

    if (m_buckets) {
        clear_private<false>();
        deallocate();
    }

    m_size = other.m_size;
    m_bucketCount = other.m_bucketCount;
    m_hash = std::move(other.m_hash);
    m_equal = std::move(other.m_equal);
    if constexpr (AllocatorTraits::propagate_on_container_move_assignment::value) {
        m_alloc = std::move(other.m_alloc);
    }

    if (AllocatorTraits::propagate_on_container_move_assignment::value || m_alloc == other.m_alloc) {
        m_buckets = other.m_buckets;
        other.m_buckets = nullptr;
    }
    else {
        allocate();
        copyBuckets<true>(other.m_buckets);
        other.clear_private<false>();
        other.deallocate();
    }

    other.m_size = 0;
    other.m_bucketCount = 0;

    return *this;
}

template <typename V, typename H, typename E, typename A>
Set<V, H, E, A> & Set<V, H, E, A>::operator=(std::initializer_list<V> values) {
    clear();
    insert(values);

    return *this;
}

//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::iterator Set<V, H, E, A>::begin() noexcept {
    return cbegin();
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::begin() const noexcept {
    return cbegin();
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::cbegin() const noexcept {
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

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::iterator Set<V, H, E, A>::end() noexcept {
    return cend();
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::end() const noexcept {
    return cend();
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::cend() const noexcept {
    return const_iterator(m_buckets + m_bucketCount);
}

//==============================================================================
// capacity
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::capacity() const {
    return m_bucketCount >> 1;
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::bucket_count() const {
    return m_bucketCount;
}

//==============================================================================
// max_bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::max_bucket_count() const {
    return std::numeric_limits<unat>::max() - 1;
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::bucket_size(unat i) const {
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

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::bucket(const V & value) const {
    return detIndex(m_hash(value));
}

//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
bool Set<V, H, E, A>::empty() const noexcept {
    return m_size == 0;
}

//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::size() const noexcept {
    return m_size;
}

//==============================================================================
// max_size
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::max_size() const {
    return max_bucket_count() >> 1;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::clear() {
    clear_private<true>();
}

template <typename V, typename H, typename E, typename A>
template <bool t_zeroDists>
void Set<V, H, E, A>::clear_private() {
    if constexpr (std::is_trivially_destructible_v<V>) {
        if constexpr (t_zeroDists) {
            if (m_size) zeroDists();
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist) {
                m_buckets[i].val.~V();
                if constexpr (t_zeroDists) {
                    m_buckets[i].dist = 0;
                }
                ++n;
            }
        }
    }

    m_size = 0;
}

//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::insert(const V & value) {
    return emplace(value);
}

template <typename V, typename H, typename E, typename A>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::insert(V && value) {
    return emplace(std::move(value));
}

template <typename V, typename H, typename E, typename A>
template <typename It>
void Set<V, H, E, A>::insert(It first, It last) {
    while (first != last) {
        emplace(*first);
        ++first;
    }
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::insert(std::initializer_list<V> values) {
    for (const V & value : values) {
        emplace(value);
    }
}

//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <typename... Args>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::emplace(Args &&... args) {
    V value(std::forward<Args>(args)...);
    return emplace_private(std::move(value), m_hash(value));
}

template <typename V, typename H, typename E, typename A>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::emplace(V && value) {
    return emplace_private(std::move(value), m_hash(value));
}

template <typename V, typename H, typename E, typename A>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::emplace_private(V && value, unat hash) {
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
                AllocatorTraits::construct(m_alloc, &bucket.val, std::move(value));
            }

            bucket.dist = dist;
            ++m_size;
            return { iterator(&bucket), true };
        }

        // Value already exists
        if (m_equal(bucket.val, value)) {
            return { iterator(&bucket), false };
        }

        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
    }

    // Will never reach reach this return
    return { end(), false };
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::propagate(V & value, unat i, Dist dist) {
    while (true) {
        if (i >= m_bucketCount) i = 0;
        Bucket & bucket(m_buckets[i]);

        if (!bucket.dist) {
            AllocatorTraits::construct(m_alloc, &bucket.val, std::move(value));
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

template<typename V, typename H, typename E, typename A>
template <typename... Args>
std::pair<typename Set<V, H, E, A>::iterator, bool> Set<V, H, E, A>::emplace_hint(const_iterator hint, Args &&... args) {
    return emplace(std::forward<Args>(args)...);
}

//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::erase(const V & value) {
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

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::iterator Set<V, H, E, A>::erase(const_iterator position) {
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

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::iterator Set<V, H, E, A>::erase(const_iterator first, const_iterator last) {
    if (first != last) {
        do {
            erase_private(first);
            ++first;
        } while (first != last);
        reserve(m_size);
    }
    return end();
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::erase_private(const_iterator position) {
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

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::swap(Set & other) noexcept {
    std::swap(m_size, other.m_size);
    std::swap(m_bucketCount, other.m_bucketCount);
    std::swap(m_buckets, other.m_buckets);
    std::swap(m_hash, other.m_hash);
    std::swap(m_equal, other.m_equal);
    if constexpr (AllocatorTraits::propagate_on_container_swap::value) {
        std::swap(m_alloc, other.m_alloc);
    }
}

//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
unat Set<V, H, E, A>::count(const V & value) const {
    return contains(value);
}

//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::iterator Set<V, H, E, A>::find(const V & value) {
    return cfind(value);
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::find(const V & value) const {
    return cfind(value);
}

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::const_iterator Set<V, H, E, A>::cfind(const V & value) const {
    if (!m_buckets) {
        return cend();
    }
    
    unat i(detIndex(m_hash(value)));
    Dist dist(1);

    while (true) {
        const Bucket & bucket(m_buckets[i]);

        if (m_equal(bucket.val, value)) {
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

template <typename V, typename H, typename E, typename A>
bool Set<V, H, E, A>::contains(const V & value) const {
    return find(value) != cend();
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
float Set<V, H, E, A>::load_factor() const {
    return float(m_size) / float(m_bucketCount);
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
float Set<V, H, E, A>::max_load_factor() const {
    return 0.5f;
}

//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::rehash(unat bucketCount) {
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

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::reserve(unat capacity) {
    rehash(capacity << 1);
}

//==============================================================================
// hash_function
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::hasher Set<V, H, E, A>::hash_function() const {
    return m_hash;
}

//==============================================================================
// key_eq
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::key_equal Set<V, H, E, A>::key_eq() const {
    return m_equal;
}

//==============================================================================
// get_allocator
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
typename Set<V, H, E, A>::allocator_type Set<V, H, E, A>::get_allocator() const {
    return m_alloc;
}

// Non-member Functions ////////////////////////////////////////////////////////

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
inline bool operator==(const Set<V, H, E, A> & s1, const Set<V, H, E, A> & s2) {
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

template <typename V, typename H, typename E, typename A>
inline bool operator!=(const Set<V, H, E, A> & s1, const Set<V, H, E, A> & s2) {
    return !(s1 == s2);
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
inline void swap(Set<V, H, E, A> & s1, Set<V, H, E, A> & s2) noexcept {
    s1.swap(s2);
}

// Private Methods /////////////////////////////////////////////////////////////

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::rehash_private(unat bucketCount) {
    unat oldSize(m_size);
    unat oldBucketCount(m_bucketCount);
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

    AllocatorTraits::deallocate(m_alloc, oldBuckets, oldBucketCount + 1);
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::allocate() {
    m_buckets = AllocatorTraits::allocate(m_alloc, m_bucketCount + 1);
    zeroDists();
    m_buckets[m_bucketCount].dist = std::numeric_limits<Dist>::max();
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::deallocate() {
    AllocatorTraits::deallocate(m_alloc, m_buckets, m_bucketCount + 1);
    m_buckets = nullptr;
}

template <typename V, typename H, typename E, typename A>
void Set<V, H, E, A>::zeroDists() {
    if constexpr (sizeof(Dist) == 1 || sizeof(Dist) >= sizeof(V) && sizeof(Dist) != sizeof(detail::hash::utype_fast<sizeof(Dist)>)) {
        std::memset(m_buckets, 0, m_bucketCount * sizeof(Bucket));
    }
    else {
        for (unat i(0); i < m_bucketCount; ++i) m_buckets[i].dist = 0;
    }
}

template <typename V, typename H, typename E, typename A>
template <bool t_move>
void Set<V, H, E, A>::copyBuckets(const Bucket * buckets) {
    if constexpr (std::is_trivially_copyable_v<V>) {
        if (m_size) {
            std::memcpy(m_buckets, buckets, m_bucketCount * sizeof(Bucket));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist = buckets[i].dist) {
                if constexpr (t_move) {
                    AllocatorTraits::construct(m_alloc, &m_buckets[i].val, std::move(buckets[i].val));
                }
                else {
                    AllocatorTraits::construct(m_alloc, &m_buckets[i].val, buckets[i].val);
                }
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

template <typename V, typename H, typename E, typename A>
template <bool t_const>
Set<V, H, E, A>::Iterator<t_const>::Iterator(const Iterator<!t_const> & other) noexcept :
    m_bucket(other.m_bucket)
{}

template <typename V, typename H, typename E, typename A>
template <bool t_const>
Set<V, H, E, A>::Iterator<t_const>::Iterator(const Bucket * bucket) noexcept :
    m_bucket(bucket)
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
typename Set<V, H, E, A>::Iterator<t_const> & Set<V, H, E, A>::Iterator<t_const>::operator=(const Iterator<!t_const> & other) noexcept {
    m_bucket = other.m_bucket;

    return *this;
}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
typename Set<V, H, E, A>::Iterator<t_const> & Set<V, H, E, A>::Iterator<t_const>::operator++() {
    do {
        ++m_bucket;
    } while (!m_bucket->dist);

    return *this;
}

//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
typename Set<V, H, E, A>::Iterator<t_const> Set<V, H, E, A>::Iterator<t_const>::operator++(int) {
    Iterator temp(*this);
    operator++();
    return temp;
}

//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E, A>::Iterator<t_const>::operator==(const Iterator<t_const_> & o) const {
    return m_bucket == o.m_bucket;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
template <bool t_const_>
bool Set<V, H, E, A>::Iterator<t_const>::operator!=(const Iterator<t_const_> & o) const {
    return m_bucket != o.m_bucket;
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
const V & Set<V, H, E, A>::Iterator<t_const>::operator*() const {
    return m_bucket->val;
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H, typename E, typename A>
template <bool t_const>
const V * Set<V, H, E, A>::Iterator<t_const>::operator->() const {
    return &m_bucket->val;
}



}