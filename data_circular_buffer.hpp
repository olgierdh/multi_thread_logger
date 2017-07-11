#ifndef __DATA_CIRCULAR_BUFFER_HPP__
#define __DATA_CIRCULAR_BUFFER_HPP__

#include <cstdlib>
#include <atomic>
#include <stdexcept>
#include <algorithm>


namespace tt
{
    template < size_t N > class data_circular_buffer
    {
      public:
        data_circular_buffer()
            : m_buffer{0}, m_head( 0 ), m_tail( 0 ), m_size_occupied( 0 ),
              m_size_to_read( 0 )
        {
        }

      public:
        template < typename T > void dump_object( const T& obj )
        {
            constexpr auto size = sizeof( T );

            auto old_tail = m_tail.load( std::memory_order_acquire );

            if ( m_size_occupied.load( std::memory_order_acquire ) + size > N )
            {
                throw std::runtime_error( "Size exceeded!" );
            }
            
            m_size_occupied.fetch_add( size, std::memory_order_acq_rel );

            // now let's calculate the new position of the tail since we know that we can
            // fit the data, we just don't know where
            auto new_tail = ( old_tail + size ) % N;

            if ( !m_tail.compare_exchange_strong( old_tail, new_tail,
                                                  std::memory_order_release ) )
            {
                throw std::runtime_error( "Synchronization issue!" );
            }

            // now we have prepared a place for the object, we can write the data and
            // increase the bytes ready to read
            uint8_t* ptr = &m_buffer[old_tail];

            const size_t capacity_left = std::min( N - old_tail, size );
            const size_t size_left     = size - capacity_left;

            memcpy( ptr, &obj, capacity_left );

            if ( size_left > 0 )
            {
                ptr = &m_buffer[0];
                memcpy( ptr, ( ( uint8_t* ) &obj ) + capacity_left, size_left );
            }

            m_size_to_read.fetch_add( size, std::memory_order_release );

            // ok we are done
        }

        template < typename T > void retrieve_object( T& obj )
        {
            constexpr auto size = sizeof( T );

            auto old_head = m_head.load( std::memory_order_acquire );

            if ( m_size_to_read.load( std::memory_order_acquire ) < size )
            {
                throw std::runtime_error( "Not enough data!" );
            }

            //
            auto new_head = ( old_head + size ) % N;

            uint8_t* ptr               = &m_buffer[old_head];
            const size_t capacity_left = std::min( N - old_head, size );
            const size_t size_left     = size - capacity_left;

            memcpy( &obj, ptr, capacity_left );

            if ( size_left > 0 )
            {
                ptr = &m_buffer[0];
                memcpy( ( ( uint8_t* ) &obj ) + capacity_left, ptr, size_left );
            }

            if ( !m_head.compare_exchange_strong( old_head, new_head,
                                                std::memory_order_acq_rel ) )
            {
                throw std::runtime_error( "Synchronization issue!" );
            }

            m_size_occupied.fetch_sub( size, std::memory_order_release );
            m_size_to_read.fetch_sub( size, std::memory_order_release );
        }

        size_t size_to_read()
        {
            return m_size_to_read.load( std::memory_order_acquire );
        }

        size_t size_occupied()
        {
            return m_size_occupied.load( std::memory_order_acquire );
        }

        template < typename T > bool can_fit()
        {
            return ( m_size_occupied.load( std::memory_order_acquire ) + sizeof( T ) ) < N;
        }

        template < typename T > bool can_read()
        {
            return m_size_to_read.load( std::memory_order_acquire ) >= sizeof( T );
        }

      private:
      private:
        /* buffer for data - flat array of bytes */
        uint8_t m_buffer[N];

        /* set of atimics to deal with the concurency */
        std::atomic_size_t m_head;
        std::atomic_size_t m_tail;
        std::atomic_size_t m_size_occupied;
        std::atomic_size_t m_size_to_read;
    };
}

#endif // __DATA_CIRCULAR_BUFFER_HPP__
