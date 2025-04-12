// #ifndef ITERABLE_H
// #define ITERABLE_H

// #include "core/types.h"
// #include "core/node.h"
// #include <unordered_map>
// #include <vector>
// // Base Iterable Interface
// class Iterable {
//     public:
//         virtual ~Iterable() = default;
    
//         virtual bool hasNext() const = 0;
//         virtual Node next() = 0;
//         virtual void reset() = 0; // Resets iterator to beginning

//         virtual Node get() = 0;
//         virtual void insert(const Node& value) = 0;
//     };
    
//     // Iterator for Vectors
//     class VectorIterator : public Iterable {
//     protected:
//         Vector<Node> data;
//         size_t index;
    
//     public:
//         explicit VectorIterator(Vector<Node> data = {});
    
//         bool hasNext() const override;
//         Node next() override;
//         void reset() override;
//         // void addValue(const Node& value);
//         void insert(const Node& value);
//     };
    
//     // Iterator for Maps (key-value structures)
//     class MapIterator : public Iterable {
//     protected:
//         std::unordered_map<Node, Node> data;
//         std::unordered_map<Node, Node>::iterator it;
    
//     public:
//         explicit MapIterator(std::unordered_map<Node, Node> data = {});
    
//         bool hasNext() const override;
//         Node next() override;
//         void reset() override;
//         void insert(const Node& key, const Node& value);
//     };

// #endif // ITERABLE_H
