#include "Memory.h"

#include <cstdlib>
#include <cstring>

MODULEINFO Memory::GetModuleInfo(const char* module) {
    MODULEINFO info{};

    HMODULE hModule = GetModuleHandleA(module);

    if (!hModule)
        return info;

    GetModuleInformation(GetCurrentProcess(), hModule, &info, sizeof(info));

    return info;
}

void Memory::WriteMemory(std::uintptr_t baseAddress,
                         int value,
                         std::uintptr_t offset1,
                         std::uintptr_t offset2) {
    auto address = *reinterpret_cast<std::uintptr_t*>(baseAddress + offset1);

    address += offset2;

    *reinterpret_cast<int*>(address) = value;
}

std::uintptr_t Memory::FindPattern(const char* module,
                                   const char* pattern,
                                   const char* mask) {
    MODULEINFO moduleInfo = Memory::GetModuleInfo(module);

    if (!moduleInfo.lpBaseOfDll || !moduleInfo.SizeOfImage || !pattern ||
        !mask) {
        return 0;
    }

    const auto base = reinterpret_cast<std::uintptr_t>(moduleInfo.lpBaseOfDll);

    const auto size = static_cast<std::size_t>(moduleInfo.SizeOfImage);

    const auto patternLength = std::strlen(mask);

    if (patternLength == 0 || patternLength > size) {
        return 0;
    }

    for (std::size_t i = 0; i <= size - patternLength; ++i) {
        bool found = true;

        for (std::size_t j = 0; j < patternLength; ++j) {
            if (mask[j] != '?' && static_cast<BYTE>(pattern[j]) !=
                                      *reinterpret_cast<BYTE*>(base + i + j)) {
                found = false;
                break;
            }
        }

        if (found)
            return base + i;
    }

    return 0;
}

bool Memory::Patch(BYTE* address, const void* value, std::size_t bytesNum) {
    if (!address || !value || bytesNum == 0) {
        return false;
    }

    DWORD oldProtection = 0;

    if (!VirtualProtect(address, bytesNum, PAGE_EXECUTE_READWRITE,
                        &oldProtection)) {
        return false;
    }

    std::memcpy(address, value, bytesNum);

    FlushInstructionCache(GetCurrentProcess(), address, bytesNum);

    DWORD temp = 0;

    VirtualProtect(address, bytesNum, oldProtection, &temp);

    return true;
}

const char* Memory::ReadText(BYTE* address) {
    return reinterpret_cast<const char*>(address);
}

void* Memory::DetourApply(BYTE* original, BYTE* hook, std::size_t length) {
    if (!original || !hook || length < 5) {
        return nullptr;
    }

    DWORD oldProtection = 0;

    if (!VirtualProtect(original, length, PAGE_EXECUTE_READWRITE,
                        &oldProtection)) {
        return nullptr;
    }

    BYTE* trampoline = static_cast<BYTE*>(std::malloc(length + 5));

    if (!trampoline) {
        DWORD temp = 0;

        VirtualProtect(original, length, oldProtection, &temp);

        return nullptr;
    }

    /*
     * Copiar instrucciones originales
     * al trampoline.
     */
    std::memcpy(trampoline, original, length);

    /*
     * JMP trampoline -> original + length
     *
     * E9 [relative address]
     */
    trampoline[length] = 0xE9;

    const auto returnAddress =
        reinterpret_cast<std::uintptr_t>(original + length);

    const auto trampolineJump =
        reinterpret_cast<std::uintptr_t>(trampoline + length + 5);

    *reinterpret_cast<std::int32_t*>(trampoline + length + 1) =
        static_cast<std::int32_t>(returnAddress - trampolineJump);

    /*
     * NOP de las instrucciones
     * que vamos a reemplazar.
     */
    std::memset(original, 0x90, length);

    /*
     * JMP original -> hook
     *
     * E9 [relative address]
     */
    original[0] = 0xE9;

    const auto hookAddress = reinterpret_cast<std::uintptr_t>(hook);

    const auto jumpAddress = reinterpret_cast<std::uintptr_t>(original + 5);

    *reinterpret_cast<std::int32_t*>(original + 1) =
        static_cast<std::int32_t>(hookAddress - jumpAddress);

    FlushInstructionCache(GetCurrentProcess(), original, length);

    DWORD temp = 0;

    VirtualProtect(original, length, oldProtection, &temp);

    return trampoline;
}

bool Memory::DetourRemove(BYTE* original,
                          const BYTE* originalBytes,
                          std::size_t length) {
    if (!original || !originalBytes || length == 0) {
        return false;
    }

    DWORD oldProtection = 0;

    if (!VirtualProtect(original, length, PAGE_EXECUTE_READWRITE,
                        &oldProtection)) {
        return false;
    }

    /*
     * Restaurar bytes originales.
     */
    std::memcpy(original, originalBytes, length);

    FlushInstructionCache(GetCurrentProcess(), original, length);

    DWORD temp = 0;

    VirtualProtect(original, length, oldProtection, &temp);

    return true;
}

bool Memory::bCompare(const BYTE* data, const BYTE* mask, const char* szMask) {
    if (!data || !mask || !szMask) {
        return false;
    }

    for (; *szMask; ++szMask, ++data, ++mask) {
        if (*szMask == 'x' && *data != *mask) {
            return false;
        }
    }

    return true;
}

Memory::MemoryHandle Memory::Signature::Scan(const MemoryRegion& region) const {
    if (!region.Base() || elements_.empty()) {
        return {};
    }

    auto compareMemory = [](const std::uint8_t* data, const Element* elements,
                            std::size_t count) -> bool {
        for (std::size_t i = 0; i < count; ++i) {
            if (!elements[i].WildCard && data[i] != elements[i].Data) {
                return false;
            }
        }

        return true;
    };

    const auto base = region.Base().As<std::uintptr_t>();

    const auto regionSize = region.Size();

    const auto patternSize = elements_.size();

    if (patternSize > regionSize)
        return {};

    const auto end = base + regionSize;

    for (std::uintptr_t address = base; address <= end - patternSize;
         ++address) {
        if (compareMemory(reinterpret_cast<const std::uint8_t*>(address),
                          elements_.data(), patternSize)) {
            return MemoryHandle(address);
        }
    }

    return {};
}
