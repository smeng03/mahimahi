/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <random>

#include "ecmp_packet_queue.hh"
#include "drop_tail_packet_queue.hh"
#include "timestamp.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

ECMPPacketQueue::ECMPPacketQueue( const string & args )
    : num_queues_      ((size_t) get_arg(args, "queues")),
      curr_queue_      (0),
      qlen_bytes_      (0),
      qlen_pkts_       (0),
      work_conserving_ ((bool) get_arg(args, "nonworkconserving") == 0),
      mean_jitter_     ((size_t) get_arg(args, "mean_jitter")),
      prng_( random_device()() ),
      poisson_gen_( get_arg(args, "mean_jitter") )
{
    if (num_queues_ == 0) {
        throw runtime_error( "ECMP queue must have > 0 queues" );
    }

    for (size_t i=0; i < num_queues_; i++) {
        internal_queues_.push_back(new DropTailPacketQueue(args));
    }
    
    assert( internal_queues_.size() == num_queues_ );

    curr_queue_ = 0;
    default_random_engine generator;
}

inline uint32_t simple_hash(const char *str, size_t len) {
    uint32_t hash = 5381;
    size_t i=0;
    int c;
    while (i < len) {
        c = *str++;
        hash = ((hash << 5) + hash) + c;
        i++;
    }
    return hash; 
}

typedef uint64_t Fnv64_t;
/*
 * 64 bit magic FNV-0 and FNV-1 prime
 */
#define FNV1_64_INIT ((Fnv64_t)0xcbf29ce484222325ULL)
#define FNV_64_PRIME ((Fnv64_t)0x100000001b3ULL)

Fnv64_t
fnv_64_buf(const void *buf, size_t len, Fnv64_t hval)
{
    size_t i;
    uint64_t v;
    uint8_t *b = (uint8_t*) buf;
    for (i = 0; i < len; i++) {
        v = (uint64_t) b[i];
        hval = hval ^ v;
        hval *= FNV_64_PRIME;
    }

    return hval;
}

#define FIVE_TUPLE_START 24
#define FIVE_TUPLE_LEN 4

inline size_t hash_flow(const char *pkt) {
    //return simple_hash(pkt, FIVE_TUPLE_LEN);
    return fnv_64_buf(pkt, FIVE_TUPLE_LEN, FNV1_64_INIT);
}

void ECMPPacketQueue::enqueue(QueuedPacket &&p ) {

    size_t hash;
    if(p.contents.size() < 28) { 
        hash = 1;
    } else {
        hash = hash_flow(p.contents.substr(FIVE_TUPLE_START, FIVE_TUPLE_LEN).c_str());
    }
    size_t qid = hash % num_queues_;

    qlen_bytes_ += p.contents.size();
    qlen_pkts_++;

    //cerr << "enqueue hash=" << hash << " q=" << qid << " qlen=" << qlen_pkts_ << endl;
    
    internal_queues_[qid]->enqueue((QueuedPacket &&) p);
}

QueuedPacket ECMPPacketQueue::dequeue( void )
{
    QueuedPacket ret = QueuedPacket("", 0);
    const uint64_t now = timestamp();

    size_t i = 0;
    while (i < num_queues_) {
        DropTailPacketQueue *q = internal_queues_[(curr_queue_ + i) % num_queues_];
        if (!q->empty()) {
            if (mean_jitter_ > 0 && (now - q->peek().arrival_time) >= poisson_gen_(prng_)) {
                ret = q->dequeue(); 
                qlen_bytes_ -= ret.contents.size();
                qlen_pkts_--;
                i++;
                break;
            }
        }
        if (!work_conserving_) {
            i++;
            break;
        }
        i++;
    }
    curr_queue_ = (curr_queue_ + i) % num_queues_;

    return ret;
}

void ECMPPacketQueue::set_bdp( int bytes )
{
    for (size_t i=0; i < num_queues_; i++) {
        internal_queues_[i]->set_bdp(bytes);
    }
}

unsigned int ECMPPacketQueue::size_bytes( void ) const
{
    return qlen_bytes_;
}

unsigned int ECMPPacketQueue::size_packets( void ) const
{
    return qlen_pkts_;
}
bool ECMPPacketQueue::empty( void ) const
{
    if (qlen_bytes_ == 0) {
        assert( qlen_pkts_ == 0 );
        return true;
    } else {

    }
    return false;
}

std::string ECMPPacketQueue::to_string( void ) const
{
    std::string ret = "ecmp {";

    for (size_t i=0; i < num_queues_; i++) {
        ret += internal_queues_[i]->to_string();
    }

    ret += "}";

    return ret;
}

unsigned int ECMPPacketQueue::get_arg( const string & args, const string & name )
{
    auto offset = args.find( name );
    if ( offset == string::npos ) {
        return 0; // default value 
    } else {
        // extract the value

        // advance by length of name 
        offset += name.size();

        // make sure next char is "=" 
        if ( args.substr( offset, 1 ) != "=" ) {
            throw runtime_error( "could not parse queue arguments: " + args );
        }

        // advance by length of "=" 
        offset++;

        // find the first non-digit character 
        auto offset2 = args.substr( offset ).find_first_not_of( "0123456789" );

        auto digit_string = args.substr( offset ).substr( 0, offset2 );

        if ( digit_string.empty() ) {
            throw runtime_error( "could not parse queue arguments: " + args );
        }

        return myatoi( digit_string );
    }
}

/*
QueuedPacket DroppingPacketQueue::dequeue( void )
{
    assert( not internal_queue_.empty() );

    QueuedPacket ret = std::move( internal_queue_.front() );
    internal_queue_.pop();

    queue_size_in_bytes_ -= ret.contents.size();
    queue_size_in_packets_--;

    assert( good() );

    return ret;
}

bool DroppingPacketQueue::empty( void ) const
{
    return internal_queue_.empty();
}

bool DroppingPacketQueue::good_with( const unsigned int size_in_bytes,
                                     const unsigned int size_in_packets ) const
{
    bool ret = true;

    if ( byte_limit_ ) {
        ret &= ( size_in_bytes <= byte_limit_ );
    }

    if ( bdp_byte_limit_ ) {
        ret &= ( size_in_bytes <= bdp_byte_limit_ );
    }

    if ( packet_limit_ ) {
        ret &= ( size_in_packets <= packet_limit_ );
    }

    return ret;
}

bool DroppingPacketQueue::good( void ) const
{
    return good_with( size_bytes(), size_packets() );
}

unsigned int DroppingPacketQueue::size_bytes( void ) const
{
    assert( queue_size_in_bytes_ >= 0 );
    return unsigned( queue_size_in_bytes_ );
}

void DroppingPacketQueue::set_bdp( int bytes )
{
    bdp_byte_limit_ = (bytes * bdp_limit_);
}

unsigned int DroppingPacketQueue::size_packets( void ) const
{
    assert( queue_size_in_packets_ >= 0 );
    return unsigned( queue_size_in_packets_ );
}

void DroppingPacketQueue::accept( QueuedPacket && p )
{
    queue_size_in_bytes_ += p.contents.size();
    queue_size_in_packets_++;
    internal_queue_.emplace( std::move( p ) );
}

string DroppingPacketQueue::to_string( void ) const
{
    string ret = type() + " [";

    if ( byte_limit_ ) {
        ret += string( "bytes=" ) + ::to_string( byte_limit_ );
    }

    if ( packet_limit_ ) {
        if ( byte_limit_ ) {
            ret += ", ";
        }

        ret += string( "packets=" ) + ::to_string( packet_limit_ );
    }

    ret += "]";

    return ret;
}

*/


