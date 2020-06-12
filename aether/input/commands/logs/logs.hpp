/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/input/commands/base_command.hpp>
#include <aether/input/util/signal_handler.hpp>
#include <aether/interceptors/attach.hpp>
#include <aether/interceptors/http/http_logger.hpp>
#include <aether/util/console.hpp>

namespace input::commands {
    class logs
        : public base_command {
    private:
        std::unique_ptr<util::signal_handler> signals;
        bool running = false;

        void attach_interceptors(proxy::server &server);
        void stop();

    public:
        void run(const arguments &args, proxy::server &server, command_service &caller) override;

        inline std::string name() const override {
            return "logs";
        }
        inline std::string args() const override {
            return "";
        }
        inline std::string description() const override {
            return "Starts logging all server activity.";
        }
        inline bool uses_signals() const override {
            return true;
        }
    };
}