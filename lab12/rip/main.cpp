#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

using namespace std::literals::chrono_literals;

#include "rip.h"

void link(std::shared_ptr<Node> node1, std::shared_ptr<Node> node2,
          int distance) {
    node1->link(node2, distance);
    node2->link(node1, distance);
}

int main() {
    std::map<std::string, std::shared_ptr<Node>> nodes;

    std::ifstream config("net.tsv");
    std::string header;
    std::getline(config, header);

    std::string fst_addr, snd_addr;
    int length;
    while (config >> fst_addr >> snd_addr >> length) {
        {
            std::lock_guard lock{cout_mutex};
            std::cout << "Received link between " << fst_addr << " and " << snd_addr << " with length=" << length << std::endl;
        }

        if (!nodes.contains(fst_addr))
            nodes[fst_addr] = Node::create(fst_addr);
        if (!nodes.contains(snd_addr))
            nodes[snd_addr] = Node::create(snd_addr);

        link(nodes[fst_addr], nodes[snd_addr], length);
    }

    std::this_thread::sleep_for(10ms);

    for (auto &[_, node] : nodes) {
        node->dump_table();
    }
}
