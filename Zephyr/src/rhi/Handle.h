#pragma once
#include "pch.h"

namespace Zephyr
{
    using HandleID = uint32_t;
    struct HandleBase
    {
        HandleBase(HandleID id) : id(id) {}

        static constexpr const HandleID InvalidHandleID = UINT32_MAX;

        inline bool IsValid() { return id != InvalidHandleID; }

        constexpr HandleBase() : id(InvalidHandleID) {}

        bool operator==(const HandleBase& rhs) const { return rhs.id == id; }

        inline HandleID GetID() { return id; }

        HandleID id;
    };

    template<typename T>
    struct Handle : HandleBase
    {
        constexpr Handle() : HandleBase() {}
        Handle(HandleID id) : HandleBase(id) {}

        bool operator==(const Handle<T>& rhs) const { return rhs.id == id; }
    };

    template<typename T>
    struct hash_fn_handle
    {
        size_t operator()(const Handle<T>& t) const { return std::hash<uint32_t>()(t.id); }
    };
} // namespace Zephyr