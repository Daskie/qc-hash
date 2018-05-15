namespace qc {



namespace detail {

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
template <typename K_, typename... ElementArgs>
Map<K, E, H>::Node::Node(size_t hash, Node * next, K_ && key, ElementArgs &&... elementArgs) :
    hash(hash),
    next(next),
    value(std::piecewise_construct, std::tuple<K_ &&>(std::forward<K_>(key)), std::tuple<ElementArgs &&...>(std::forward<ElementArgs>(elementArgs)...))
{}



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
Map<K, E, H>::Map(size_t minNSlots) :
    m_size(0),
    m_nSlots(detail::ceil2(max(minNSlots, size_t(1)))),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
}

template <typename K, typename E, typename H>
Map<K, E, H>::Map(const Map<K, E, H> & map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_rehashing(false)
{
    for (size_t i(0); i < m_nSlots; ++i) {
        m_slots[i] = map.m_slots[i];

        if (m_slots[i]) {
            m_slots[i] = new (m_nodeStore + i) Node(*m_slots[i]);
            Node ** node(&m_slots[i]->next);
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
    m_nSlots(map.m_nSlots),
    m_slots(std::move(map.m_slots)),
    m_nodeStore(map.m_nodeStore),
    m_rehashing(false)
{
    map.m_size = 0;
    map.m_nSlots = 0;
    map.m_nodeStore = nullptr;
}

template <typename K, typename E, typename H>
template <typename InputIt>
Map<K, E, H>::Map(InputIt first, InputIt last) :
    m_size(0),
    m_nSlots(detail::ceil2(std::distance(first, last))),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
    insert(first, last);
}

template <typename K, typename E, typename H>
Map<K, E, H>::Map(std::initializer_list<V> pairs) :
    m_size(0),
    m_nSlots(detail::ceil2(pairs.size())),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
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
    m_nSlots = map.m_nSlots;
    m_slots = std::move(map.m_slots);
    m_nodeStore = map.m_nodeStore;

    map.m_size = 0;
    map.m_nSlots = 0;
    map.m_nodeStore = nullptr;

    return *this;
}

template <typename K, typename E, typename H>
Map<K, E, H> & Map<K, E, H>::operator=(std::initializer_list<V> pairs) {
    return *this = std::move(Map<K, E, H>(pairs));
}



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::swap(Map<K, E, H> & map) {
    std::swap(m_size, map.m_size);
    std::swap(m_nSlots, map.m_nSlots);
    std::swap(m_slots, map.m_slots);
    std::swap(m_nodeStore, map.m_nodeStore);
}



//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert(const K & key, const E & element) {
    return insert_h(H()(key), key, element);
}

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert(const V & value) {
    return insert(value.first, value.second);
}

template <typename K, typename E, typename H>
template <typename InputIt>
void Map<K, E, H>::insert(InputIt first, InputIt last) {
    while (first != last) {
        insert_h(H()(first->first), first->first, first->second);
        ++first;
    }
}

template <typename K, typename E, typename H>
void Map<K, E, H>::insert(std::initializer_list<V> pairs) {
    for (const auto & pair : pairs) {
        insert_h(H()(pair.first), pair.first, pair.second);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::insert_h(size_t hash, const K & key, const E & element) {
    return emplace_h(hash, key, element);
}



//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <typename K_, typename... ElementArgs>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::emplace(K_ && key, ElementArgs &&... elementArgs) {
    return emplace_h(H()(key), std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
}



//==============================================================================
// emplace_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
template <typename K_, typename... ElementArgs>
std::pair<typename Map<K, E, H>::iterator, bool> Map<K, E, H>::emplace_h(size_t hash, K_ && key, ElementArgs &&... elementArgs) {
    if (m_size >= m_nSlots) {
        rehash(m_nSlots * 2);
    }

    size_t slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    if (*node) {
        while (*node && (*node)->hash < hash) {
            node = &(*node)->next;
        }
        if (*node && (*node)->hash == hash) {
            return { iterator(*this, slotI, *node), false };
        }
        *node = new Node(hash, *node, std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
    }
    else {
        *node = new (m_nodeStore + slotI) Node(hash, nullptr, std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
    }

    ++m_size;
    return { iterator(*this, slotI, *node), true };
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
    size_t slotI(detSlotI(hash));

    Node * node(m_slots[slotI]);
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



//==============================================================================
// access_h
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
E & Map<K, E, H>::access_h(size_t hash, const K & key) {
    return emplace_h(hash, key).first->second;
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
    size_t slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    while (*node && (*node)->hash < hash) {
        node = &(*node)->next;
    }
    if (!*node || (*node)->hash != hash) {
        return false;
    }

    Node * next((*node)->next);
    if (*node < m_nodeStore || *node >= m_nodeStore + m_nSlots) {
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
    size_t slotI(detSlotI(hash));

    Node * node = m_slots[slotI];
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return 1;
    }

    return 0;
}



//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::rehash(size_t minNSlots) {
    if (m_rehashing) {
        return;
    }

    Map<K, E, H> map(minNSlots);
    m_rehashing = true;
    map.m_rehashing = true;

    for (size_t i(0); i < m_nSlots; ++i) {
        Node * node = m_slots[i]; 
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
void Map<K, E, H>::reserve(size_t nSlots) {
    if (nSlots <= m_nSlots) {
        return;
    }

    rehash(nSlots);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
void Map<K, E, H>::clear() {
    Node * storeStart(m_nodeStore), * storeEnd(m_nodeStore + m_nSlots);
    for (size_t i = 0; i < m_nSlots; ++i) {
        Node * node(m_slots[i]), * next;
        while (node) {
            next = node->next;
            if (node < storeStart || node >= storeEnd) delete node;
            node = next;
        }

        m_slots[i] = nullptr;
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
    return iterator(*this, m_nSlots, nullptr);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cend() const {
    return const_iterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
typename Map<K, E, H>::iterator Map<K, E, H>::find(const K & key) {
    return cfind(key);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::find(const K & key) const {
    return cfind(key);
}

template <typename K, typename E, typename H>
typename Map<K, E, H>::const_iterator Map<K, E, H>::cfind(const K & key) const {
    return find_h(H()(key));
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
    size_t slotI(detSlotI(hash));

    Node * node(m_slots[slotI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return const_iterator(*this, slotI, node);
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
// nSlots
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::nSlots() const {
    return m_nSlots;
}

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket_count() const {
    return nSlots();
}

//==============================================================================
// slotSize
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::slotSize(size_t slotI) const {
    if (slotI < 0 || slotI >= m_nSlots) {
        return 0;
    }

    size_t size(0);

    for (Node * node(m_slots[slotI]); node; node = node->next) {
        ++size;
    }
    
    return size;
}

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket_size(size_t slotI) const {
    return slotSize(slotI);
}

//==============================================================================
// slot
//------------------------------------------------------------------------------

template <typename K, typename E, typename H>
size_t Map<K, E, H>::slot(const K & key) const {
    return detSlotI(H()(key));
}

template <typename K, typename E, typename H>
size_t Map<K, E, H>::bucket(const K & key) const {
    return slot(key);
}



//------------------------------------------------------------------------------
// Private Methods

template <typename K, typename E, typename H>
inline size_t Map<K, E, H>::detSlotI(size_t hash) const {
    return hash & (m_nSlots - 1);
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
    m_slot(0),
    m_node(nullptr)
{
    while (!m_map->m_slots[m_slot]) ++m_slot;
    if (m_slot < m_map->m_nSlots) m_node = m_map->m_slots[m_slot];
}

template <typename K, typename E, typename H>
template <bool t_const>
Map<K, E, H>::Iterator<t_const>::Iterator(const Map<K, E, H> & map, size_t slot, typename Map<K, E, H>::Node * node) :
    m_map(&map),
    m_slot(slot),
    m_node(node)
{}

template <typename K, typename E, typename H>
template <bool t_const>
template <bool t_const_>
Map<K, E, H>::Iterator<t_const>::Iterator(typename const Map<K, E, H>::Iterator<t_const_> & iterator) :
    m_map(iterator.m_map),
    m_slot(iterator.m_slot),
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
    m_slot = iterator.m_slot;
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
        while (++m_slot < m_map->m_nSlots) {
            if (m_node = m_map->m_slots[m_slot]) break;
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