#include "mt/api/cli_commands.hpp"
#include "mt/app/run_backtest.hpp"

int main(int argc, char** argv) {
    return mt::app::run_backtest(mt::parse_cli(argc, argv));
}
