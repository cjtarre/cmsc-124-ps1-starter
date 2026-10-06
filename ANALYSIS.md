# Joint Analysis

**Members:**
- Rafahil D. Palompon
- Ma. Christie Jude L. Tarre

## 1. Data Type Trade-offs

**Arrays: Ada.** Ada provides arrays with explicitly defined index ranges, including ranges that do not start at zero. In our C implementation, `dt_array` stores the element block, length, and lower bound, and each access manually checks the requested index and translates it into a zero-based storage offset. Ada makes these bounds and checks part of the language instead of requiring us to implement them ourselves. Both approaches can involve runtime bounds checking, which becomes more noticeable in code that performs many array accesses, although a compiler may eliminate checks it can prove unnecessary. The benefit is safer and more expressive indexing without manually maintaining the descriptor logic used in our C implementation.

**Records: Rust.** Rust provides structs whose named fields are known at compile time. Our `dt_record` instead stores copied field names and searches them using `strcmp()` in `dt_record_get` and `dt_record_set`. This requires storage for the names and runtime searching, whereas access to a normal Rust struct field can use a location known by the compiler. What the Rust struct gives up is runtime flexibility. Its fields normally have to be declared in advance, while our record can receive its field names when it is constructed. This difference matters most when field access is frequent or when records need schemas determined at runtime.

**Maps: Python.** Python provides dictionaries directly, while our `dt_map` manually implements FNV-1a hashing, bucket chaining, copied keys, allocation handling, and a separate array for insertion order. Python hides these responsibilities and provides a general-purpose dictionary capable of working with Python objects. Supporting that generality requires runtime metadata and bookkeeping that our more specialized map does not need. The memory cost becomes more noticeable when a program creates many dictionaries or stores large numbers of entries. In exchange, Python removes much of the manual memory management and data-structure implementation required by our C version.

## 2. Tagged Union Safety

## 3. Map Insertion Order

Our `dt_map` keeps insertion order in a separate dynamic array in addition to the hash buckets used for lookup. An alternative design could remove this array and store only the hash buckets and their chained entries. This would reduce memory usage and simplify insertion and removal because the map would no longer have to maintain two structures. Key-based lookup would still work, but traversing the map would follow the organization of the hash buckets rather than the order in which keys were inserted. As a result, `dt_map_key_at()` could no longer provide the required insertion order, and the behavior where updating a key preserves its position while removing and reinserting it moves it to the end would be lost. We would not ship this alternative for our project because insertion order is part of the required behavior. However, for a map used only for key-based lookup where iteration order does not matter, the simpler unordered design could be a reasonable choice.

## 4. Access After Release vs. Unreleased Allocations