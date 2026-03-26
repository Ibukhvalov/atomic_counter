# Test
```bash
g++ -std=c++17 ./Part1/test_atomic_counter_func.cpp -o test_variadic && ./test_func
g++ -std=c++17 ./Part2/test_atomic_counter_variadic.cpp -o test_variadic && ./test_variadic
```

# Some notes
This repo implementats wait-free algorithm which consumes NumberOfWords-1 bits for syncronization.

There is also an implementation without bits for internal usage but it is only lock-free.

Function signature is a bit differ from the given, since it is much more convenient to use std::atomic<uint32_t>, but it could be made with the given one with some casts or even raw c-style atomic calls

Also, there is an assumption, that neither of threads would be waiting more than the 2^(lowerAccumulatorBits-1) or it would yield wrong fetched result since phase (reserved) bit is responsible for that specific range and flips on overflow.
