#include <spdlog/spdlog.h>

#include <game/config.h>
#include <game/game.h>

using namespace game;

int main(int argc, char *argv[]){
    auto _logger = spdlog::stdout_color_mt(Config::logger_name());
    spdlog::set_level(spdlog::level::trace);
    Game game;
    if(argc > 1 && argv[1]) Config::map_name(argv[1]);

    return game.start();
}
