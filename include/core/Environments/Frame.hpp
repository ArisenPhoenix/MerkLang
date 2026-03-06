#pragma once

#include "core/TypesFWD.hpp"
#include "core/node/Node.hpp"
#include <unordered_map>

// Lightweight call-frame index for fast local variable access.
// Owned locals are stored in FrameCell and only materialized to VarNode on demand.
class Frame {
public:
    enum class SlotKind {
        Unknown,
        Int,
        Double,
        Bool,
        Object
    };

    static SlotKind classify(NodeValueType type) {
        if (type == NodeValueType::Int) return SlotKind::Int;
        if (type == NodeValueType::Double || type == NodeValueType::Float) return SlotKind::Double;
        if (type == NodeValueType::Bool) return SlotKind::Bool;
        if (type == NodeValueType::UNKNOWN || type == NodeValueType::Uninitialized) return SlotKind::Unknown;
        return SlotKind::Object;
    }

    struct FrameCell {
        DataTypeFlags flags;
        SlotKind kind = SlotKind::Unknown;
        int intValue = 0;
        double doubleValue = 0.0;
        bool boolValue = false;
        Node objectValue;
        mutable UniquePtr<VarNode> materialized;

        void setFromNode(const Node& value) {
            kind = classify(value.getType());
            switch (kind) {
                case SlotKind::Int: intValue = value.toInt(); break;
                case SlotKind::Double: doubleValue = value.toDouble(); break;
                case SlotKind::Bool: boolValue = value.toBool(); break;
                case SlotKind::Object: objectValue = value; break;
                case SlotKind::Unknown: default: objectValue = value; break;
            }
            if (materialized) {
                materialized->setValue(toNode());
            }
        }

        Node toNode() const {
            switch (kind) {
                case SlotKind::Int: return Node(intValue);
                case SlotKind::Double: return Node(doubleValue);
                case SlotKind::Bool: return Node(boolValue);
                case SlotKind::Object: return objectValue;
                case SlotKind::Unknown: default: return objectValue;
            }
        }

        VarNode& materialize(const String& name) const {
            Node value = toNode();
            if (!materialized) {
                DataTypeFlags declFlags = flags;
                if (declFlags.name.empty()) {
                    declFlags.name = name;
                }
                materialized = makeUnique<VarNode>(value, declFlags);
            } else {
                if (value.isNull()) {
                    DataTypeFlags declFlags = flags;
                    if (declFlags.name.empty()) {
                        declFlags.name = name;
                    }
                    materialized = makeUnique<VarNode>(value, declFlags);
                } else {
                    materialized->setValue(value);
                }
            }
            return *materialized;
        }
    };

    void clear() {
        borrowedSlots.clear();
        ownedSlots.clear();
        slotKinds.clear();
        slotByName.clear();
    }

    bool hasSlot(const String& name) const {
        return slotByName.find(name) != slotByName.end();
    }

    VarNode* getSlot(const String& name) const {
        auto borrowed = borrowedSlots.find(name);
        if (borrowed != borrowedSlots.end()) {
            return borrowed->second;
        }
        auto owned = ownedSlots.find(name);
        if (owned != ownedSlots.end()) {
            return &owned->second.materialize(name);
        }
        return nullptr;
    }

    size_t bindSlot(const String& name, VarNode* var, NodeValueType valueType = NodeValueType::UNKNOWN) {
        borrowedSlots[name] = var;
        auto it = slotByName.find(name);
        const SlotKind kind = classify(valueType);
        if (it != slotByName.end()) {
            const size_t idx = it->second;
            if (idx < slotKinds.size()) {
                slotKinds[idx] = kind;
                return idx;
            }
        }
        const size_t idx = slotKinds.size();
        slotKinds.push_back(kind);
        slotByName[name] = idx;
        return idx;
    }

    bool hasOwned(const String& name) const {
        return ownedSlots.find(name) != ownedSlots.end();
    }

    VarNode* getOwned(const String& name) const {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return nullptr;
        return &it->second.materialize(name);
    }

    DataTypeFlags* getOwnedFlags(const String& name) {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return nullptr;
        return &it->second.flags;
    }

    const DataTypeFlags* getOwnedFlags(const String& name) const {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return nullptr;
        return &it->second.flags;
    }

    bool updateOwnedValue(const String& name, const Node& value) {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return false;
        it->second.setFromNode(value);
        setSlotKind(name, value.getType());
        return true;
    }

