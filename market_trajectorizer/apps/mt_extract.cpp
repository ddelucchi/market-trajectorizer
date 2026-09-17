#include "mt/api/cli_commands.hpp"
#include "mt/app/run_extract.hpp"

int main(int argc, char** argv) {
    return mt::app::run_extract(mt::parse_cli(argc, argv));
}
