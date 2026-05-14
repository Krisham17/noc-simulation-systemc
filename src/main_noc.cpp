// main_noc.cpp
#include "systemc.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <map>
#include <set>
#include "packet.h"
#include "source.h"
#include "sink.h"
#include "router.h"

std::string to_string_compat(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}


std::vector<int> generate_uniform_pattern(int num_cores) {
    std::vector<int> dest_ids(num_cores);
    for (int i = 0; i < num_cores; i++) dest_ids[i] = i;
    std::random_shuffle(dest_ids.begin(), dest_ids.end());
    return dest_ids;
}




enum SigType { SIG_SOURCE, SIG_OUTPUT, SIG_ZERO, SIG_SINK };

struct PortMapping {
    SigType type;
    int index;
};


enum AckType { ACK_SRC, ACK_IN, ACK_SINK, ACK_ZERO };

struct AckPortMapping {
    AckType type;
    int index;
};

const int NUM_ROUTERS = 16;
const int NUM_PORTS   = 5;


PortMapping routerIn[NUM_ROUTERS][NUM_PORTS] = {
  { {SIG_SOURCE,0}, {SIG_OUTPUT,2},  {SIG_OUTPUT,12}, {SIG_ZERO,1},  {SIG_ZERO,2} },
  { {SIG_SOURCE,1}, {SIG_OUTPUT,0},  {SIG_OUTPUT,7},  {SIG_OUTPUT,16}, {SIG_ZERO,5} },
  { {SIG_SOURCE,2}, {SIG_OUTPUT,4},  {SIG_OUTPUT,9},  {SIG_OUTPUT,17}, {SIG_ZERO,7} },
  { {SIG_SOURCE,3}, {SIG_OUTPUT,5},  {SIG_OUTPUT,21}, {SIG_ZERO,8},  {SIG_ZERO,9} },
  { {SIG_SOURCE,4}, {SIG_OUTPUT,1},  {SIG_OUTPUT,13}, {SIG_OUTPUT,26}, {SIG_ZERO,10} },
  { {SIG_SOURCE,5}, {SIG_OUTPUT,3},  {SIG_OUTPUT,10}, {SIG_OUTPUT,20}, {SIG_OUTPUT,30} },
  { {SIG_SOURCE,6}, {SIG_OUTPUT,6},  {SIG_OUTPUT,15}, {SIG_OUTPUT,22}, {SIG_OUTPUT,31} },
  { {SIG_SOURCE,7}, {SIG_OUTPUT,8},  {SIG_OUTPUT,18}, {SIG_OUTPUT,35}, {SIG_ZERO,11} },
  { {SIG_SOURCE,8}, {SIG_OUTPUT,11}, {SIG_OUTPUT,27}, {SIG_OUTPUT,38}, {SIG_ZERO,12} },
  { {SIG_SOURCE,9}, {SIG_OUTPUT,14}, {SIG_OUTPUT,24}, {SIG_OUTPUT,34}, {SIG_OUTPUT,41} },
  { {SIG_SOURCE,10},{SIG_OUTPUT,19}, {SIG_OUTPUT,29}, {SIG_OUTPUT,36}, {SIG_OUTPUT,43} },
  { {SIG_SOURCE,11},{SIG_OUTPUT,23}, {SIG_OUTPUT,32}, {SIG_OUTPUT,46}, {SIG_ZERO,13} },
  { {SIG_SOURCE,12},{SIG_OUTPUT,25}, {SIG_OUTPUT,40}, {SIG_ZERO,14}, {SIG_ZERO,15} },
  { {SIG_SOURCE,13},{SIG_OUTPUT,28}, {SIG_OUTPUT,39}, {SIG_OUTPUT,45}, {SIG_ZERO,16} },
  { {SIG_SOURCE,14},{SIG_OUTPUT,33}, {SIG_OUTPUT,42}, {SIG_OUTPUT,47}, {SIG_ZERO,17} },
  { {SIG_SOURCE,15},{SIG_OUTPUT,37}, {SIG_OUTPUT,44}, {SIG_ZERO,18}, {SIG_ZERO,19} }
};

