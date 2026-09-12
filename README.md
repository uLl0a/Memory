# Memory

Utility C++ para trabajar con información de módulos, búsqueda de patrones, regiones de memoria y manipulación controlada de memoria dentro de aplicaciones Windows.

**Requisitos**
* C++17 o superior
* Windows SDK

La funcionalidad está agrupada en una única clase:
```cpp
class Memory
```

Dentro de ella también existen las clases auxiliares:
```cpp
Memory::MemoryHandle
Memory::MemoryRegion
Memory::Signature
```
## 1. Include

En cualquier archivo donde quieras utilizar la clase:
```cpp
#include "Memory.h"
```

No necesitas declarar un namespace adicional.

Ejemplo:
```cpp
#include "Memory.h"

int main()
{
    // Memory usage
}
```
## 2. GetModuleInfo

Obtiene información de un módulo cargado en el proceso actual.

Uso
```cpp
MODULEINFO info =
    Memory::GetModuleInfo("Game.exe");

```

Puedes obtener la dirección base:
```cpp
auto base =
    reinterpret_cast<std::uintptr_t>(
        info.lpBaseOfDll
    );

```
Y el tamaño de la imagen:
```cpp
auto size =
    static_cast<std::size_t>(
        info.SizeOfImage
    );

```
Por ejemplo:
```cpp
MODULEINFO info =
    Memory::GetModuleInfo("Game.exe");

if (info.lpBaseOfDll)
{
    std::uintptr_t base =
        reinterpret_cast<std::uintptr_t>(
            info.lpBaseOfDll
        );

    std::size_t size =
        info.SizeOfImage;
}
```

Si el módulo no existe o no se puede obtener su información, se devuelve una estructura vacía.

## 3. FindPattern

FindPattern busca una secuencia de bytes dentro de un módulo utilizando una máscara.

La firma es:
```cpp
static std::uintptr_t FindPattern(
    const char* module,
    const char* pattern,
    const char* mask
);
```
Máscara

La máscara utiliza:
```cpp
x
```

para indicar un byte que debe coincidir y:
```cpp
?
```

para indicar un byte comodín.

Por ejemplo:
```cpp
"\x48\x8B\x05\x00\x00\x00\x00"
```

con:
```cpp
"xxx????"
```

significa:
```cpp
48 8B 05 ?? ?? ?? ??
```

Uso:
```cpp
auto address = Memory::FindPattern(
    "Game.exe",
    "\x48\x8B\x05\x00\x00\x00\x00",
    "xxx????"
);

if (address)
{
    // Pattern found
}
```

El resultado es un std::uintptr_t.

Si no encuentra el patrón:
```cpp
address == 0
```
## 4. MemoryHandle

MemoryHandle es un wrapper pequeño para trabajar con direcciones.

Puedes construirlo utilizando un puntero:
```cpp
Memory::MemoryHandle handle(
    reinterpret_cast<void*>(address)
);
```

O directamente con una dirección:
```cpp
Memory::MemoryHandle handle(address);
```
Obtener una dirección
```cpp
auto address = handle.As<std::uintptr_t>();
```
Obtener un puntero
```cpp
auto ptr = handle.As<int*>();
```

También puedes utilizar otros tipos:
```cpp
auto ptr = handle.As<MyStruct*>();
```
Add

Permite añadir un offset:
```cpp
auto next = handle.Add(0x20);
```

Equivale conceptualmente a:
```cpp
address + 0x20
```
Sub

Para restar un offset:
```cpp
auto previous = handle.Sub(0x20);
```
Rip

Rip() permite resolver una dirección relativa de 32 bits, común en determinadas instrucciones x64.
```cpp
auto target = handle.Rip();
```
## 5. MemoryRegion

MemoryRegion representa una región de memoria utilizando una dirección base y un tamaño.

Ejemplo:
```cpp
MODULEINFO info =
    Memory::GetModuleInfo("Game.exe");

Memory::MemoryRegion region(
    Memory::MemoryHandle(info.lpBaseOfDll),
    info.SizeOfImage
);
```

Ahora puedes obtener:
```cpp
auto base = region.Base();
auto end = region.End();
auto size = region.Size();
```
Contains

Puedes comprobar si una dirección pertenece a la región:
```cpp
Memory::MemoryHandle address(
    reinterpret_cast<void*>(someAddress)
);

if (region.Contains(address))
{
    // Address belongs to region
}
```

# 6. Signature

Signature proporciona una forma alternativa de construir un patrón utilizando elementos individuales.

Ejemplo:

Memory::Signature signature;
```cpp
signature.Add(0x48);
signature.Add(0x8B);
signature.Add(0x05);
```

Los bytes comodín se pueden añadir así:
```cpp
signature.Add(0x00, true);
```

Por ejemplo:

Memory::Signature signature;
```cpp
signature.Add(0x48);
signature.Add(0x8B);
signature.Add(0x05);

signature.Add(0x00, true);
signature.Add(0x00, true);
signature.Add(0x00, true);
signature.Add(0x00, true);
```

Esto representa:
```cpp
48 8B 05 ?? ?? ?? ??
```
# 7. Scan

Una vez creada una Signature, puedes buscarla dentro de una MemoryRegion.
```cpp
MODULEINFO info =
    Memory::GetModuleInfo("Game.exe");

Memory::MemoryRegion region(
    Memory::MemoryHandle(info.lpBaseOfDll),
    info.SizeOfImage
);

Memory::Signature signature;

signature.Add(0x48);
signature.Add(0x8B);
signature.Add(0x05);

signature.Add(0x00, true);
signature.Add(0x00, true);
signature.Add(0x00, true);
signature.Add(0x00, true);

auto result =
    signature.Scan(region);
```

