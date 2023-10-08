# Gkm-Local code style convention

## Language standard
On Windows we use the latest Visual Studio IDE (currently VS 2022) and the corresponding C++ compiler.

## File names
* Source code should be stored in files either with *.cpp* or *.h* extensions.
  File names should use *snake_case* naming convention.
* Each namespace should use its own folder. Folder name should be the same as namespace name, except different naming convention.
  Folders could be nested in the same way as namespaces.

## Formatting
* Spaces should be used in source and header files. Tabs symbols are prohibited.
* Tab size should be equal to 4 space symbols.
* Local scopes (```{``` and ```}``` keywords) should be used as defined in BSD-style code convention.
* Don't use space symbols at the end of lines.
* Empty lines should not contain space symbols.
* Each source or header file should contain one empty line at the end of file.
* Instead of using macros, *#pragma once* directive should be used.
* Usually comments inside source code should begin with double slash (//) symbols, then a space should come.
  Text should begin with a capital letter, like that: *// Compress points*.
  In case if a comment at the end of the source code line then it should be separated by a space from the source code:
  ```int i = 1; // Comment text```
* Use *unsigned* keyword instead of *unsigned int*.
* For pointer and reference types use _type_name* var_name_ or _type_name& var_name_ scheme.

## Conventions of file inclusions
The file inclusions should be organized in fhe following order:
* Include system headers, like ```#include <vector>```
* Include libraries from *3rdparty* folder, like *Eigen*: ```#include "Eigen/Eigen"```
  Inclusion of libraries from *3rdparty folder uses quotes ("") instead of angle brackets (&lt;&gt;) because we don't use these libraries from the system locations.
* Include Gkm-Local specific libraries, like ```#include "protocol.h"```
* Include all other local headers.

## Naming convention
* Namespace names should be in *PascalCase* naming convention.
  Prefer names in singular form rather than plural names.
  For instance, use *Network::Container* instead of *Network::Containers*.
* Interface names should be in *PascalCase* naming convention starting with capital *I* letter.
  For instance, *IMemoryBuffer*.
* Interface methods should use *camelCase* naming convention.
* Classes and structures should use *PascalCase* naming convention.
* Class and structure methods should use *camelCase* naming convention.
* Arguments for methods, interface methods, functions, constructors and destructors should use *snake_case* naming convention.
* Local variables should use *snake_case* naming convention.
* Class and structure fields should use *snake_case* naming convention starting without any prefix.
  For instance, *buffer*. If field name conflicts with argument name then add *'_'* suffix to the argument name,
  for instance, *buffer_*.
* Template parameters should use *PascalCase* naming convention. For instance, *Vector*.
* Standalone functions should use *camelCase* naming convention.
* Enumerators should use *PascalCase* naming convention with *E* prefix.
  Enumerator members also should use *PascalCase* naming convention without any prefix. Use strongly typed enumerations.
* Constants could be declared inside structures, classes or namespaces. They should use *MACROS_CASE* naming convention.
* Macros should be used where they are really needed. They should use *MACROS_CASE* naming convention.
* Typedefs should use *PascalCase* naming convention.

## Coding recommendations
* The C++ "const" specifier should be used where it is possible.
* The C++ "override" and "final" specifiers should be used where it is possible.
* The C-style type casting is prohibited, instead use C++ style type casting, for instance, ```static_cast<>``` or other.
* Code should be exception safety as much as possible. We should satisfy at least *basic* guarantee.
* New and delete statements are prohibited, except case when new statement is a part of smart pointer initialization expression.
  Instead consider to use *std::vector* for temporary buffers or other smart buffer types.
  Consider to use factories which return smart pointers. Use smart pointers instead of raw pointers.
  Use raw pointers only when you need to have an non-owning pointers which are not required to be deallocated.
* FILE* pointers are prohibited. Instead consider to use *std::fstream* to work with files.
* We should allow running our code in parallel environment where it is possible.
* Don't use global variables where it is possible.
  Use *g_* prefix for global variables.

## Glossary
* *PascalCase* naming convention.
  Naming convention where names start with a capital letter.
  Name can consist from several words and each word starts with capital letter.
  Other letters are small. Some examples are *MemoryBuffer*, *TriangleFan*, *CreateMemoryBuffer*.
  In some cases when a part of name is a contraction this part of name could be all in upper case.
* *camelCase* naming convention.
  Naming convention where names start with a small letter.
  Name can consist from several words and each word starts with capital letter (except first letter).
  Other letters are small. Some examples are *memoryBuffer*, *triangleFan*, *createMemoryBuffer*.
  In some cases when a part of name is a contraction this part of name could be all in upper case.
* *snake_case* naming convention.
  Naming convention where all letters in a name are small.
  Name can consist from several words and each word is separated from other words by using underscore symbol.
  Some examples are *memory_buffer*, *triangle_fan*, *create_memory_buffer*.
* *kebab-case* naming convention.
  Naming convention where all letters in a name are small.
  Name can consist from several words and each word is separated from other words by using hyphen symbol.
  Some examples are *memory-buffer*, *triangle-fan*, *create-memory-buffer*.
* *MACROS_CASE* naming convention.
  Naming convention where all letters in a name are capital.
  Name can consist from several words and each word is separated from other words by using underscore symbol.
  Some examples are *MEMORY_BUFFER*, *TRIANGLE_FAN*, *CREATE_MEMORY_BUFFER*.