PortMapping routerOut[NUM_ROUTERS][NUM_PORTS] = {
  { {SIG_SINK,0}, {SIG_ZERO,3},  {SIG_OUTPUT,0}, {SIG_OUTPUT,1}, {SIG_ZERO,4} },
  { {SIG_SINK,1}, {SIG_ZERO,6},  {SIG_OUTPUT,4}, {SIG_OUTPUT,3}, {SIG_OUTPUT,2} },
  { {SIG_SINK,2}, {SIG_ZERO,19}, {SIG_OUTPUT,5}, {SIG_OUTPUT,6}, {SIG_OUTPUT,7} },
  { {SIG_SINK,3}, {SIG_ZERO,21}, {SIG_ZERO,20},{SIG_OUTPUT,8}, {SIG_OUTPUT,9} },
  { {SIG_SINK,4}, {SIG_OUTPUT,12},{SIG_OUTPUT,10},{SIG_OUTPUT,11},{SIG_ZERO,22} },
  { {SIG_SINK,5}, {SIG_OUTPUT,16},{SIG_OUTPUT,15},{SIG_OUTPUT,14},{SIG_OUTPUT,13} },
  { {SIG_SINK,6}, {SIG_OUTPUT,17},{SIG_OUTPUT,18},{SIG_OUTPUT,19},{SIG_OUTPUT,20} },
  { {SIG_SINK,7}, {SIG_OUTPUT,21},{SIG_ZERO,23}, {SIG_OUTPUT,23},{SIG_OUTPUT,22} },
  { {SIG_SINK,8}, {SIG_OUTPUT,26},{SIG_OUTPUT,24},{SIG_OUTPUT,25},{SIG_ZERO,24} },
  { {SIG_SINK,9}, {SIG_OUTPUT,30},{SIG_OUTPUT,29},{SIG_OUTPUT,28},{SIG_OUTPUT,27} },
  { {SIG_SINK,10},{SIG_OUTPUT,31},{SIG_OUTPUT,32},{SIG_OUTPUT,33},{SIG_OUTPUT,34} },
  { {SIG_SINK,11},{SIG_OUTPUT,35},{SIG_ZERO,25}, {SIG_OUTPUT,37},{SIG_OUTPUT,36} },
  { {SIG_SINK,12},{SIG_OUTPUT,38},{SIG_OUTPUT,39},{SIG_ZERO,26},{SIG_ZERO,27} },
  { {SIG_SINK,13},{SIG_OUTPUT,41},{SIG_OUTPUT,42},{SIG_ZERO,28},{SIG_OUTPUT,40} },
  { {SIG_SINK,14},{SIG_OUTPUT,43},{SIG_OUTPUT,44},{SIG_ZERO,29},{SIG_OUTPUT,45} },
  { {SIG_SINK,15},{SIG_OUTPUT,46},{SIG_ZERO,30},{SIG_ZERO,31},{SIG_OUTPUT,47} }
};


AckPortMapping routerInAck[NUM_ROUTERS][NUM_PORTS] = {
  { {ACK_SINK,0}, {ACK_IN,2},  {ACK_IN,12}, {ACK_ZERO,1}, {ACK_ZERO,2} },
  { {ACK_SINK,1}, {ACK_IN,0},  {ACK_IN,7},  {ACK_IN,16}, {ACK_ZERO,5} },
  { {ACK_SINK,2}, {ACK_IN,4},  {ACK_IN,9},  {ACK_IN,17}, {ACK_ZERO,7} },
  { {ACK_SINK,3}, {ACK_IN,5},  {ACK_IN,21}, {ACK_ZERO,8}, {ACK_ZERO,9} },
  { {ACK_SINK,4}, {ACK_IN,1},  {ACK_IN,13}, {ACK_IN,26}, {ACK_ZERO,10} },
  { {ACK_SINK,5}, {ACK_IN,3},  {ACK_IN,10}, {ACK_IN,20}, {ACK_IN,30} },
  { {ACK_SINK,6}, {ACK_IN,6},  {ACK_IN,15}, {ACK_IN,22}, {ACK_IN,31} },
  { {ACK_SINK,7}, {ACK_IN,8},  {ACK_IN,18}, {ACK_IN,35}, {ACK_ZERO,11} },
  { {ACK_SINK,8}, {ACK_IN,11}, {ACK_IN,27}, {ACK_IN,38}, {ACK_ZERO,12} },
  { {ACK_SINK,9}, {ACK_IN,14}, {ACK_IN,24}, {ACK_IN,34}, {ACK_IN,41} },
  { {ACK_SINK,10},{ACK_IN,19}, {ACK_IN,29}, {ACK_IN,36}, {ACK_IN,43} },
  { {ACK_SINK,11},{ACK_IN,23}, {ACK_IN,32}, {ACK_IN,46}, {ACK_ZERO,13} },
  { {ACK_SINK,12},{ACK_IN,25}, {ACK_IN,40}, {ACK_ZERO,14}, {ACK_ZERO,15} },
  { {ACK_SINK,13},{ACK_IN,28}, {ACK_IN,39}, {ACK_IN,45}, {ACK_ZERO,16} },
  { {ACK_SINK,14},{ACK_IN,33}, {ACK_IN,42}, {ACK_IN,47}, {ACK_ZERO,17} },
  { {ACK_SINK,15},{ACK_IN,37}, {ACK_IN,44}, {ACK_ZERO,18}, {ACK_ZERO,19} }
};

