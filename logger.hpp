#ifndef __LOGGING_HPP__
#define __LOGGING_HPP__

#include <type_traits>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>
#include <bitset>

namespace tt
{
    namespace log_levels
    {
        struct LOGGER_INFO
        {
            static constexpr char id[] = "I";
        };

        struct LOGGER_WARNING
        {
            static constexpr char id[] = "W";
        };

        struct LOGGER_ERROR
        {
            static constexpr char id[] = "E";
        };
    };

    namespace detail
    {
        template < typename... Ts > struct types_holder : public Ts...
        {
        };

        struct type_defined
        {
        };

        struct type_undefined
        {
        };

        template < typename T > constexpr type_defined is_type_defined_impl( T&& );
        template < typename T > constexpr type_undefined is_type_defined_impl( ... );

        template < typename T, typename Tt > struct is_type_defined
        {
            static constexpr bool value =
                std::is_same< decltype( is_type_defined_impl< T >( Tt{} ) ),
                              type_defined >::value;
        };

        template < typename... T > constexpr void expander( T&&... )
        {
        }
    }

    template < typename... T >
    struct logger_configuration : public detail::types_holder< T... >
    {
    };

    // log receiver should work on worker thread in order to enable thread safe
    // logging
    class logger_receiver
    {
            public:
        logger_receiver()
        {
        }

            public:
        std::thread m_thread;
    };

    template < size_t... args > struct summer;
    template < size_t head, size_t... tail > struct summer< head, tail... >
    {
        static constexpr size_t value = head + summer< tail... >::value;
    };

    template < size_t head > struct summer< head >
    {
        static constexpr size_t value = head;
    };

    template < typename... Args > constexpr size_t args_pack_size()
    {
        return summer< sizeof( Args )... >::value;
    }

    template < typename T > constexpr bool extract_log_type( uint8_t** data )
    {
        T ret{};
        std::memcpy( ( void* )&ret, ( const void* )*data, sizeof( T ) );
        *data += sizeof( T );
        std::cout << ret;
        return true;
    }

    template < typename T > constexpr int pack_type( const T& t, uint8_t** data )
    {
        std::memcpy( *data, ( const void* )&t, sizeof( T ) );
        *data += sizeof( T );
        return 0;
    }

    using extract_fnc_t = void ( * )( uint8_t** data );

    template < typename... Args > void extract_log_data( uint8_t** data )
    {
        detail::expander( extract_log_type< Args >( data )... );
        std::cout << std::endl;
    }

    template < typename... Args > void handle_log( const Args&... args )
    {
        constexpr size_t array_size = args_pack_size< Args... >();

        uint8_t buffer[array_size] = {0};

        {
            uint8_t* ptr   = &buffer[0];
            uint8_t** ptr2 = &ptr;
            detail::expander( pack_type( args, ptr2 )... );
        }

        extract_fnc_t fnc = &extract_log_data< Args... >;

        {
            uint8_t* ptr   = &buffer[0];
            uint8_t** ptr2 = &ptr;

            fnc( ptr2 );
        }
    }


    template < typename Config > class logger_interface
    {
            public:
        template < typename LL,
                   typename = std::enable_if_t<
                       tt::detail::is_type_defined< LL, Config >::value >,
                   typename... Args >
        static void log( Args&&... args )
        {
            auto now =
                std::chrono::high_resolution_clock::now().time_since_epoch().count();

            std::cout << "[" << LL::id << "]";
            std::cout << "[tid: " << std::this_thread::get_id() << "]";
            std::cout << "[t :" << now << "]";
            std::cout << " ";

            tt::detail::expander( std::cout << std::forward< Args >( args )... );

            std::cout << std::endl;
        }

        // empty logging function for disabled log levels
        template < typename... T > static void log( ... )
        {
        }

        template < typename... Args > static void log_info( Args&&... args )
        {
            log< tt::log_levels::LOGGER_INFO >( std::forward< Args >( args )... );
        }

        template < typename... Args > static void log_warning( Args&&... args )
        {
            log< tt::log_levels::LOGGER_WARNING >( std::forward< Args >( args )... );
        }

        template < typename... Args > static void log_error( Args&&... args )
        {
            log< tt::log_levels::LOGGER_ERROR >( std::forward< Args >( args )... );
        }
    };

    using default_logger_config =
        tt::logger_configuration< tt::log_levels::LOGGER_INFO,
                                  tt::log_levels::LOGGER_ERROR,
                                  tt::log_levels::LOGGER_WARNING >;
}
#endif // __LOGGING_HPP__
