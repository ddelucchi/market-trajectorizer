#include "mt/api/cli_commands.hpp"
#include "mt/app/run_benchmark.hpp"

int main(int argc, char** argv) {
    return mt::app::run_benchmark(mt::parse_cli(argc, argv));
}
