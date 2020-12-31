/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <type_traits>
#include <vector>
#include <functional>

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/http/exchange.hpp>
#include <aether/proxy/tcp/websocket/pipeline.hpp>

#define EVENT_CATEGORIES(X, other) \
X(server, SERVER, other) \
X(http, HTTP, other) \
X(tls, TLS, other) \
X(tunnel, TUNNEL, other) \
X(websocket, WEBSOCKET, other) \
X(websocket_message, WEBSOCKET_MESSAGE, other)

#define SERVER_TYPES(X) X(connection::connection_flow &)
#define HTTP_TYPES(X) X(connection::connection_flow &), X(http::exchange &)
#define TLS_TYPES(X) X(connection::connection_flow &)
#define TUNNEL_TYPES(X) X(connection::connection_flow &)
#define WEBSOCKET_TYPES(X) X(connection::connection_flow &), X(websocket::pipeline &)
#define WEBSOCKET_MESSAGE_TYPES(X) WEBSOCKET_TYPES(X), X(websocket::message &)

#define SERVER_EVENTS(X, other1, other2) \
X(connect, 0, other1, other2) \
X(disconnect, 1, other1, other2)

#define HTTP_EVENTS(X, other1, other2) \
X(request, 2, other1, other2) \
X(connect, 3, other1, other2) \
X(any_request, 4, other1, other2) \
X(websocket_handshake, 5, other1, other2) \
X(response, 6, other1, other2) \
X(error, 7, other1, other2)

#define TLS_EVENTS(X, other1, other2) \
X(established, 8, other1, other2) \
X(error, 9, other1, other2)

#define TUNNEL_EVENTS(X, other1, other2) \
X(start, 10, other1, other2) \
X(stop, 11, other1, other2)

#define WEBSOCKET_EVENTS(X, other1, other2) \
X(start, 12, other1, other2) \
X(stop, 13, other1, other2) \
X(error, 14, other1, other2)

#define WEBSOCKET_MESSAGE_EVENTS(X, other1, other2) \
X(received, 15, other1, other2)

// Make sure this value is larger than all of the numbers above
#define MAX_EVENT_ENUM 16

namespace proxy::tcp::intercept {

    // Create enumeration types for all event categories

#define CREATE_EVENT_ENUMS(name, macro_name, foreach) \
enum class name##_event { macro_name##_EVENTS(foreach, , ) };
#define CREATE_EVENT_ENUM_ENTRY(name, num, other1, other2) name = num,

    EVENT_CATEGORIES(CREATE_EVENT_ENUMS, CREATE_EVENT_ENUM_ENTRY)

#undef CREATE_EVENT_ENUM_ENTRY
#undef CATEGORY_CREATE_EVENT_ENUMS

    // Create SFINAE methods for each event to test if a type has a standard event interceptor

    namespace _meta {

#define DECLARE_VAL(type) std::declval<type>()
#define CREATE_ALL_EVENT_SFINAES(name, macro_name, foreach) macro_name##_EVENTS(foreach, name, macro_name##_TYPES(DECLARE_VAL))
#define CREATE_EVENT_SFINAE(name, num, category, declared_types) \
template <typename T, typename = void> \
struct has_on_##category##_##name## : std::false_type { }; \
template <typename T> \
struct has_on_##category##_##name##<T, decltype(std::declval<T *>()->on_##category##_##name##(##declared_types##))> : std::true_type { };

        EVENT_CATEGORIES(CREATE_ALL_EVENT_SFINAES, CREATE_EVENT_SFINAE)

#undef CREATE_EVENT_SFINAE
#undef CREATE_ALL_EVENT_SFINAES
#undef DECLARE_VAL

#define CREATE_DISJUNCTION_EVENT_SFINAE(name, macro_name, foreach) \
template <typename T> \
using has_some_##name##_event = std::disjunction<##macro_name##_EVENTS(foreach, name, ) std::false_type>; \
template <typename T> \
constexpr bool has_some_##name##_event_v = has_some_##name##_event<T>::value;

#define CHECK_EVENT_SFINAE(name, num, category, other) has_on_##category##_##name##<T>,