    bool updateOwnedInt(const String& name, int value) {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return false;
        it->second.kind = SlotKind::Int;
        it->second.intValue = value;
        if (it->second.materialized) {
            it->second.materialized->setValue(Node(value));
        }
        setSlotKind(name, NodeValueType::Int);
        return true;
    }

    FrameCell* getOwnedCell(const String& name) {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return nullptr;
        return &it->second;
    }

    const FrameCell* getOwnedCell(const String& name) const {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) return nullptr;
        return &it->second;
    }

    FrameCell* ensureOwnedIntCell(const String& name, int initialValue = 0, const DataTypeFlags* initialFlags = nullptr) {
        auto it = ownedSlots.find(name);
        if (it == ownedSlots.end()) {
            FrameCell cell;
            cell.kind = SlotKind::Int;
            cell.intValue = initialValue;
            if (initialFlags) {
                cell.flags = *initialFlags;
            }
            if (cell.flags.name.empty()) {
                cell.flags.name = name;
            }
            auto [newIt, _inserted] = ownedSlots.emplace(name, std::move(cell));
            it = newIt;
        } else if (it->second.kind != SlotKind::Int) {
            it->second.kind = SlotKind::Int;
            it->second.intValue = initialValue;
            if (it->second.materialized) {
                it->second.materialized->setValue(Node(initialValue));
            }
        }

        auto slotIt = slotByName.find(name);
        if (slotIt == slotByName.end()) {
            slotByName[name] = slotKinds.size();
            slotKinds.push_back(SlotKind::Int);
        } else if (slotIt->second < slotKinds.size()) {
            slotKinds[slotIt->second] = SlotKind::Int;
        }

        borrowedSlots.erase(name);
        return &it->second;
    }

    VarNode* insertOwned(const String& name, UniquePtr<VarNode> value) {
        if (!value) return nullptr;
        FrameCell cell;
        cell.flags = value->getVarFlags();
        cell.setFromNode(value->getValueNode());
        auto [it, inserted] = ownedSlots.emplace(name, std::move(cell));
        if (!inserted) return nullptr;

        const NodeValueType type = it->second.toNode().getType();
        auto slotIt = slotByName.find(name);
        if (slotIt == slotByName.end()) {
            slotByName[name] = slotKinds.size();
            slotKinds.push_back(classify(type));
        } else if (slotIt->second < slotKinds.size()) {
            slotKinds[slotIt->second] = classify(type);
        }

        borrowedSlots.erase(name);
        return &it->second.materialize(name);
    }

    VarNode* assignOwned(const String& name, UniquePtr<VarNode> value) {
        if (!value) return nullptr;
        FrameCell cell;
        cell.flags = value->getVarFlags();
        cell.setFromNode(value->getValueNode());
        ownedSlots[name] = std::move(cell);

        const NodeValueType type = ownedSlots[name].toNode().getType();
        auto slotIt = slotByName.find(name);
        if (slotIt == slotByName.end()) {
            slotByName[name] = slotKinds.size();
            slotKinds.push_back(classify(type));
        } else if (slotIt->second < slotKinds.size()) {
            slotKinds[slotIt->second] = classify(type);
        }

        borrowedSlots.erase(name);
        return &ownedSlots[name].materialize(name);
    }

    void setSlotKind(const String& name, NodeValueType valueType) {
        auto it = slotByName.find(name);
        if (it == slotByName.end()) return;
        const size_t idx = it->second;
        if (idx >= slotKinds.size()) return;
        slotKinds[idx] = classify(valueType);
    }

    SlotKind getSlotKind(const String& name) const {
        auto it = slotByName.find(name);
        if (it == slotByName.end()) return SlotKind::Unknown;
        const size_t idx = it->second;
        if (idx >= slotKinds.size()) return SlotKind::Unknown;
        return slotKinds[idx];
    }

    bool tryGetInt(const String& name, int& out) const {
        auto owned = ownedSlots.find(name);
        if (owned != ownedSlots.end()) {
            if (owned->second.kind != SlotKind::Int) return false;
            out = owned->second.intValue;
            return true;
        }

        auto borrowed = borrowedSlots.find(name);
        if (borrowed == borrowedSlots.end() || borrowed->second == nullptr) return false;
        const Node& n = borrowed->second->getValueNode();
        if (!n.isInt()) return false;
        out = n.toInt();
        return true;
    }

    size_t slotCount() const { return slotByName.size(); }
    size_t ownedCount() const { return ownedSlots.size(); }

private:
    std::unordered_map<String, VarNode*> borrowedSlots;
    std::unordered_map<String, FrameCell> ownedSlots;
    std::unordered_map<String, size_t> slotByName;
    Vector<SlotKind> slotKinds;
};
