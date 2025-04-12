// #ifndef ENUM_STRUCTURE_H
// #define ENUM_STRUCTURE_H

// #include "core/types.h"
// #include "core/node.h"
// #include "helpers/iterable.h"
// #include "ast/ast_structures.h"


// // ========== VectorIterator Implementation ==========

// VectorIterator::VectorIterator(Vector<Node> data) : data(std::move(data)), index(0) {}

// bool VectorIterator::hasNext() const {
//     return index < data.size();
// }

// Node VectorIterator::next() {
//     if (!hasNext()) {
//         throw std::out_of_range("VectorIterator reached end");
//     }
//     return data[index++];
// }

// void VectorIterator::reset() {
//     index = 0;
// }

// void VectorIterator::insert(const Node& value) {
//     data.push_back(value);
// }

// // ========== MapIterator Implementation ==========

// MapIterator::MapIterator(std::unordered_map<Node, Node> data)
//     : data(std::move(data)), it(this->data.begin()) {}

// bool MapIterator::hasNext() const {
//     return it != data.end();
// }

// Node MapIterator::next() {
//     if (!hasNext()) {
//         throw std::out_of_range("MapIterator reached end");
//     }
//     Node key = it->first;
//     ++it;
//     return key; // Returning key for iteration
// }

// void MapIterator::reset() {
//     it = data.begin();
// }

// void MapIterator::insert(const Node& key, const Node& value) {
//     data[key] = value;
// }

// #endif // ENUM_STRUCTURE_H
