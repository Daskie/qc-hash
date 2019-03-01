namespace qc {



//==============================================================================
// Map /////////////////////////////////////////////////////////////////////////
//==============================================================================



//==============================================================================
// Map
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
QC_MAP::Map(unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    m_size(0),
    m_bucketCount(minCapacity <= config::map::minCapacity ? config::map::minBucketCount : detail::hash::ceil2(minCapacity << 1)),
    m_buckets(nullptr),
    m_hash(hash),
    m_equal(equal),
    m_alloc(alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(unat minCapacity, const A & alloc) :
    Map(minCapacity, H(), E(), alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(unat minCapacity, const H & hash, const A & alloc) :
    Map(minCapacity, hash, E(), alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(const A & alloc) :
    Map(config::map::minCapacity, H(), E(), alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(const Map & other) :
    Map(other, std::allocator_traits<A>::select_on_container_copy_construction(other.m_alloc))
{}

QC_MAP_TEMPLATE
QC_MAP::Map(const Map & other, const A & alloc) :
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

QC_MAP_TEMPLATE
QC_MAP::Map(Map && other) :
    Map(std::move(other), std::move(other.m_alloc))
{}

QC_MAP_TEMPLATE
QC_MAP::Map(Map && other, const A & alloc) :
    Map(std::move(other), A(alloc))
{}

QC_MAP_TEMPLATE
QC_MAP::Map(Map && other, A && alloc) :
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

QC_MAP_TEMPLATE
template <typename It>
QC_MAP::Map(It first, It last, unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    Map(minCapacity ? minCapacity : std::distance(first, last), hash, equal, alloc)
{
    insert(first, last);
}

QC_MAP_TEMPLATE
template <typename It>
QC_MAP::Map(It first, It last, unat minCapacity, const A & alloc) :
    Map(first, last, minCapacity, H(), E(), alloc)
{}

QC_MAP_TEMPLATE
template <typename It>
QC_MAP::Map(It first, It last, unat minCapacity, const H & hash, const A & alloc) :
    Map(first, last, minCapacity, hash, E(), alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(std::initializer_list<V> values, unat minCapacity, const H & hash, const E & equal, const A & alloc) :
    Map(minCapacity ? minCapacity : values.size(), hash, equal, alloc)
{
    insert(values);
}

QC_MAP_TEMPLATE
QC_MAP::Map(std::initializer_list<V> values, unat minCapacity, const A & alloc) :
    Map(values, minCapacity, H(), E(), alloc)
{}

QC_MAP_TEMPLATE
QC_MAP::Map(std::initializer_list<V> values, unat minCapacity, const H & hash, const A & alloc) :
    Map(values, minCapacity, hash, E(), alloc)
{}

//==============================================================================
// ~Map
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
QC_MAP::~Map() {
    if (m_buckets) {
        clear_private<false>();
        deallocate();
    }
}

//==============================================================================
// operatator=
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
QC_MAP & QC_MAP::operator=(const Map & other) {
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

QC_MAP_TEMPLATE
QC_MAP & QC_MAP::operator=(Map && other) noexcept {
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
        moveBuckets(other.m_buckets);
        other.clear_private<false>();
        other.deallocate();
    }

    other.m_size = 0;
    other.m_bucketCount = 0;

    return *this;
}

QC_MAP_TEMPLATE
QC_MAP & QC_MAP::operator=(std::initializer_list<V> values) {
    clear();
    insert(values);

    return *this;
}

//==============================================================================
// begin
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::begin() noexcept {
    return begin_private<false>();
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::begin() const noexcept {
    return begin_private<true>();
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::cbegin() const noexcept {
    return begin_private<true>();
}

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const> QC_MAP::begin_private() const noexcept {
    if (!m_size) {
        return end_private<t_const>();
    }

    for (unat i(0); ; ++i) {
        if (m_buckets[i].dist) {
            return makeIterator<t_const>(m_buckets + i);
        }
    }
}

//==============================================================================
// end
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::end() noexcept {
    return end_private<false>();
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::end() const noexcept {
    return end_private<true>();
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::cend() const noexcept {
    return end_private<true>();
}

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const> QC_MAP::end_private() const noexcept {
    return makeIterator<t_const>(m_buckets + m_bucketCount);
}

//==============================================================================
// capacity
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::capacity() const {
    return m_bucketCount >> 1;
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::bucket_count() const {
    return m_bucketCount;
}

//==============================================================================
// max_bucket_count
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::max_bucket_count() const {
    return std::numeric_limits<unat>::max() - 1;
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::bucket_size(unat i) const {
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

QC_MAP_TEMPLATE
unat QC_MAP::bucket(const K & key) const {
    return detIndex(m_hash(key));
}

//==============================================================================
// empty
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
bool QC_MAP::empty() const noexcept {
    return m_size == 0;
}

//==============================================================================
// size
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::size() const noexcept {
    return m_size;
}

//==============================================================================
// max_size
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::max_size() const {
    return max_bucket_count() >> 1;
}

//==============================================================================
// clear
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
void QC_MAP::clear() {
    clear_private<true>();
}

QC_MAP_TEMPLATE
template <bool t_zeroDists>
void QC_MAP::clear_private() {
    if constexpr (std::is_trivially_destructible_v<V>) {
        if constexpr (t_zeroDists) {
            if (m_size) zeroDists();
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist) {
                m_buckets[i].val().~V();
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

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, bool> QC_MAP::insert(const V & value) {
    return emplace(value);
}

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, bool> QC_MAP::insert(V && value) {
    return emplace(std::move(value));
}

QC_MAP_TEMPLATE
template <typename V_, std::enable_if_t<std::is_constructible_v<typename detail::map::Types<K, T>::V, V_ &&>, int>>
std::pair<typename QC_MAP::iterator, bool> QC_MAP::insert(V_ && value) {
    return emplace(std::forward<V_>(value));
}

QC_MAP_TEMPLATE
template <typename It>
void QC_MAP::insert(It first, It last) {
    while (first != last) {
        emplace(*first);
        ++first;
    }
}

QC_MAP_TEMPLATE
void QC_MAP::insert(std::initializer_list<V> values) {
    for (const V & value : values) {
        emplace(value);
    }
}

//==============================================================================
// emplace
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, bool> QC_MAP::emplace(const V & value) {
    if constexpr (k_isSet) {
        return try_emplace(value);
    }
    else {
        return try_emplace(value.first, value.second);
    }
}

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, bool> QC_MAP::emplace(V && value) {
    if constexpr (k_isSet) {
        return try_emplace(std::move(value));
    }
    else {
        return try_emplace(std::move(value.first), std::move(value.second));
    }
}

QC_MAP_TEMPLATE
template <typename... Args>
std::pair<typename QC_MAP::iterator, bool> QC_MAP::emplace(Args &&... args) {
    return emplace(std::move(V(std::forward<Args>(args)...)));
}

//==============================================================================
// try_emplace
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <typename... Args>
std::pair<typename QC_MAP::iterator, bool> QC_MAP::try_emplace(const K & key, Args &&... args) {
    return try_emplace_private(m_hash(key), key, std::forward<Args>(args)...);
}

QC_MAP_TEMPLATE
template <typename... Args>
std::pair<typename QC_MAP::iterator, bool> QC_MAP::try_emplace(K && key, Args &&... args) {
    return try_emplace_private(m_hash(key), std::move(key), std::forward<Args>(args)...);
}

QC_MAP_TEMPLATE
template <typename K_, typename... Args>
std::pair<typename QC_MAP::iterator, bool> QC_MAP::try_emplace_private(unat hash, K_ && key, Args &&... args) {
    static_assert(sizeof...(Args) == 0 || std::is_default_constructible_v<T>, "Member type must be default constructible");

    if (!m_buckets) allocate();
    unat i(detIndex(hash));
    Dist dist(1);

    while (true) {
        Bucket & bucket(m_buckets[i]);

        // Can be inserted
        if (bucket.dist < dist) {
            if (m_size >= (m_bucketCount >> 1)) {
                rehash_private(m_bucketCount << 1);
                return try_emplace_private(hash, std::forward<K_>(key), std::forward<Args>(args)...);
            }

            // Value here has smaller dist, robin hood
            if (bucket.dist) {
                propagate(bucket.val(), i + 1, bucket.dist + 1);
                bucket.val().~V();
            }

            // Open slot
            AllocatorTraits::construct(m_alloc, &bucket.key, std::forward<K_>(key));
            if constexpr (!k_isSet) {
                AllocatorTraits::construct(m_alloc, &bucket.mem, std::forward<Args>(args)...);
            }

            bucket.dist = dist;
            ++m_size;
            return { iterator(&bucket), true };
        }

        // Value already exists
        if (m_equal(bucket.key, key)) {
            return { iterator(&bucket), false };
        }

        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
    }

    // Will never reach reach this return
    return { end(), false };
}

QC_MAP_TEMPLATE
void QC_MAP::propagate(V & value, unat i, Dist dist) {
    while (true) {
        if (i >= m_bucketCount) i = 0;
        Bucket & bucket(m_buckets[i]);

        if (!bucket.dist) {
            AllocatorTraits::construct(m_alloc, &bucket.val(), std::move(value));
            bucket.dist = dist;
            return;
        }

        if (bucket.dist < dist) {
            std::swap(value, bucket.val());
            std::swap(dist, bucket.dist);
        }

        ++i;
        ++dist;
    }
}

//==============================================================================
// erase
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::erase(const K & key) {
    iterator it(find(key));
    if (it == end()) {
        return 0;
    }
    erase_private(it);
    if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::set::minBucketCount) {
        rehash_private(m_bucketCount >> 1);
    }
    return 1;
}

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::erase(const_iterator position) {
    iterator endIt(end());
    if (position != endIt) {
        erase_private(position);
        if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::map::minBucketCount) {
            rehash_private(m_bucketCount >> 1);
            endIt = end();
        }
    }
    return endIt;
}

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::erase(const_iterator first, const_iterator last) {
    if (first != last) {
        do {
            erase_private(first);
            ++first;
        } while (first != last);
        reserve(m_size);
    }
    return end();
}

QC_MAP_TEMPLATE
void QC_MAP::erase_private(const_iterator position) {
    unat i(position.m_bucket - m_buckets), j(i + 1);

    while (true) {
        if (j >= m_bucketCount) j = 0;

        if (m_buckets[j].dist <= 1) {
            break;
        }

        m_buckets[i].val() = std::move(m_buckets[j].val());
        m_buckets[i].dist = m_buckets[j].dist - 1;

        ++i; ++j;
        if (i >= m_bucketCount) i = 0;
    }

    m_buckets[i].val().~V();
    m_buckets[i].dist = 0;
    --m_size;
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
void QC_MAP::swap(Map & other) noexcept {
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
// at
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
std::add_lvalue_reference_t<T> QC_MAP::at(const K & key) {
    return const_cast<std::add_lvalue_reference_t<T>>(const_cast<const Map *>(this)->at(key));
}

QC_MAP_TEMPLATE
std::add_lvalue_reference_t<const T> QC_MAP::at(const K & key) const {
    if constexpr (!k_isSet) {
        return find(key)->second;
    }
}

//==============================================================================
// operator[]
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
std::add_lvalue_reference_t<T> QC_MAP::operator[](const K & key) {
    if constexpr (k_isSet) {
        try_emplace(key);
    }
    else {
        return try_emplace(key).first->second;
    }
}

QC_MAP_TEMPLATE
std::add_lvalue_reference_t<T> QC_MAP::operator[](K && key) {
    if constexpr (k_isSet) {
        try_emplace(std::move(key));
    }
    else {
        return try_emplace(std::move(key)).first->second;
    }
}

//==============================================================================
// count
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
unat QC_MAP::count(const K & key) const {
    return contains(key);
}

QC_MAP_TEMPLATE
unat QC_MAP::count(const K & key, unat hash) const {
    return contains(key, hash);
}

//==============================================================================
// find
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::find(const K & key) {
    return find(key, m_hash(key));
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::find(const K & key) const {
    return find(key, m_hash(key));
}

QC_MAP_TEMPLATE
typename QC_MAP::iterator QC_MAP::find(const K & key, unat hash) {
    return find_private<false>(key, hash);
}

QC_MAP_TEMPLATE
typename QC_MAP::const_iterator QC_MAP::find(const K & key, unat hash) const {
    return find_private<true>(key, hash);
}

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const> QC_MAP::find_private(const K & key, unat hash) const {
    if (!m_buckets) {
        return end_private<t_const>();
    }

    unat i(detIndex(hash));
    Dist dist(1);

    while (true) {
        const Bucket & bucket(m_buckets[i]);

        if (m_equal(bucket.key, key)) {
            return makeIterator<t_const>(&bucket);
        }

        if (bucket.dist < dist) {
            return end_private<t_const>();
        }

        ++i;
        ++dist;

        if (i >= m_bucketCount) i = 0;
    };

    // Will never reach reach this return
    return end_private<t_const>();
}

//==============================================================================
// equal_range
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, typename QC_MAP::iterator> QC_MAP::equal_range(const K & key) {
    return equal_range(key, m_hash(key));
}

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::const_iterator, typename QC_MAP::const_iterator> QC_MAP::equal_range(const K & key) const {
    return equal_range(key, m_hash(key));
}

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::iterator, typename QC_MAP::iterator> QC_MAP::equal_range(const K & key, unat hash) {
    return equal_range_private<false>(key, hash);
}

QC_MAP_TEMPLATE
std::pair<typename QC_MAP::const_iterator, typename QC_MAP::const_iterator> QC_MAP::equal_range(const K & key, unat hash) const {
    return equal_range_private<true>(key, hash);
}

QC_MAP_TEMPLATE
template <bool t_const>
std::pair<typename QC_MAP::Iterator<t_const>, typename QC_MAP::Iterator<t_const>> QC_MAP::equal_range_private(const K & key, unat hash) const {
    Iterator<t_const> it(find_private<t_const>(key, hash));
    return { it, it };
}

//==============================================================================
// contains
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
bool QC_MAP::contains(const K & key) const {
    return contains(key, m_hash(key));
}

QC_MAP_TEMPLATE
bool QC_MAP::contains(const K & key, unat hash) const {
    return find(key, hash) != cend();
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
float QC_MAP::load_factor() const {
    return float(m_size) / float(m_bucketCount);
}

//==============================================================================
// load_factor
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
float QC_MAP::max_load_factor() const {
    return 0.5f;
}

//==============================================================================
// rehash
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
void QC_MAP::rehash(unat bucketCount) {
    bucketCount = detail::hash::ceil2(bucketCount);
    if (bucketCount < config::map::minBucketCount) bucketCount = config::map::minBucketCount;
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

QC_MAP_TEMPLATE
void QC_MAP::reserve(unat capacity) {
    rehash(capacity << 1);
}

//==============================================================================
// hash_function
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::hasher QC_MAP::hash_function() const {
    return m_hash;
}

//==============================================================================
// key_eq
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::key_equal QC_MAP::key_eq() const {
    return m_equal;
}

//==============================================================================
// get_allocator
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
typename QC_MAP::allocator_type QC_MAP::get_allocator() const {
    return m_alloc;
}

// Non-member Functions ////////////////////////////////////////////////////////

//==============================================================================
// operator==
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
inline bool operator==(const QC_MAP & s1, const QC_MAP & s2) {
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

QC_MAP_TEMPLATE
inline bool operator!=(const QC_MAP & s1, const QC_MAP & s2) {
    return !(s1 == s2);
}

//==============================================================================
// swap
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
inline void swap(QC_MAP & s1, QC_MAP & s2) noexcept {
    s1.swap(s2);
}

// Private Methods /////////////////////////////////////////////////////////////

QC_MAP_TEMPLATE
void QC_MAP::rehash_private(unat bucketCount) {
    unat oldSize(m_size);
    unat oldBucketCount(m_bucketCount);
    Bucket * oldBuckets(m_buckets);

    m_size = 0;
    m_bucketCount = bucketCount;
    allocate();

    for (unat i(0), n(0); n < oldSize; ++i) {
        Bucket & bucket(oldBuckets[i]);
        if (bucket.dist) {
            emplace(std::move(bucket.val()));
            bucket.val().~V();
            ++n;
        }
    }

    AllocatorTraits::deallocate(m_alloc, oldBuckets, oldBucketCount + 1);
}

QC_MAP_TEMPLATE
void QC_MAP::allocate() {
    m_buckets = AllocatorTraits::allocate(m_alloc, m_bucketCount + 1);
    zeroDists();
    m_buckets[m_bucketCount].dist = std::numeric_limits<Dist>::max();
}

QC_MAP_TEMPLATE
void QC_MAP::deallocate() {
    AllocatorTraits::deallocate(m_alloc, m_buckets, m_bucketCount + 1);
    m_buckets = nullptr;
}

QC_MAP_TEMPLATE
void QC_MAP::zeroDists() {
    if constexpr (sizeof(Dist) == 1 || sizeof(Dist) >= sizeof(V) && sizeof(Dist) != sizeof(detail::hash::utype_fast<sizeof(Dist)>)) {
        std::memset(m_buckets, 0, m_bucketCount * sizeof(Bucket));
    }
    else {
        for (unat i(0); i < m_bucketCount; ++i) m_buckets[i].dist = 0;
    }
}

QC_MAP_TEMPLATE
void QC_MAP::copyBuckets(const Bucket * buckets) {
    if constexpr (std::is_trivially_copyable_v<V>) {
        if (m_size) {
            std::memcpy(m_buckets, buckets, m_bucketCount * sizeof(Bucket));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist = buckets[i].dist) {
                AllocatorTraits::construct(m_alloc, &m_buckets[i].val(), buckets[i].val());
                ++n;
            }
        }
    }
}

QC_MAP_TEMPLATE
void QC_MAP::moveBuckets(Bucket * buckets) {
    if constexpr (std::is_trivially_copyable_v<V>) {
        if (m_size) {
            std::memcpy(m_buckets, buckets, m_bucketCount * sizeof(Bucket));
        }
    }
    else {
        for (unat i(0), n(0); n < m_size; ++i) {
            if (m_buckets[i].dist = buckets[i].dist) {
                AllocatorTraits::construct(m_alloc, &m_buckets[i].val(), std::move(buckets[i].val()));
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

QC_MAP_TEMPLATE
template <bool t_const>
template <bool t_const_, typename>
QC_MAP::Iterator<t_const>::Iterator(const Iterator<!t_const> & other) noexcept :
    m_bucket(other.m_bucket)
{}

QC_MAP_TEMPLATE
template <bool t_const>
QC_MAP::Iterator<t_const>::Iterator(Bucket * bucket) noexcept :
    m_bucket(bucket)
{}

//==============================================================================
// operator=
//------------------------------------------------------------------------------

//QC_MAP_TEMPLATE
//template <bool t_const>
//template <bool t_const_, typename>
//typename QC_MAP::Iterator<t_const> & QC_MAP::Iterator<t_const>::operator=(const Iterator<!t_const> & other) noexcept {
//    m_bucket = other.m_bucket;
//
//    return *this;
//}

//==============================================================================
// operator++
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const> & QC_MAP::Iterator<t_const>::operator++() {
    do {
        ++m_bucket;
    } while (!m_bucket->dist);

    return *this;
}

//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const> QC_MAP::Iterator<t_const>::operator++(int) {
    Iterator temp(*this);
    operator++();
    return temp;
}

//==============================================================================
// operator==
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
template <bool t_const_>
bool QC_MAP::Iterator<t_const>::operator==(const Iterator<t_const_> & o) const {
    return m_bucket == o.m_bucket;
}

//==============================================================================
// operator!=
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
template <bool t_const_>
bool QC_MAP::Iterator<t_const>::operator!=(const Iterator<t_const_> & o) const {
    return m_bucket != o.m_bucket;
}

//==============================================================================
// operator*
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const>::value_type & QC_MAP::Iterator<t_const>::operator*() const {
    return m_bucket->val();
}

//==============================================================================
// operator->
//------------------------------------------------------------------------------

QC_MAP_TEMPLATE
template <bool t_const>
typename QC_MAP::Iterator<t_const>::value_type * QC_MAP::Iterator<t_const>::operator->() const {
    return &m_bucket->val();
}



}