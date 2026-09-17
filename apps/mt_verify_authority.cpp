#include "mt/api/cli_commands.hpp"
#include "mt/app/run_verify_authority.hpp"

int main(int argc, char** argv) {
    return mt::app::run_verify_authority(mt::parse_cli(argc, argv));
}
