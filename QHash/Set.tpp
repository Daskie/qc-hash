namespace qc {



//======================================================================================================================
// SET IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Node
//------------------------------------------------------------------------------

template <typename V, typename H>
template <typename V_>
Set<V, H>::Node::Node(size_t hash, Node * next, V_ && value) :
    hash(hash),
    next(next),
    value(std::forward<K_>(value))
{}



//==============================================================================
// Set
//------------------------------------------------------------------------------

template <typename V, typename H>
Set<V, H>::Set(size_t minNBuckets) :
    m_size(0),
    m_nBuckets(detail::ceil2(minNBuckets >= 1 ? minNBuckets : 1)),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
}

template <typename V, typename H>
Set<V, H>::Set(const Set<V, H> & set) :
    m_size(set.m_size),
    m_nBuckets(set.m_nBuckets),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    for (size_t i(0); i < m_nBuckets; ++i) {
        m_buckets[i] = set.m_buckets[i];

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

template <typename V, typename H>
Set<V, H>::Set(Set<V, H> && set) :
    m_size(set.m_size),
    m_nBuckets(set.m_nBuckets),
    m_buckets(std::move(set.m_buckets)),
    m_nodeStore(set.m_nodeStore),
    m_rehashing(false)
{
    set.m_size = 0;
    set.m_nBuckets = 0;
    set.m_nodeStore = nullptr;
}

template <typename V, typename H>
template <typename InputIt>
Set<V, H>::Set(InputIt first, InputIt last) :
    m_size(0),
    m_nBuckets(detail::ceil2(std::distance(first, last))),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
    insert(first, last);
}

template <typename V, typename H>
Set<V, H>::Set(std::initializer_list<V> values) :
    m_size(0),
    m_nBuckets(detail::ceil2(values.size())),
    m_buckets(new Node *[m_nBuckets]),
    m_nodeStore((Node *)std::malloc(m_nBuckets * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_buckets.get(), 0, m_nBuckets * sizeof(Node *));
    insert(values);
}



//==============================================================================
// ~Set
//------------------------------------------------------------------------------

template <typename V, typename H>
Set<V, H>::~Set() {
    clear();

    if (m_nodeStore) {
        std::free(m_nodeStore);
    }
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename V, typename H>
Set<V, H> & Set<V, H>::operator=(const Set<V, H> & set) {
    return *this = std::move(Set<V, H>(set));
}

template <typename V, typename H>
Set<V, H> & Set<V, H>::operator=(Set<V, H> && set) {
    if (&set == this) {
        return *this;
    }

    clear();
    std::free(m_nodeStore);

    m_size = set.m_size;
    m_nBuckets = set.m_nBuckets;
    m_buckets = std::move(set.m_buckets);
    m_nodeStore = set.m_nodeStore;

    set.m_size = 0;
    set.m_nBuckets = 0;
    set.m_nodeStore = nullptr;

    return *this;
}

template <typename V, typename H>
Set<V, H> & Set<V, H>::operator=(std::initializer_list<V> values) {
    return *this = std::move(Set<V, H>(values));
}



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H>
void Set<V, H>::swap(Set<V, H> & set) {
    std::swap(m_size, set.m_size);
    std::swap(m_nBuckets, set.m_nBuckets);
    std::swap(m_buckets, set.m_buckets);
    std::swap(m_nodeStore, set.m_nodeStore);
}



//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename V, typename H>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::insert(const V & value) {
    return insert_h(H()(value), value);
}

template <typename V, typename H>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::insert(V && value) {
    size_t hash(H()(value));
    return insert_h(hash, std::move(value));
}

template <typename V, typename H>
template <typename InputIt>
void Set<V, H>::insert(InputIt first, InputIt last) {
    while (first != last) {
        insert_h(H()(first->first), *first);
        ++first;
    }
}

