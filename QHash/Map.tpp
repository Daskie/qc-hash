namespace qc {



namespace detail {

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr int log2Floor(T v) {
    static_assert(sizeof(T) <= 8);

    int log(0);
    if constexpr (sizeof(T) >= 8)
        if (v & 0xFFFFFFFF00000000ULL) { v >>= 32; log += 32; }
    if constexpr (sizeof(T) >= 4)
        if (v & 0x00000000FFFF0000ULL) { v >>= 16; log += 16; }
    if constexpr (sizeof(T) >= 2)
        if (v & 0x000000000000FF00ULL) { v >>=  8; log +=  8; }
    if (    v & 0x00000000000000F0ULL) { v >>=  4; log +=  4; }
    if (    v & 0x000000000000000CULL) { v >>=  2; log +=  2; }
    if (    v & 0x0000000000000002ULL) {           log +=  1; }
    return log;
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr int log2Ceil(T v) {
    return log2Floor(2 * v - 1);
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr T ceil2(T v) {
    return T(1) << log2Ceil(v);
}

}



//======================================================================================================================
// MAP IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Node
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <typename K_, typename E_>
Map<K, E, H>::Node::Node(size_t hash, Node * next, K_ && key, E_ && element) :
    hash(hash),
    next(next),
    value(std::forward<K_>(key), std::forward<E_>(element))
{}



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
Map<K, E, H>::Map(size_t minNBuckets) :
    m_size(0),
    m_nBuckets(detail::ceil2(max(minNBuckets, size_t(1)))),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
}

template <typename K, typename E, typename H>
Map<K, E, H>::Map(const Map<K, E, H> & map) :
    m_size(map.m_size),
    m_nBuckets(map.m_nBuckets),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    for (size_t i(0); i < m_nBuckets; ++i) {
        m_buckets[i] = map.m_buckets[i];

        if (m_buckets[i]) {
            m_buckets[i] = new (m_nodeStore + i) Node(*m_buckets[i]);
            Node ** node(&m_buckets[i]->next);
            while (*node) {
                *node = new Node(**node);
                node = &(*node)->next;
            }
        }
    }
}

template <typename K, typename E, typename H>
Map<K, E, H>::Map(Map<K, E, H> && map) :
    m_size(map.m_size),
    m_nBuckets(map.m_nBuckets),
    m_buckets(std::move(map.m_buckets)),
    m_nodeStore(map.m_nodeStore),
    m_rehashing(false)
{
    map.m_size = 0;
    map.m_nBuckets = 0;
    map.m_nodeStore = nullptr;
}

template <typename K, typename E, typename H>
template <typename InputIt>
Map<K, E, H>::Map(InputIt first, InputIt last) :
    m_size(0),
    m_nBuckets(detail::ceil2(std::distance(first, last))),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
    insert(first, last);
}

template <typename K, typename E, typename H>
Map<K, E, H>::Map(std::initializer_list<V> pairs) :
    m_size(0),
    m_nBuckets(detail::ceil2(pairs.size())),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
    insert(pairs);
}



//==============================================================================
// ~Map
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
Map<K, E, H>::~Map() {
    clear();

    if (m_nodeStore) {
        std::free(m_nodeStore);
    }
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
Map<K, E, H> & Map<K, E, H>::operator=(const Map<K, E, H> & map) {
    return *this = std::move(Map<K, E, H>(map));
}

template <typename K, typename E, typename H>
Map<K, E, H> & Map<K, E, H>::operator=(Map<K, E, H> && map) {
    if (&map == this) {
        return *this;
    }

    clear();
    std::free(m_nodeStore);

    m_size = map.m_size;
    m_nBuckets = map.m_nBuckets;
    m_buckets = std::move(map.m_buckets);
    m_nodeStore = map.m_nodeStore;

    map.m_size = 0;
    map.m_nBuckets = 0;
    map.m_nodeStore = nullptr;

    return *this;
}

template <typename K, typename E, typename H>
Map<K, E, H> & Map<K, E, H>::operator=(std::initializer_list<V> values) {
    return *this = std::move(Map<K, E, H>(values));
}



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::swap(Map<K, E, H> & map) {
    std::swap(m_size, map.m_size);
    std::swap(m_nBuckets, map.m_nBuckets);
    std::swap(m_buckets, map.m_buckets);
    std::swap(m_nodeStore, map.m_nodeStore);
}



//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert(const V & value) {
    return insert_h(H()(value.first), value);
}

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert(V && value) {
    size_t hash(H()(value.first));
    return insert_h(hash, std::move(value));
}

