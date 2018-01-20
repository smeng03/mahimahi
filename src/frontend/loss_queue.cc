/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <iostream>

#include "loss_queue.hh"
#include "timestamp.hh"

using namespace std;

LossQueue::LossQueue()
    : prng_( random_device()() )
{}

void LossQueue::read_packet( const string & contents )
{
    if ( not drop_packet( contents ) ) {
        packet_queue_.emplace( contents );
    }
}

void LossQueue::write_packets( FileDescriptor & fd )
{
    while ( not packet_queue_.empty() ) {
        fd.write( packet_queue_.front() );
        packet_queue_.pop();
    }
}

unsigned int LossQueue::wait_time( void )
{
    return packet_queue_.empty() ? numeric_limits<uint16_t>::max() : 0;
}

bool IIDLoss::drop_packet( const string & packet __attribute((unused)) )
{
    return drop_dist_( prng_ );
}

bool DeterministicLoss::drop_packet( const string & packet __attribute((unused)) )
{
    //this->drop_counter++;
    //if (this->drop_counter % 100 == 0) {
    //    return true;
    //}

    //return false;
    int super_random_value = rand() % 10000;
    if (super_random_value < ((int) (this->loss_rate * 10000))) {
        return true;
    }
    return false;
    //return (rand() % 100) < ((int) this->loss_rate * 100);
}

static const double MS_PER_SECOND = 1000.0;

SwitchingLink::SwitchingLink( const double mean_on_time, const double mean_off_time )
    : link_is_on_( false ),
      on_process_( 1.0 / (MS_PER_SECOND * mean_off_time) ),
      off_process_( 1.0 / (MS_PER_SECOND * mean_on_time) ),
      next_switch_time_( timestamp() )
{}

uint64_t bound( const double x )
{
    if ( x > (1 << 30) ) {
        return 1 << 30;
    }

    return x;
}

unsigned int SwitchingLink::wait_time( void )
{
    const uint64_t now = timestamp();

    while ( next_switch_time_ <= now ) {
        /* switch */
        link_is_on_ = !link_is_on_;
        /* worried about integer overflow when mean time = 0 */
        next_switch_time_ += bound( (link_is_on_ ? off_process_ : on_process_)( prng_ ) );
    }

    if ( LossQueue::wait_time() == 0 ) {
        return 0;
    }

    if ( next_switch_time_ - now > numeric_limits<uint16_t>::max() ) {
        return numeric_limits<uint16_t>::max();
    }

    return next_switch_time_ - now;
}

bool SwitchingLink::drop_packet( const string & packet __attribute((unused)) )
{
    return !link_is_on_;
}