        EVENT_CATEGORIES(CREATE_DISJUNCTION_EVENT_SFINAE, CHECK_EVENT_SFINAE)

#undef CHECK_EVENT_SFINAE
#undef CREATE_DISJUNCTION_EVENT_SFINAE

#define IS_EVENT_ENUM(name, macro_name, foreach) std::is_same<T, name##_event>,
        template <typename T>
        using is_event_enum = std::disjunction<EVENT_CATEGORIES(IS_EVENT_ENUM, ) std::false_type>;

        template <typename T>
        constexpr bool is_event_enum_v = is_event_enum<T>::value;

    }
    
    /*
        Base class for objects that group together one or more interceptor methods.
    */
    class interceptor_hub {
    private:
        std::vector<interceptor_id> ids = std::vector<interceptor_id>(MAX_EVENT_ENUM, not_attached);

    protected:
        interceptor_hub() = default;

        // Create empty virtual methods that can be overridden

    private:

#define GIVE_BACK_TYPE(type) type
#define CREATE_VIRTUAL_INTERCECPTORS(name, macro_name, foreach) macro_name##_EVENTS(foreach, name, macro_name##_TYPES(GIVE_BACK_TYPE))
#define CREATE_VIRTUAL_INTERCEPTOR(name, num, category, types) virtual void on_##category##_##name##(##types##) { }

        EVENT_CATEGORIES(CREATE_VIRTUAL_INTERCECPTORS, CREATE_VIRTUAL_INTERCEPTOR)

#undef CREATE_VIRTUAL_INTERCEPTOR
#undef CREATE_VIRTUAL_INTERCEPTORS
#undef GIVE_BACK_TYPE

    public:
        template <typename T>
        constexpr std::enable_if_t<_meta::is_event_enum_v<T>, interceptor_id &>
        operator[](T event) {
            return ids[static_cast<std::size_t>(event)];
        }
    };

    // Define an interceptor service for each event category

#define GIVE_BACK_TYPE(type) type
#define CREATE_INTERCEPTOR_SERVICES(name, macro_name, foreach) \
class name##_interceptor_service \
    : public base_interceptor_service<name##_event, macro_name##_TYPES(GIVE_BACK_TYPE)> { \
public: \
    template <typename T> \
    std::enable_if_t<std::is_base_of_v<interceptor_hub, T>, void> \
    attach_hub(T &hub) { \
        macro_name##_EVENTS(foreach, name, ) \
    } \
};
#define ADD_INTERCEPTOR_IF_METHOD_EXISTS(name, num, category, other) \
if constexpr (_meta::has_on_##category##_##name##<T>::value) { \
    hub[##category##_event::##name##] = attach(##category##_event::##name##, [&hub](auto &&... args) { hub.on_##category##_##name##(args...); }); \
}
        
    EVENT_CATEGORIES(CREATE_INTERCEPTOR_SERVICES, ADD_INTERCEPTOR_IF_METHOD_EXISTS)

#undef ADD_INTERCEPTOR_IF_METHOD_EXISTS
#undef CREATE_INTERCEPTOR_SERVICES
#undef GIVE_BACK_TYPE

    // Define an interceptor manger that owns all interceptor services

#define CREATE_SERVICE_MEMBER(name, macro_name, foreach) name##_interceptor_service name;
#define ATTACH_HUB_LOCALLY(name, macro_name, foreach) \
if constexpr (_meta::has_some_##name##_event_v<T>) { \
    ##name##.attach_hub(hub); \
}

    /*
        Interface to all interceptor services.
    */
    class interceptor_manager
        : private boost::noncopyable {
    public:
        EVENT_CATEGORIES(CREATE_SERVICE_MEMBER, )

        template <typename T>
        std::enable_if_t<std::is_base_of_v<interceptor_hub, T>, void>
        attach_hub(T &hub) {
            EVENT_CATEGORIES(ATTACH_HUB_LOCALLY, )
        }
    };

#undef ATTACH_HUB_LOCALLY
#undef CREATE_SERVICE_MEMBER
}