AckPortMapping routerOutAck[NUM_ROUTERS][NUM_PORTS] = {
  { {ACK_SRC,0}, {ACK_ZERO,3}, {ACK_IN,0},  {ACK_IN,1},  {ACK_ZERO,4} },
  { {ACK_SRC,1}, {ACK_ZERO,6}, {ACK_IN,4},  {ACK_IN,3},  {ACK_IN,2} },
  { {ACK_SRC,2}, {ACK_ZERO,19},{ACK_IN,5},  {ACK_IN,6},  {ACK_IN,7} },
  { {ACK_SRC,3}, {ACK_ZERO,21},{ACK_ZERO,20},{ACK_IN,8},  {ACK_IN,9} },
  { {ACK_SRC,4}, {ACK_IN,12}, {ACK_IN,10}, {ACK_IN,11}, {ACK_ZERO,22} },
  { {ACK_SRC,5}, {ACK_IN,16}, {ACK_IN,15}, {ACK_IN,14}, {ACK_IN,13} },
  { {ACK_SRC,6}, {ACK_IN,17}, {ACK_IN,18}, {ACK_IN,19}, {ACK_IN,20} },
  { {ACK_SRC,7}, {ACK_IN,21}, {ACK_ZERO,23},{ACK_IN,23}, {ACK_IN,22} },
  { {ACK_SRC,8}, {ACK_IN,26}, {ACK_IN,24}, {ACK_IN,25}, {ACK_ZERO,24} },
  { {ACK_SRC,9}, {ACK_IN,30}, {ACK_IN,29}, {ACK_IN,28}, {ACK_IN,27} },
  { {ACK_SRC,10},{ACK_IN,31}, {ACK_IN,32}, {ACK_IN,33}, {ACK_IN,34} },
  { {ACK_SRC,11},{ACK_IN,35}, {ACK_ZERO,25},{ACK_IN,37}, {ACK_IN,36} },
  { {ACK_SRC,12},{ACK_IN,38}, {ACK_IN,39}, {ACK_ZERO,26},{ACK_ZERO,27} },
  { {ACK_SRC,13},{ACK_IN,41}, {ACK_IN,42}, {ACK_ZERO,28},{ACK_IN,40} },
  { {ACK_SRC,14},{ACK_IN,43}, {ACK_IN,44}, {ACK_ZERO,29},{ACK_IN,45} },
  { {ACK_SRC,15},{ACK_IN,46}, {ACK_ZERO,30},{ACK_ZERO,31},{ACK_IN,47} }
};