Comprueba el resultado:
```cpp
if (result)
{
    auto address =
        result.As<std::uintptr_t>();
}

```
## 8. WriteMemory

WriteMemory permite escribir un int utilizando una dirección base y offsets.
```cpp
Memory::WriteMemory(
    baseAddress,
    value,
    offset1,
    offset2
);
```

Ejemplo:
```cpp
Memory::WriteMemory(
    baseAddress,
    100,
    0x10,
    0x20
);
```

Conceptualmente realiza:
```cpp
*(baseAddress + offset1) + offset2 = value
```

Esta función presupone que la cadena de punteros y las direcciones utilizadas son válidas.

## 9. Patch

Patch permite copiar bytes a una dirección determinada.

Ejemplo:
```cpp
BYTE data[] =
{
    0x90,
    0x90
};

Memory::Patch(
    reinterpret_cast<BYTE*>(address),
    data,
    sizeof(data)
);
```

La función devuelve true si la operación pudo realizarse:
```cpp
if (Memory::Patch(
        reinterpret_cast<BYTE*>(address),
        data,
        sizeof(data)))
{
    // Success
}
```

También puedes utilizar un array:
```cpp
const BYTE data[] =
{
    0x01,
    0x02,
    0x03,
    0x04
};

Memory::Patch(
    reinterpret_cast<BYTE*>(address),
    data,
    sizeof(data)
);
```
## 10. ReadText

ReadText convierte una dirección en un puntero const char*.
```cpp
const char* text =
    Memory::ReadText(
        reinterpret_cast<BYTE*>(address)
    );
```

Después:
```cpp
printf("%s\n", text);
```

La dirección debe apuntar a una cadena válida y terminada en \0.

## 11. bCompare

bCompare compara bytes utilizando una máscara.
```cpp
BYTE data[] =
{
    0x48,
    0x8B,
    0x05,
    0x12
};

BYTE mask[] =
{
    0x48,
    0x8B,
    0x05,
    0xFF
};

bool result =
    Memory::bCompare(
        data,
        mask,
        "xxx?"
    );
```

La máscara utiliza:
```
x = comparar byte
? = ignorar byte
```
## 12. DetourApply / DetourRemove
```DetourApply()``` permite instalar un detour sobre una dirección de memoria, redirigiendo la ejecución hacia una función destino.

```DetourRemove()``` elimina el detour previamente instalado y restaura los bytes originales.

### DetourApply
Uso general:
```cpp
Memory::DetourApply(
    address,
    destination
);
```
Donde:

address es la dirección donde se instalará el detour.
destination es la dirección de la función a la que se redirigirá la ejecución.
La función guarda los bytes originales necesarios para poder restaurarlos posteriormente.

Ejemplo conceptual:
```cpp
auto address = Memory::FindPattern(
    "Game.exe",
    "\x48\x8B\x05\x00\x00\x00\x00",
    "xxx????"
);

if (address)
{
    Memory::DetourApply(
        reinterpret_cast<BYTE*>(address),
        reinterpret_cast<void*>(&MyFunction)
    );
}
```
### DetourRemove
Para eliminar el detour y restaurar los bytes originales:
```cpp
Memory::DetourRemove(address);
```
Ejemplo:
```cpp
Memory::DetourRemove(
    reinterpret_cast<BYTE*>(address)
);
```
Esto restaura los bytes que se encontraban en la dirección antes de aplicar el detour.

## 13. Ejemplo

Un ejemplo combinando GetModuleInfo, MemoryRegion y Signature:
```cpp
#include "Memory.h"

int main()
{
    MODULEINFO info =
        Memory::GetModuleInfo("Game.exe");

    if (!info.lpBaseOfDll)
        return 1;

    Memory::MemoryRegion region(
        Memory::MemoryHandle(
            info.lpBaseOfDll
        ),
        info.SizeOfImage
    );

    Memory::Signature signature;

    signature.Add(0x48);
    signature.Add(0x8B);
    signature.Add(0x05);

    signature.Add(0x00, true);
    signature.Add(0x00, true);
    signature.Add(0x00, true);
    signature.Add(0x00, true);

    Memory::MemoryHandle result =
        signature.Scan(region);

    if (!result)
        return 1;

    std::uintptr_t address =
        result.As<std::uintptr_t>();

    return 0;
}
```
## API Reference
| Función | Descripción |
|---|---|
| `GetModuleInfo()` | Obtiene información de un módulo |
| `WriteMemory()` | Escribe un `int` mediante offsets |
| `FindPattern()` | Busca un patrón usando máscara |
| `Patch()` | Copia bytes a una dirección |
| `ReadText()` | Obtiene texto desde una dirección |
| `DetourApply()` | Instala un detour |
| `DetourRemove()` | Restaura bytes originales |
| `bCompare()` | Compara bytes con máscara |
| `MemoryHandle::Add()` | Añade un offset |
| `MemoryHandle::Sub()` | Resta un offset |
| `MemoryHandle::Rip()` | Resuelve una dirección relativa |
| `MemoryRegion::Contains()` | Comprueba pertenencia a una región |
| `Signature::Scan()` | Busca una signature en una región |
