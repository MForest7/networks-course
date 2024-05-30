#pragma once

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

struct Message {
    std::string from;
    std::map<std::string, int> distances;
};

int log_entry_index;
std::mutex cout_mutex;

class Node {
public:
    static std::shared_ptr<Node> create(const std::string &ip) {
        static std::atomic_int next_id_{0};
        return std::shared_ptr<Node>(new Node(ip));
    }

    std::string id() { return id_; }

    void link(std::shared_ptr<Node> to, int distance) {
        mutex_.lock();
        adjacent_[to->id_] = std::weak_ptr(to);
        distance_vector_[id_] = 0;
        distance_vector_[to->id_] = distance;
        next_hop_[to->id_] = to->id_;
        edge_to_adjacent_[to->id_] = distance;
        adjacent_distances_[to->id_][id_] = distance;
        auto adjacent = snapshot_adjacent();
        auto message = Message{.from = id_, .distances = distance_vector_};
        mutex_.unlock();

        send_messages(adjacent, message);
    }

    void receive(Message message) {
        queue_mutex_.lock();
        queue_.push(message);
        queue_mutex_.unlock();
        queue_not_empty_.notify_one();
    }

    int distance(std::shared_ptr<Node> to) {
        return distance_vector_.at(to->id_);
    }

    ~Node() {
        terminated_.test_and_set();
        queue_not_empty_.notify_one();
    }

    void dump_table() {
        std::lock_guard lock{mutex_};
        std::lock_guard cout_lock{cout_mutex};
        std::cout << "Final state of router " << id_ << std::endl;
        std::cout << "         [Source IP]    [Destination IP]          [Next Hop]       [Metric]" << std::endl;
        for (auto &[to, dist] : distance_vector_) {
            std::cout << std::setw(20) << id_ << std::setw(20) << to << std::setw(20) << next_hop_[to] << std::setw(15) << dist << std::endl;
        }
    }

private:
    Node(const std::string &id)
        : id_(id), mainloop_([this] {
              while (true) {
                  std::unique_lock queue_lock{queue_mutex_};
                  if (queue_.empty()) {
                      queue_not_empty_.wait(queue_lock, [this] {
                          return terminated_.test() || !queue_.empty();
                      });
                  }

                  if (queue_.empty()) {
                      break;
                  }

                  auto message = queue_.front();
                  queue_.pop();
                  queue_lock.unlock();

                  std::unique_lock lock{mutex_};
                  if (!edge_to_adjacent_.contains(message.from)) {
                      lock.unlock();
                      continue;
                  }

                  bool update = false;
                  auto adjacent = snapshot_adjacent();

                  adjacent_distances_[message.from] = message.distances;
                  message.distances[message.from] = 0;
                  for (auto [to, d] : message.distances) {
                      if (to == id_)
                          continue;
                      if (distance_vector_.contains(to)) {
                          int old_value = distance_vector_[to];
                          int new_value = d + edge_to_adjacent_[message.from];
                          std::string next_hop = message.from;
                          for (auto &[node, distances] : adjacent_distances_) {
                              if (distances.contains(to) && new_value > edge_to_adjacent_[node] + distances[to]) {
                                  new_value = edge_to_adjacent_[node] + distances[to];
                                  next_hop = node;
                              }
                          }
                          if (new_value != old_value) {
                              update = true;
                              distance_vector_[to] = new_value;
                              next_hop_[to] = next_hop;
                          }
                      } else {
                          update = true;
                          distance_vector_[to] = d + edge_to_adjacent_[message.from];
                          next_hop_[to] = message.from;
                      }
                  }

                  {
                      std::lock_guard cout_lock{cout_mutex};
                      std::cout << "Step " << ++log_entry_index << " router " << id_ << std::endl;
                      std::cout << "         [Source IP]    [Destination IP]          [Next Hop]       [Metric]" << std::endl;
                      for (auto &[to, dist] : distance_vector_) {
                          std::cout << std::setw(20) << id_ << std::setw(20) << to << std::setw(20) << next_hop_[to] << std::setw(15) << dist << std::endl;
                      }
                  }

                  auto new_message =
                      Message{.from = id_, .distances = distance_vector_};
                  lock.unlock();

                  if (update) {
                      send_messages(adjacent, new_message);
                  }
              }
          }) {}

    std::vector<std::weak_ptr<Node>> snapshot_adjacent() {
        std::vector<std::weak_ptr<Node>> messages;
        for (auto &[_, ptr] : adjacent_) {
            messages.emplace_back(ptr);
        }
        return messages;
    }

    void send_messages(std::vector<std::weak_ptr<Node>> to, Message message) {
        for (auto ptr : to) {
            if (auto lock = ptr.lock(); lock) {
                lock->receive(message);
            }
        }
    }

    std::string id_;
    std::map<std::string, int> distance_vector_;
    std::map<std::string, std::string> next_hop_;
    std::map<std::string, std::weak_ptr<Node>> adjacent_;
    std::map<std::string, int> edge_to_adjacent_;
    std::map<std::string, std::map<std::string, int>> adjacent_distances_;
    std::mutex mutex_;

    std::queue<Message> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_not_empty_;

    std::jthread mainloop_;
    std::atomic_flag terminated_;
};
