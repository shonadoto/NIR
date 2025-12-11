#include "commands/CommandManager.h"

#include <algorithm>
#include <string>

CommandManager::CommandManager(QObject* parent) : QObject(parent) {}

auto CommandManager::execute(std::unique_ptr<Command> command) -> bool {
  if (command == nullptr) {
    return false;
  }

  // Execute the command
  if (!command->execute()) {
    return false;
  }

  // Remove any commands after current_index_ (user did redo, then new command)
  if (current_index_ < history_.size()) {
    history_.erase(history_.begin() + static_cast<ptrdiff_t>(current_index_),
                   history_.end());
  }

  // Add command to history
  history_.push_back(std::move(command));
  current_index_ = history_.size();

  // Trim history if needed
  trim_history();

  emit history_changed();
  return true;
}

auto CommandManager::undo() -> bool {
  if (!can_undo()) {
    return false;
  }

  // Move back in history
  --current_index_;

  // Undo the command at current_index_
  if (history_[current_index_]->undo()) {
    emit history_changed();
    return true;
  }

  // If undo failed, move forward again
  ++current_index_;
  return false;
}

auto CommandManager::redo() -> bool {
  if (!can_redo()) {
    return false;
  }

  // Redo the command at current_index_
  if (history_[current_index_]->execute()) {
    ++current_index_;
    emit history_changed();
    return true;
  }

  return false;
}

auto CommandManager::can_undo() const -> bool {
  return current_index_ > 0;
}

auto CommandManager::can_redo() const -> bool {
  return current_index_ < history_.size();
}

void CommandManager::clear() {
  history_.clear();
  current_index_ = 0;
  emit history_changed();
}

auto CommandManager::undo_description() const -> std::string {
  if (!can_undo()) {
    return {};
  }
  return history_[current_index_ - 1]->description();
}

auto CommandManager::redo_description() const -> std::string {
  if (!can_redo()) {
    return {};
  }
  return history_[current_index_]->description();
}

void CommandManager::trim_history() {
  if (max_history_size_ == 0) {
    return;  // Unlimited
  }

  // If history exceeds max size, remove oldest commands
  while (history_.size() > max_history_size_) {
    // Remove from the beginning, but adjust current_index_
    history_.erase(history_.begin());
    if (current_index_ > 0) {
      --current_index_;
    }
  }
}
