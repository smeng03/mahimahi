#ifndef FAIR_PACKET_QUEUE_HH
#define FAIR_PACKET_QUEUE_HH

#include "abstract_packet_queue.hh"
#include "drop_tail_packet_queue.hh"
#include "exception.hh"

class FairPacketQueue : public AbstractPacketQueue
{
private:
    const std::string& args;
    size_t num_queues_;

    std::vector<DropTailPacketQueue*> internal_queues_ {};
    size_t curr_queue_;

public:
    FairPacketQueue( const std::string & args );

    void enqueue( QueuedPacket && p ) override;

    QueuedPacket dequeue( void ) override;

    bool empty( void ) const override;

    unsigned int size_bytes( void ) const override;
    unsigned int size_packets( void ) const override;

    void set_bdp( int bytes ) override;

    std::string to_string( void ) const override;

    static unsigned int get_arg( const std::string & args, const std::string & name );
};

#endif
