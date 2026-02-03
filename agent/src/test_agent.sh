#!/bin/bash

echo "--- Building project ---"
mkdir -p build && cd build
cmake .. && make -j$(nproc)

g++ ../src/test_app.cpp -o test_app -g

./test_app > /dev/null &
TEST_PID=$!
echo "--- Started test_app (PID: $TEST_PID) ---"

echo "--- Profiling for 3 seconds ---"
./cputrace --pid $TEST_PID --duration 3 --out test_trace.json

kill $TEST_PID

echo "--- Verifying Symbol Resolution in test_trace.json ---"
if grep -q "expensive_math_task" test_trace.json; then
    echo "SUCCESS: Found 'expensive_math_task' in trace!"
else
    echo "FAILURE: Function names not found. Check SymbolResolver logic."
fi

if grep -q "memory_heavy_task" test_trace.json; then
    echo "SUCCESS: Found 'memory_heavy_task' in trace!"
fi