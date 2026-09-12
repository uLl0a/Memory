#pragma once

#include <Psapi.h>
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#pragma comment(lib, "Psapi.lib")

class Memory {
   public:
    class MemoryHandle {
       public:
        constexpr MemoryHandle(void* ptr = nullptr) : m_Ptr(ptr) {}

        explicit MemoryHandle(std::uintptr_t ptr)
            : m_Ptr(reinterpret_cast<void*>(ptr)) {}

        template <typename T>
        constexpr std::enable_if_t<std::is_pointer_v<T>, T> As() const {
            return static_cast<T>(m_Ptr);
        }

        template <typename T>
        constexpr std::enable_if_t<std::is_lvalue_reference_v<T>, T> As()
            const {
            return *static_cast<
                std::add_pointer_t<std::remove_reference_t<T> > >(m_Ptr);
        }

        template <typename T>
        constexpr std::enable_if_t<std::is_same_v<T, std::uintptr_t>, T> As()
            const {
            return reinterpret_cast<T>(m_Ptr);
        }

        template <typename T>
        constexpr MemoryHandle Add(T offset) const {
            return MemoryHandle(As<std::uintptr_t>() +
                                static_cast<std::uintptr_t>(offset));
        }

        template <typename T>
        constexpr MemoryHandle Sub(T offset) const {
            return MemoryHandle(As<std::uintptr_t>() -
                                static_cast<std::uintptr_t>(offset));
        }

        constexpr MemoryHandle Rip() const {
            if (!m_Ptr)
                return {};

            return Add(As<std::int32_t&>()).Add(4U);
        }

        constexpr explicit operator bool() const noexcept {
            return m_Ptr != nullptr;
        }

       private:
        void* m_Ptr;
    };

    class MemoryRegion {
       public:
        constexpr MemoryRegion(MemoryHandle base, std::size_t size)
            : m_Base(base), m_Size(size) {}

        constexpr MemoryHandle Base() const { return m_Base; }

        constexpr MemoryHandle End() const { return m_Base.Add(m_Size); }

        constexpr std::size_t Size() const { return m_Size; }

        constexpr bool Contains(MemoryHandle ptr) const {
            const auto address = ptr.As<std::uintptr_t>();

            const auto begin = m_Base.As<std::uintptr_t>();

            const auto end = End().As<std::uintptr_t>();

            return address >= begin && address <= end;
        }

       private:
        MemoryHandle m_Base;
        std::size_t m_Size;
    };

    struct Element {
        std::uint8_t Data{};
        bool WildCard{};
    };

    class Signature {
       public:
        MemoryHandle Scan(const MemoryRegion& region) const;

        void Add(std::uint8_t data, bool wildcard = false) {
            elements_.push_back({data, wildcard});
        }

        std::size_t Size() const { return elements_.size(); }

       private:
        std::vector<Element> elements_;
    };

   public:
    static MODULEINFO GetModuleInfo(const char* module);

    static void WriteMemory(std::uintptr_t baseAddress,
                            int value,
                            std::uintptr_t offset1,
                            std::uintptr_t offset2);

    static std::uintptr_t FindPattern(const char* module,
                                      const char* pattern,
                                      const char* mask);

    static bool Patch(BYTE* address, const void* value, std::size_t bytesNum);

    static const char* ReadText(BYTE* address);

    static void* DetourApply(BYTE* original, BYTE* hook, std::size_t length);

    static bool DetourRemove(BYTE* original,
                             const BYTE* originalBytes,
                             std::size_t length);

    static bool bCompare(const BYTE* data,
                         const BYTE* mask,
                         const char* szMask);
};
