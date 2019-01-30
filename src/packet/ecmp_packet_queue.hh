/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef ECMP_PACKET_QUEUE_HH
#define ECMP_PACKET_QUEUE_HH

#include <queue>
#include <cassert>
#include <random>

#include "abstract_packet_queue.hh"
#include "drop_tail_packet_queue.hh"
#include "exception.hh"

class ECMPPacketQueue : public AbstractPacketQueue
{
private:
    size_t num_queues_,
           curr_queue_,
           qlen_bytes_,
           qlen_pkts_;
    bool   work_conserving_;
    size_t mean_jitter_;

    std::default_random_engine prng_;
    std::poisson_distribution<size_t> poisson_gen_;

    std::vector<DropTailPacketQueue*> internal_queues_ {};

    /*
    virtual const std::string & type( void ) const override
    {
        static const std::string type_ { "ecmp" };
        return type_;
    }
    */

public:
    ECMPPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;

    bool empty( void ) const override;

    unsigned int size_bytes( void ) const override;
    unsigned int size_packets( void ) const override;

    void set_bdp( int bytes ) override;

    std::string to_string( void ) const override;

    static unsigned int get_arg( const std::string & args, const std::string & name );
};

#endif /* ECMP_PACKET_QUEUE_HH */ 
