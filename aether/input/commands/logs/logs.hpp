/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/input/commands/base_command.hpp>
#include <aether/interceptors/attach.hpp>
#include <aether/interceptors/http/http_logger.hpp>
#include <aether/util/signal_handler.hpp>
#include <aether/util/thread_blocker.hpp>

namespace input::commands {
    class logs
        : public base_command {
    private:
        std::unique_ptr<util::signal_handler> signals;
        util::thread_blocker blocker;
        proxy::tcp::intercept::interceptor_id http_id;

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

        /*
            Attaches all logging interceptors to the server and enables logging.
        */
        void attach_interceptors(proxy::server &server);

        /*
            Detaches all logging interceptors from the server.
        */
        void detach_interceptors(proxy::server &server);
    };
}