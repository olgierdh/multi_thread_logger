#include "logger.hpp"
#include "data_circular_buffer.hpp"

#include <thread>
#include <chrono>
#include <vector>
#include <cassert>
#include <stdexcept>

struct foo
{
    int a = 123;
    int b = 344;
    char a_str[256];
};

int main( int, const char** )
{
#if 1
    std::atomic_int atomic_i( 0 );
    std::atomic_bool work( true );

    int b = 32;

    std::thread t0( [&]() {
        while ( work )
        {
            int p = 0;
            
            if( ( p = atomic_i.load( std::memory_order_acquire ) ) < 2 )
                continue;

            assert( b == 56 );
        }
    } );
    
    std::thread t1( [&]() {
        while ( work )
        {
            int p = 0;
            
            if( ( p = atomic_i.load( std::memory_order_acquire ) ) < 2 )
                continue;

            assert( b == 56 );
        }
    } );

    std::thread t2( [&]() {
        // std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        while ( work )
        {
            int expected = 1;
            if ( !atomic_i.compare_exchange_strong( expected, 2,
                                                       std::memory_order_acq_rel ) )
            {
                continue;
            }
            b = 56;
        }
    } );

    std::thread t3( [&]() {
        // std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        
        while ( work )
        {
            b = 52;
            atomic_i.store( 1, std::memory_order_release );
        }
    } );

    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

    work.store( false, std::memory_order_relaxed );

    t0.join();
    t1.join();
    t2.join();
    t3.join();
#endif
#if 0 
    tt::data_circular_buffer< 65536 > dcb;
    tt::handle_log( 123, " test ", 1234, " bla bla bla " );

    using thread_pool_t = std::vector< std::thread >;
    thread_pool_t thread_pool;

    std::atomic_bool work( true );

    std::thread consumer( [&] {
        while ( work )
        {
            foo a_foo;

            while ( !dcb.can_read< foo >() )
                ;
            dcb.retrieve_object( a_foo );

            std::cout << "retrieved object foo: "
                      << "a: " << a_foo.a << " b: " << a_foo.b << " c: " << a_foo.a_str
                      << std::endl;
        }
    } );

    for ( int i = 0; i < 10; ++i )
    {
        thread_pool.emplace_back( [&] {
            while ( work )
            {
                if ( dcb.can_fit< foo >() )
                {
                    foo a_foo{21, 323, "test"};
                    dcb.dump_object( a_foo );
                }
            }
        } );
    }

    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );

    work = false;
    for ( auto& c : thread_pool )
    {
        c.join();
    }

    consumer.join();
#endif
    return 0;
}