template <typename K, typename E, typename H>
template <typename InputIt>
void Map<K, E, H>::insert(InputIt first, InputIt last) {
    while (first != last) {
        insert_h(H()(first->first), *first);
        ++first;
    }
}

template <typename K, typename E, typename H>
void Map<K, E, H>::insert(std::initializer_list<V> values) {
    for (const auto & value : values) {
        insert_h(H()(value.first), value);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert_h(size_t hash, const V & value) {
    return emplace_h(hash, value.first, value.second);
}

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert_h(size_t hash, V && value) {
    return emplace_h(hash, std::move(value.first), std::move(value.second));
}



//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <typename K_, typename E_>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::emplace(K_ && key, E_ && element) {
    size_t hash(H()(key));
    return emplace_h(hash, std::forward<K_>(key), std::forward<E_>(element));
}



//==============================================================================
// emplace_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <typename K_, typename E_>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::emplace_h(size_t hash, K_ && key, E_ && element) {
    size_t bucketI(detBucketI(hash));

    Node ** node(&m_buckets[bucketI]);
    if (*node) {
        while (*node && (*node)->hash < hash) {
            node = &(*node)->next;
        }
        if (*node && (*node)->hash == hash) {
            return { iterator(*this, bucketI, *node), false };
        }
        *node = new Node(hash, *node, std::forward<K_>(key), std::forward<E_>(element));
    }
    else {
        *node = new (m_nodeStore + bucketI) Node(hash, nullptr, std::forward<K_>(key), std::forward<E_>(element));
    }

    ++m_size;
    if (m_size > m_nBuckets) {
        rehash(m_nBuckets * 2);
        return { find_h(hash), true };
    }
    return { iterator(*this, bucketI, *node), true };    
}



//==============================================================================
// at
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
E & Map<K, E, H>::at(const K & key) const {
    return at_h(H()(key));
}