template <typename V, typename H>
void Set<V, H>::insert(std::initializer_list<V> values) {
    for (const auto & value : values) {
        insert_h(H()(value), value);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename V, typename H>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::insert_h(size_t hash, const V & value) {
    return emplace_h(hash, value);
}

template <typename V, typename H>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::insert_h(size_t hash, V && value) {
    return emplace_h(hash, std::move(value));
}



//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename V, typename H>
template <typename V_>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::emplace(V_ && value) {
    size_t hash(H()(value));
    return emplace_h(hash, std::forward<V_>(value));
}



//==============================================================================
// emplace_h
//------------------------------------------------------------------------------

template <typename V, typename H>
template <typename V_>
std::pair<typename Set<V, H>::iterator, bool> Set<V, H>::emplace_h(size_t hash, V_ && value) {
    size_t bucketI(detBucketI(hash));

    Node ** node(&m_buckets[bucketI]);
    if (*node) {
        while (*node && (*node)->hash < hash) {
            node = &(*node)->next;
        }
        if (*node && (*node)->hash == hash) {
            return { iterator(*this, bucketI, *node), false };
        }
        *node = new Node(hash, *node, std::forward<V_>(value));
    }
    else {
        *node = new (m_nodeStore + bucketI) Node(hash, nullptr, std::forward<V_>(value));
    }

    ++m_size;
    if (m_size > m_nBuckets) {
        rehash(m_nBuckets * 2);
        return { find_h(hash), true };
    }
    return { iterator(*this, bucketI, *node), true };    
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename V, typename H>
bool Set<V, H>::erase(const V & value) {
    return erase_h(H()(value));
}

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::erase(const_iterator position) {
    if (position == cend()) {
        return position;
    }

    iterator next(position); ++next;
    
    return erase_h(position.hash()) ? next : position;
}

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::erase(const_iterator first, const_iterator last) {
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

template <typename V, typename H>
bool Set<V, H>::erase_h(size_t hash) {
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

template <typename V, typename H>
size_t Set<V, H>::count(const V & value) const {
    return count_h(H()(value));
}



//==============================================================================
// count_h
//------------------------------------------------------------------------------

template <typename V, typename H>
size_t Set<V, H>::count_h(size_t hash) const {
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

template <typename V, typename H>
void Set<V, H>::rehash(size_t minNBuckets) {
    if (m_rehashing) {
        return;
    }

    Set<V, H> set(minNBuckets);
    m_rehashing = true;
    set.m_rehashing = true;

    for (size_t i(0); i < m_nBuckets; ++i) {
        Node * node = m_buckets[i]; 
        while (node) {
            set.emplace_h(node->hash, std::move(node->value));
            node = node->next;
        }
    }

    set.m_rehashing = false;
    m_rehashing = false;
    *this = std::move(set);
}



//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename V, typename H>
void Set<V, H>::reserve(size_t nBuckets) {
    if (nBuckets <= m_nBuckets) {
        return;
    }

    rehash(nBuckets);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename V, typename H>
void Set<V, H>::clear() {
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

template <typename V, typename H>
bool Set<V, H>::operator==(const Set<V, H> & set) const {
    if (&set == this) {
        return true;
    }

    if (m_size != set.m_size) {
        return false;
    }

    const_iterator it1(cbegin()), it2(set.cbegin());
    for (; it1 != cend() && it2 != set.cend(); ++it1, ++it2) {
        if (*it1 != *it2) {
            return false;
        }
    }
    return it1 == cend() && it2 == set.cend();
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H>
bool Set<V, H>::operator!=(const Set<V, H> & set) const {
    return !(*this == set);
}



//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::begin() {
    return iterator(*this);
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::cbegin() const {
    return const_iterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::end() {
    return iterator(*this, m_nBuckets, nullptr);
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::cend() const {
    return const_iterator(*this, m_nBuckets, nullptr);
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::find(const V & value) {
    return find_h(H()(value));
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::find(const V & value) const {
    return find_h(H()(value));
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::cfind(const V & value) const {
    return cfind_h(H()(value));
}



//==============================================================================
// find_h
//------------------------------------------------------------------------------

template <typename V, typename H>
typename Set<V, H>::iterator Set<V, H>::find_h(size_t hash) {
    return cfind_h(hash);
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::find_h(size_t hash) const {
    return cfind_h(hash);
}

template <typename V, typename H>
typename Set<V, H>::const_iterator Set<V, H>::cfind_h(size_t hash) const {
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
// size
//------------------------------------------------------------------------------

template <typename V, typename H>
size_t Set<V, H>::size() const {
    return m_size;
}



//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename V, typename H>
bool Set<V, H>::empty() const {
    return m_size == 0;
}

//==============================================================================
// bucket_count
//------------------------------------------------------------------------------

template <typename V, typename H>
size_t Set<V, H>::bucket_count() const {
    return m_nBuckets;
}

//==============================================================================
// bucket_size
//------------------------------------------------------------------------------

template <typename V, typename H>
size_t Set<V, H>::bucket_size(size_t bucketI) const {
    if (bucketI < 0 || bucketI >= m_nBuckets) {
        return 0;
    }

    size_t size(0);

    for (Node * node(m_buckets[bucketI]); node; node = node->next) {
        ++size;
    }
    
    return size;
}

//==============================================================================
// bucket
//------------------------------------------------------------------------------

template <typename V, typename H>
size_t Set<V, H>::bucket(const V & value) const {
    return detBucketI(H()(value));
}



//------------------------------------------------------------------------------
// Private Methods

template <typename V, typename H>
inline size_t Set<V, H>::detBucketI(size_t hash) const {
    return hash & (m_nBuckets - 1);
}



//======================================================================================================================
// Iterator Implementation /////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
Set<V, H>::Iterator<t_const>::Iterator(const Set<V, H> & set) :
    m_set(&set),
    m_bucket(0),
    m_node(nullptr)
{
    while (!m_set->m_buckets[m_bucket]) ++m_bucket;
    if (m_bucket < m_set->m_nBuckets) m_node = m_set->m_buckets[m_bucket];
}

template <typename V, typename H>
template <bool t_const>
Set<V, H>::Iterator<t_const>::Iterator(const Set<V, H> & set, size_t bucket, typename Set<V, H>::Node * node) :
    m_set(&set),
    m_bucket(bucket),
    m_node(node)
{}

template <typename V, typename H>
template <bool t_const>
template <bool t_const_>
Set<V, H>::Iterator<t_const>::Iterator(typename const Set<V, H>::Iterator<t_const_> & iterator) :
    m_set(iterator.m_set),
    m_bucket(iterator.m_bucket),
    m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
template <bool t_const_>
typename Set<V, H>::Iterator<t_const> & Set<V, H>::Iterator<t_const>::operator=(typename const Set<V, H>::Iterator<t_const_> & iterator) {
    m_set = iterator.m_set;
    m_bucket = iterator.m_bucket;
    m_node = iterator.m_node;
    return *this;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
typename Set<V, H>::Iterator<t_const> & Set<V, H>::Iterator<t_const>::operator++() {
    m_node = m_node->next;
    if (!m_node) {
        while (++m_bucket < m_set->m_nBuckets) {
            if (m_node = m_set->m_buckets[m_bucket]) break;
        }
    }
    return *this;
}



//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
typename Set<V, H>::Iterator<t_const> Set<V, H>::Iterator<t_const>::operator++(int) {
    Set<V, H>::Iterator<t_const> temp(*this);
    operator++();
    return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
template <bool t_const_>
bool Set<V, H>::Iterator<t_const>::operator==(typename const Set<V, H>::Iterator<t_const_> & o) const {
    return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
template <bool t_const_>
bool Set<V, H>::Iterator<t_const>::operator!=(typename const Set<V, H>::Iterator<t_const_> & o) const {
    return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
typename Set<V, H>::Iterator<t_const>::reference Set<V, H>::Iterator<t_const>::operator*() const {
    return m_node->value;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
typename Set<V, H>::Iterator<t_const>::pointer Set<V, H>::Iterator<t_const>::operator->() const {
    return &m_node->value;
}



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <typename V, typename H>
template <bool t_const>
size_t Set<V, H>::Iterator<t_const>::hash() const {
    return m_node->hash;
}



//======================================================================================================================
// Functions Implementation ////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename V, typename H>
inline void swap(Set<V, H> & m1, Set<V, H> & m2) {
    m1.swap(m2);
}



}