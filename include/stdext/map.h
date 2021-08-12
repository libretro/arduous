#ifndef STDEXT_MAP_H
#define STDEXT_MAP_H

template <class Map, typename Key = typename Map::key_type, typename Value = typename Map::mapped_type>
typename Map::mapped_type get_default(const Map& map, const Key& key, Value&& dflt) {
    using M = typename Map::mapped_type;
    auto pos = map.find(key);
    return (pos != map.end()) ? pos->second : static_cast<M>(static_cast<Value&&>(dflt));
}

#endif
