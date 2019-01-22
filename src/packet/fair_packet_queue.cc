#include <iostream>

#include "exception.hh"
#include "fair_packet_queue.hh"
#include "ezio.hh"

FairPacketQueue::FairPacketQueue(const std::string& args_)
  : args(args_),
    num_queues_((size_t) get_arg(args, "queues")),
    curr_queue_(0)
{
    if (num_queues_ == 0) {
        throw std::runtime_error( "Fair queue must have some number of queues" );
    }

    for (size_t i = 0; i < num_queues_; i++) {
        internal_queues_.push_back(new DropTailPacketQueue(args));
    }

    assert( internal_queues_.size() == num_queues_ );

    curr_queue_ = 0;
}

inline void hash_flow( size_t *qid, const uint32_t *p, size_t num_buckets ) {
    (*qid) = (*p) % num_buckets;
}

void FairPacketQueue::enqueue(QueuedPacket&& p) {
    size_t qid;
    hash_flow(&qid, (uint32_t *) p.contents.substr(24,4).c_str(), num_queues_);

    internal_queues_[qid]->enqueue((QueuedPacket &&) p);
}

// can assume nonempty
QueuedPacket FairPacketQueue::dequeue(void) {
    assert( not empty() );

    bool did_dequeue = false;
    while (!did_dequeue) {
        curr_queue_ = (curr_queue_ + 1) % num_queues_;
        if (!internal_queues_[curr_queue_]->empty()) {
            return internal_queues_[curr_queue_]->dequeue();
        }
    }

    assert( false ); // unreachable!()
}

void FairPacketQueue::set_bdp( int bytes )
{
    for (size_t i = 0; i < internal_queues_.size(); i++) {
        internal_queues_[i]->set_bdp(bytes);
    }
}

unsigned int FairPacketQueue::size_bytes(void) const {
    unsigned int ret = 0;

    for (size_t i = 0; i < internal_queues_.size(); i++) {
        ret += internal_queues_[i]->size_bytes();
    }

    return ret;
}

unsigned int FairPacketQueue::size_packets(void) const {
    unsigned int ret = 0;

    for (size_t i = 0; i < internal_queues_.size(); i++) {
        ret += internal_queues_[i]->size_packets();
    }
    
    return ret;
}

bool FairPacketQueue::empty(void) const {
    for (size_t i = 0; i < internal_queues_.size(); i++) {
        if (!internal_queues_[i]->empty()) {
            return false;
        }
    }

    return true;
}

std::string FairPacketQueue::to_string( void ) const
{
    std::string ret = "fq {";
    for (size_t i = 0; i < internal_queues_.size(); i++) {
        ret += internal_queues_[i]->to_string();
    }

    ret += "]";

    return ret;
}

unsigned int FairPacketQueue::get_arg( const std::string & args, const std::string & name )
{
    auto offset = args.find( name );
    if ( offset == std::string::npos ) {
        return 0; /* default value */
    } else {
        /* extract the value */

        /* advance by length of name */
        offset += name.size();

        /* make sure next char is "=" */
        if ( args.substr( offset, 1 ) != "=" ) {
            throw std::runtime_error( "could not parse queue arguments: " + args );
        }

        /* advance by length of "=" */
        offset++;

        /* find the first non-digit character */
        auto offset2 = args.substr( offset ).find_first_not_of( "0123456789" );

        auto digit_string = args.substr( offset ).substr( 0, offset2 );

        if ( digit_string.empty() ) {
            throw std::runtime_error( "could not parse queue arguments: " + args );
        }

        return myatoi( digit_string );
    }
}
