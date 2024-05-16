#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;

#include "dvr.h"

void link(std::shared_ptr<Node> node1, std::shared_ptr<Node> node2,
          int distance) {
  node1->link(node2, distance);
  node2->link(node1, distance);
}

int main() {
  auto node0 = Node::create();
  auto node1 = Node::create();
  auto node2 = Node::create();
  auto node3 = Node::create();

  link(node0, node1, 1);
  link(node1, node2, 1);
  link(node2, node3, 2);
  link(node3, node0, 2);

  std::this_thread::sleep_for(1ms);

  assert(node0->distance(node1) == 1);
  assert(node0->distance(node2) == 2);
  assert(node0->distance(node3) == 2);
  assert(node1->distance(node0) == 1);
  assert(node1->distance(node2) == 1);
  assert(node1->distance(node3) == 3);
  assert(node2->distance(node0) == 2);
  assert(node2->distance(node1) == 1);
  assert(node2->distance(node3) == 2);
  assert(node3->distance(node0) == 2);
  assert(node3->distance(node1) == 3);
  assert(node3->distance(node2) == 2);

  link(node0, node3, 7);
  link(node0, node2, 3);

  std::this_thread::sleep_for(1ms);

  assert(node0->distance(node1) == 1);
  assert(node0->distance(node2) == 2);
  assert(node0->distance(node3) == 4);
  assert(node1->distance(node0) == 1);
  assert(node1->distance(node2) == 1);
  assert(node1->distance(node3) == 3);
  assert(node2->distance(node0) == 2);
  assert(node2->distance(node1) == 1);
  assert(node2->distance(node3) == 2);
  assert(node3->distance(node0) == 4);
  assert(node3->distance(node1) == 3);
  assert(node3->distance(node2) == 2);
}
