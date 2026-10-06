# Joint Analysis

**Members:**
- Rafahil D. Palompon
- Ma. Christie Jude L. Tarre

## 1. Data Type Trade-offs

**Arrays: Ada.** Ada provides arrays with explicitly defined index ranges, including ranges that do not start at zero. In our C implementation, `dt_array` stores the element block, length, and lower bound, and each access manually checks the requested index and translates it into a zero-based storage offset. Ada makes these bounds and checks part of the language instead of requiring us to implement them ourselves. Both approaches can involve runtime bounds checking, which becomes more noticeable in code that performs many array accesses, although a compiler may eliminate checks it can prove unnecessary. The benefit is safer and more expressive indexing without manually maintaining the descriptor logic used in our C implementation.

**Records: Rust.** Rust provides structs whose named fields are known at compile time. Our `dt_record` instead stores copied field names and searches them using `strcmp()` in `dt_record_get` and `dt_record_set`. This requires storage for the names and runtime searching, whereas access to a normal Rust struct field can use a location known by the compiler. What the Rust struct gives up is runtime flexibility. Its fields normally have to be declared in advance, while our record can receive its field names when it is constructed. This difference matters most when field access is frequent or when records need schemas determined at runtime.

**Maps: Python.** Python provides dictionaries directly, while our `dt_map` manually implements FNV-1a hashing, bucket chaining, copied keys, allocation handling, and a separate array for insertion order. Python hides these responsibilities and provides a general-purpose dictionary capable of working with Python objects. Supporting that generality requires runtime metadata and bookkeeping that our more specialized map does not need. The memory cost becomes more noticeable when a program creates many dictionaries or stores large numbers of entries. In exchange, Python removes much of the manual memory management and data-structure implementation required by our C version.

## 2. Tagged Union Safety

Our dt_value uses a tag to identify which member of its C union contains the current value. Functions such as dt_value_as_int therefore have to check the tag manually before reading the corresponding union member. C still allows us to bypass this check, access a different union member directly, or forget to handle one of the possible tags. Languages such as Rust provide tagged unions through enum and can use match to require the programmer to handle every variant. C gives us more direct control over the underlying representation, which can be useful for low-level programming or interoperability, but it also places the responsibility for correct access on the programmer. For our implementation, this freedom is not particularly valuable because dt_value is intended to provide a predictable tagged-value abstraction. Compiler-enforced checks would therefore be preferable since they could prevent mistakes that we currently have to detect manually.

## 3. Map Insertion Order

Our `dt_map` keeps insertion order in a separate dynamic array in addition to the hash buckets used for lookup. An alternative design could remove this array and store only the hash buckets and their chained entries. This would reduce memory usage and simplify insertion and removal because the map would no longer have to maintain two structures. Key-based lookup would still work, but traversing the map would follow the organization of the hash buckets rather than the order in which keys were inserted. As a result, `dt_map_key_at()` could no longer provide the required insertion order, and the behavior where updating a key preserves its position while removing and reinserting it moves it to the end would be lost. We would not ship this alternative for our project because insertion order is part of the required behavior. However, for a map used only for key-based lookup where iteration order does not matter, the simpler unordered design could be a reasonable choice.

## 4. Access After Release vs. Unreleased Allocations

Access after release and an unreleased allocation are different ownership problems. Access after release happens when a resource has already been released, but the program attempts to use it again, which can lead to invalid data, incorrect behavior, or a crash. 

In a long-running server, access after release can result in dangling references in a memory block being handed over to another thread, and reading can result in corrupted data. This can also become a vulnerability where unauthorized attackers can inject malicious code that results in giving them access. In contrast, unreleased allocation can cause memory to build up over time and eventually cause the server to crash from memory overload.

 For command-line tools that can exit in a second, access after release can still do damage. An allocator may reuse the address for something new and end up overwriting data. A segfault may also get triggered and the OS immediately kills the program. Unreleased allocation on the other hand usually may just result in a few megabytes of memory being lost but since the process terminates immediately after running, the lost memory just gets reclaimed. 