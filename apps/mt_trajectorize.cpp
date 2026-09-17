#include "mt/api/cli_commands.hpp"
#include "mt/app/run_trajectorize.hpp"

int main(int argc, char** argv) {
    return mt::app::run_trajectorize(mt::parse_cli(argc, argv));
}
