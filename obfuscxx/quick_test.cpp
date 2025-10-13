#include "metrics/unit_test.h"
#include "metrics/benchmark.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto const result = RUN_ALL_TESTS();

    std::cout << '\n';

    if (result != 0) {
        std::cout << "UNIT TEST FAILED: RESULT: " << result << '\n';
        return result;
    }

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