//==============================================================================
// at_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
E & Map<K, E, H>::at_h(size_t hash) const {
    size_t bucketI(detBucketI(hash));

    Node * node(m_buckets[bucketI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return node->value.second;
    }

    throw std::out_of_range("key not found");
}



//==============================================================================
// operator[]
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
E & Map<K, E, H>::operator[](const K & key) {
    return access_h(H()(key), key);
}

template <typename K, typename E, typename H>
E & Map<K, E, H>::operator[](K && key) {
    return access_h(H()(key), std::move(key));
}



//==============================================================================
// access_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
E & Map<K, E, H>::access_h(size_t hash, const K & key) {
    return emplace_h(hash, key, E()).first->second;
}

template <typename K, typename E, typename H>
E & Map<K, E, H>::access_h(size_t hash, K && key) {
    return emplace_h(hash, std::move(key), E()).first->second;
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
bool Map<K, E, H>::erase(const K & key) {
    return erase_h(H()(key));
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::erase(const_iterator position) {
    if (position == cend()) {
        return position;
    }

    iterator next(position); ++next;
    
    return erase_h(position.hash()) ? next : position;
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::erase(const_iterator first, const_iterator last) {
    while (first != last) {
        if (!erase_h((first++).hash())) {
            break;
        }
    }

    return first;
}



//==============================================================================
// erase_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
bool Map<K, E, H>::erase_h(size_t hash) {
    size_t bucketI(detBucketI(hash));

    Node ** node(&m_buckets[bucketI]);
    while (*node && (*node)->hash < hash) {
        node = &(*node)->next;
    }
    if (!*node || (*node)->hash != hash) {
        return false;
    }

    Node * next((*node)->next);
    if (*node < m_nodeStore || *node >= m_nodeStore + m_nBuckets) {
        delete *node;
    }
    *node = next;

    --m_size;
    return true;
}



//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::count(const K & key) const {
    return count_h(H()(key));
}



//==============================================================================
// count_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::count_h(size_t hash) const {
    size_t bucketI(detBucketI(hash));

    Node * node = m_buckets[bucketI];
    while (node && node->hash < hash) {
        node = node->next;
    }
    
    return node && node->hash == hash;
}



//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::rehash(size_t minNBuckets) {
    if (m_rehashing) {
        return;
    }

    Map<K, E, H> map(minNBuckets);
    m_rehashing = true;
    map.m_rehashing = true;

    for (size_t i(0); i < m_nBuckets; ++i) {
        Node * node = m_buckets[i]; 
        while (node) {
            map.emplace_h(node->hash, std::move(node->value.first), std::move(node->value.second));
            node = node->next;
        }
    }

    map.m_rehashing = false;
    m_rehashing = false;
    *this = std::move(map);
}



//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::reserve(size_t nBuckets) {
    if (nBuckets <= m_nBuckets) {
        return;
    }

    rehash(nBuckets);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::clear() {
    Node * storeStart(m_nodeStore), * storeEnd(m_nodeStore + m_nBuckets);
    for (size_t i = 0; i < m_nBuckets; ++i) {
        Node * node(m_buckets[i]), * next;
        while (node) {
            next = node->next;
            if (node < storeStart || node >= storeEnd) delete node;
            node = next;
        }

        m_buckets[i] = nullptr;
    }

    m_size = 0;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
bool Map<K, E, H>::operator==(const Map<K, E, H> & map) const {
    if (&map == this) {
        return true;
    }

    if (m_size != map.m_size) {
        return false;
    }

    const_iterator it1(cbegin()), it2(map.cbegin());
    for (; it1 != cend() && it2 != map.cend(); ++it1, ++it2) {
        if (*it1 != *it2) {
            return false;
        }
    }
    return it1 == cend() && it2 == map.cend();
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
bool Map<K, E, H>::operator!=(const Map<K, E, H> & map) const {
    if (&map == this) {
        return false;
    }

    if (m_size != map.m_size) {
        return true;
    }

    const_iterator it1(cbegin()), it2(map.cbegin());
    for (; it1 != cend() && it2 != map.cend(); ++it1, ++it2) {
        if (*it1 == *it2) {
            return false;
        }
    }
    return it1 != cend() || it2 != map.cend();
}



//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::begin() {
    return iterator(*this);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cbegin() const {
    return const_iterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::end() {
    return iterator(*this, m_nBuckets, nullptr);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cend() const {
    return const_iterator(*this, m_nBuckets, nullptr);
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::find(const K & key) {
    return find_h(H()(key));
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::find(const K & key) const {
    return find_h(H()(key));
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cfind(const K & key) const {
    return cfind_h(H()(key));
}



//==============================================================================
// find_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::find_h(size_t hash) {
    return cfind_h(hash);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::find_h(size_t hash) const {
    return cfind_h(hash);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cfind_h(size_t hash) const {
    size_t bucketI(detBucketI(hash));

    Node * node(m_buckets[bucketI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return const_iterator(*this, bucketI, node);
    }

    return cend();
}



//==============================================================================
// find_e
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::find_e(const E & element) {
    return cfind_e(element);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::find_e(const E & element) const {
    return cfind_e(element);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cfind_e(const E & element) const {
    for (const_iterator it(cbegin()); it != cend(); ++it) {
        if (it->second == element) {
            return it;
        }
    }

    return cend();
}



//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::size() const {
    return m_size;
}



//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
bool Map<K, E, H>::empty() const {
    return m_size == 0;
}

//==============================================================================
// nBuckets
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::nBuckets() const {
    return m_nBuckets;
}

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket_count() const {
    return nBuckets();
}

//==============================================================================
// bucketSize
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucketSize(size_t bucketI) const {
    if (bucketI < 0 || bucketI >= m_nBuckets) {
        return 0;
    }

    size_t size(0);

    for (Node * node(m_buckets[bucketI]); node; node = node->next) {
        ++size;
    }
    
    return size;
}

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket_size(size_t bucketI) const {
    return bucketSize(bucketI);
}

//==============================================================================
// bucket
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket(const K & key) const {
    return detBucketI(H()(key));
}



//------------------------------------------------------------------------------
// Private Methods

template <typename K, typename E, typename H>
inline size_t Map<K, E, H>::detBucketI(size_t hash) const {
    return hash & (m_nBuckets - 1);
}



//======================================================================================================================
// Iterator Implementation /////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
Map<K, E, H>::Iterator<t_const>::Iterator(const Map<K, E, H> & map) :
    m_map(&map),
    m_bucket(0),
    m_node(nullptr)
{
    while (!m_map->m_buckets[m_bucket]) ++m_bucket;
    if (m_bucket < m_map->m_nBuckets) m_node = m_map->m_buckets[m_bucket];
}

template <typename K, typename E, typename H>
template <bool t_const>
Map<K, E, H>::Iterator<t_const>::Iterator(const Map<K, E, H> & map, size_t bucket, typename Map<K, E, H>::Node * node) :
    m_map(&map),
    m_bucket(bucket),
    m_node(node)
{}

template <typename K, typename E, typename H>
template <bool t_const>
template <bool t_const_>
Map<K, E, H>::Iterator<t_const>::Iterator(typename const Map<K, E, H>::Iterator<t_const_> & iterator) :
    m_map(iterator.m_map),
    m_bucket(iterator.m_bucket),
    m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
template <bool t_const_>
typename Map<K, E, H>::Iterator<t_const> & Map<K, E, H>::Iterator<t_const>::operator=(typename const Map<K, E, H>::Iterator<t_const_> & iterator) {
    m_map = iterator.m_map;
    m_bucket = iterator.m_bucket;
    m_node = iterator.m_node;
    return *this;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
typename Map<K, E, H>::Iterator<t_const> & Map<K, E, H>::Iterator<t_const>::operator++() {
    m_node = m_node->next;
    if (!m_node) {
        while (++m_bucket < m_map->m_nBuckets) {
            if (m_node = m_map->m_buckets[m_bucket]) break;
        }
    }
    return *this;
}



//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
typename Map<K, E, H>::Iterator<t_const> Map<K, E, H>::Iterator<t_const>::operator++(int) {
    Map<K, E, H>::Iterator<t_const> temp(*this);
    operator++();
    return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
template <bool t_const_>
bool Map<K, E, H>::Iterator<t_const>::operator==(typename const Map<K, E, H>::Iterator<t_const_> & o) const {
    return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
template <bool t_const_>
bool Map<K, E, H>::Iterator<t_const>::operator!=(typename const Map<K, E, H>::Iterator<t_const_> & o) const {
    return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
typename Map<K, E, H>::Iterator<t_const>::reference Map<K, E, H>::Iterator<t_const>::operator*() const {
    return m_node->value;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
typename Map<K, E, H>::Iterator<t_const>::pointer Map<K, E, H>::Iterator<t_const>::operator->() const {
    return &m_node->value;
}



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
size_t Map<K, E, H>::Iterator<t_const>::hash() const {
    return m_node->hash;
}



//==============================================================================
// key
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
const K & Map<K, E, H>::Iterator<t_const>::key() const {
    return m_node->value.first;
}



//==============================================================================
// element
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <bool t_const>
typename Map<K, E, H>::Iterator<t_const>::IE & Map<K, E, H>::Iterator<t_const>::element() const {
    return m_node->value.second;
}



//======================================================================================================================
// Functions Implementation ////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
inline void swap(Map<K, E, H> & m1, Map<K, E, H> & m2) {
    m1.swap(m2);
}



}