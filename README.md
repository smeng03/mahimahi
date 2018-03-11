mahimahi: a web performance measurement toolkit

# Dependencies

```
sudo apt-get install autotools-dev autoconf libtool apache2 apache2-dev
protobuf-compiler libprotobuf-dev libssl-dev xcb libxcb-composite0-dev
libxcb-present-dev libcairo2-dev libpango1.0-dev
```

# Install

```
./autogen.sh && ./configure && make && sudo make install
```

# Experimental features in this fork

1. mm-link reports the BDP in bytes and packets on startup when a delayshell is
   nested inside
2. Dropping packet queues accept limits in BDP for convenience
3. Run mm-link --cbr [INT][K|M] to automatically generate and use a constant
   bit-rate trace file for any integer value of Kbps or Mbps
4. Record src and dst port of packets for distinguishing flows in plotting
5. Improved plotting script mm-graph (more customizable and able to show
   throughput for individual flows)
5. (Coming soon) live graphing over ssh (via a browser app)