int sc_main(int argc, char* argv[]) {

    sc_vector< sc_signal<packet> > si_source("si_source", 16);
    sc_vector< sc_signal<packet> > si_sink("si_sink", 16);
    sc_vector< sc_signal<packet> > si_output("si_output", 64);
    sc_vector< sc_signal<packet> > si_zero("si_zero", 64);

    sc_vector< sc_signal<packet> > sioutput("sioutput", 16);

    sc_vector< sc_signal<bool> > si_ack_src("si_ack_src", 64);
    sc_vector< sc_signal<bool> > si_ack_in("si_ack_in", 64);
    sc_vector< sc_signal<bool> > si_ack_sink("si_ack_sink", 16);
    sc_vector< sc_signal<bool> > si_ack_zero("si_ack_zero", 64);

    sc_vector< sc_signal<sc_uint<4> > > siid("siid", 16);
    sc_vector< sc_signal<sc_uint<4> > > scid("scid", 16);
    sc_vector< sc_signal<sc_uint<4> > > id("id", 16);

    sc_signal<int> scinput;
    sc_signal<sc_uint<4> > check;

    sc_clock s_clock("S_CLOCK", 125, SC_NS, 0.5, 0.0, SC_NS);
    sc_clock r_clock("R_CLOCK", 5, SC_NS, 0.5, 10.0, SC_NS);
    sc_clock d_clock("D_CLOCK", 5, SC_NS, 0.5, 10.0, SC_NS);

    sc_vector<source> sources("sources", 16);
    sc_vector<router> routers("routers", 16);
    sc_vector<sink> sinks("sinks", 16);


    for (int i = 0; i < 16; i++) {
        std::string srcName = "source" + to_string_compat(i);
        sources[i].operator()(si_source[i], scid[i], si_ack_src[i], s_clock, scinput, check);
        scid[i].write(i);
    }

    for (int i = 0; i < 16; i++) {
        std::string sinkName = "sink" + to_string_compat(i);
        sinks[i].operator()(si_sink[i], si_ack_sink[i], siid[i], d_clock, sioutput[i]);
        siid[i].write(i);
    }


    for (int i = 0; i < 16; i++) {
        id[i].write(i);
    }


    for (int r = 0; r < NUM_ROUTERS; r++) {
        routers[r].router_id(id[r]);

        // Connect Data Input Ports 
        {
            PortMapping pm = routerIn[r][0];
            if (pm.type == SIG_SOURCE)
                routers[r].in0(si_source[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].in0(si_output[pm.index]);
            else 
                routers[r].in0(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerIn[r][1];
            if (pm.type == SIG_SOURCE)
                routers[r].in1(si_source[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].in1(si_output[pm.index]);
            else
                routers[r].in1(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerIn[r][2];
            if (pm.type == SIG_SOURCE)
                routers[r].in2(si_source[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].in2(si_output[pm.index]);
            else
                routers[r].in2(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerIn[r][3];
            if (pm.type == SIG_SOURCE)
                routers[r].in3(si_source[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].in3(si_output[pm.index]);
            else
                routers[r].in3(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerIn[r][4];
            if (pm.type == SIG_SOURCE)
                routers[r].in4(si_source[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].in4(si_output[pm.index]);
            else
                routers[r].in4(si_zero[pm.index]);
        }

        // Connect Data Output Ports 
        {
            PortMapping pm = routerOut[r][0];
            if (pm.type == SIG_SINK)
                routers[r].out0(si_sink[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].out0(si_output[pm.index]);
            else
                routers[r].out0(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerOut[r][1];
            if (pm.type == SIG_SINK)
                routers[r].out1(si_sink[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].out1(si_output[pm.index]);
            else
                routers[r].out1(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerOut[r][2];
            if (pm.type == SIG_SINK)
                routers[r].out2(si_sink[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].out2(si_output[pm.index]);
            else
                routers[r].out2(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerOut[r][3];
            if (pm.type == SIG_SINK)
                routers[r].out3(si_sink[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].out3(si_output[pm.index]);
            else
                routers[r].out3(si_zero[pm.index]);
        }
        {
            PortMapping pm = routerOut[r][4];
            if (pm.type == SIG_SINK)
                routers[r].out4(si_sink[pm.index]);
            else if (pm.type == SIG_OUTPUT)
                routers[r].out4(si_output[pm.index]);
            else
                routers[r].out4(si_zero[pm.index]);
        }

        // Connect Ack Input Ports 
        {
            AckPortMapping amp = routerInAck[r][0];
            if (amp.type == ACK_SINK)
                routers[r].inack0(si_ack_sink[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].inack0(si_ack_in[amp.index]);
            else 
                routers[r].inack0(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerInAck[r][1];
            if (amp.type == ACK_SINK)
                routers[r].inack1(si_ack_sink[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].inack1(si_ack_in[amp.index]);
            else
                routers[r].inack1(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerInAck[r][2];
            if (amp.type == ACK_SINK)
                routers[r].inack2(si_ack_sink[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].inack2(si_ack_in[amp.index]);
            else
                routers[r].inack2(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerInAck[r][3];
            if (amp.type == ACK_SINK)
                routers[r].inack3(si_ack_sink[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].inack3(si_ack_in[amp.index]);
            else
                routers[r].inack3(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerInAck[r][4];
            if (amp.type == ACK_SINK)
                routers[r].inack4(si_ack_sink[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].inack4(si_ack_in[amp.index]);
            else
                routers[r].inack4(si_ack_zero[amp.index]);
        }

        //  Connect Ack Output Ports 
        {
            AckPortMapping amp = routerOutAck[r][0];
            if (amp.type == ACK_SRC)
                routers[r].outack0(si_ack_src[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].outack0(si_ack_in[amp.index]);
            else
                routers[r].outack0(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerOutAck[r][1];
            if (amp.type == ACK_SRC)
                routers[r].outack1(si_ack_src[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].outack1(si_ack_in[amp.index]);
            else
                routers[r].outack1(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerOutAck[r][2];
            if (amp.type == ACK_SRC)
                routers[r].outack2(si_ack_src[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].outack2(si_ack_in[amp.index]);
            else
                routers[r].outack2(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerOutAck[r][3];
            if (amp.type == ACK_SRC)
                routers[r].outack3(si_ack_src[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].outack3(si_ack_in[amp.index]);
            else
                routers[r].outack3(si_ack_zero[amp.index]);
        }
        {
            AckPortMapping amp = routerOutAck[r][4];
            if (amp.type == ACK_SRC)
                routers[r].outack4(si_ack_src[amp.index]);
            else if (amp.type == ACK_IN)
                routers[r].outack4(si_ack_in[amp.index]);
            else
                routers[r].outack4(si_ack_zero[amp.index]);
        }

        routers[r].rclk(r_clock);
    }


    for (int i = 0; i < 16; i++) {
        siid[i].write(i);
        scid[i].write(i);
    }

    // Ask user for source and destination IDs
std::string pattern_choice;
    std::cout << "Enter traffic pattern (uniform-1/neighbor-0): ";
    std::cin >> pattern_choice;
    std::vector<int> traffic_map;

    std::srand(std::time(0));
    if (pattern_choice == "0") {
        //traffic_map = generate_neighboring_pattern(16);
    } else {
        traffic_map = generate_uniform_pattern(16);
    }



    sc_trace_file *tf = sc_create_vcd_trace_file("graph");
    sc_trace(tf, s_clock, "s_clock");
    sc_trace(tf, d_clock, "d_clock");
    sc_trace(tf, r_clock, "r_clock");
    for (int i = 0; i < 16; i++) {
        sc_trace(tf, si_source[i], ("si_source[" + to_string_compat(i) + "]").c_str());
        sc_trace(tf, si_sink[i], ("si_sink[" + to_string_compat(i) + "]").c_str());
    }

    std::cout << "\n-------------------------------------------------------------------------------" << std::endl;
    std::cout << "\n 4x4 Mesh NOC Simulator using Wormhole Routers" << std::endl;
    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    std::cout << "Press \"Return\" key to start the simulation..." << std::endl;
    getchar();
	for (int i = 0; i < 16; i++) {
    	check.write(i);              
    	scinput.write(traffic_map[i]);
    	sc_start(125, SC_NS);        
	}

    sc_close_vcd_trace_file(tf);

    std::cout << "\n-------------------------------------------------------------------------------" << std::endl;
	std::cout << "End of simulation..." << std::endl;
	std::cout << "----- Final Summary (Source vs. Sink) -----" << std::endl;
	for (int i = 0; i < 16; i++) {
    	std::cout << "Node " << i
    	          << ": Sent = " << sources[i].pkt_snt
    	          << ", Received = " << sinks[i].pkt_recv
    	          << std::endl;
	}	
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	std::cout << "Press \"Return\" key to end the simulation..." << std::endl;
	getchar();


    return 0;
}

