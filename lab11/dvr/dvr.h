#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

struct Message {
  int from;
  std::map<int, int> distances;
};

class Node {
public:
  static std::shared_ptr<Node> create() {
    static std::atomic_int next_id_{0};
    return std::shared_ptr<Node>(new Node(next_id_.fetch_add(1)));
  }

  int id() { return id_; }

  void link(std::shared_ptr<Node> to, int distance) {
    mutex_.lock();
    adjacent_[to->id_] = std::weak_ptr(to);
    distance_vector_[id_] = 0;
    distance_vector_[to->id_] = distance;
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

private:
  Node(int id)
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
                for (auto &[node, distances] : adjacent_distances_) {
                  if (distances.contains(to)) {
                    new_value = std::min(new_value, edge_to_adjacent_[node] +
                                                        distances[to]);
                  }
                }
                if (new_value != old_value) {
                  update = true;
                  distance_vector_[to] = new_value;
                }
              } else {
                update = true;
                distance_vector_[to] = d + edge_to_adjacent_[message.from];
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

  int id_;
  std::map<int, int> distance_vector_;
  std::map<int, std::weak_ptr<Node>> adjacent_;
  std::map<int, int> edge_to_adjacent_;
  std::map<int, std::map<int, int>> adjacent_distances_;
  std::mutex mutex_;

  std::queue<Message> queue_;
  std::mutex queue_mutex_;
  std::condition_variable queue_not_empty_;

  std::jthread mainloop_;
  std::atomic_flag terminated_;
};
