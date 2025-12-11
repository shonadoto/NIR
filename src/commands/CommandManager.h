#pragma once

#include <QObject>
#include <memory>
#include <vector>

#include "commands/Command.h"

/**
 * @brief Manages command history for Undo/Redo functionality.
 *
 * This class maintains a history of executed commands and provides
 * undo/redo capabilities. Commands are stored in a vector with a
 * current index pointing to the last executed command.
 */
class CommandManager : public QObject {
  Q_OBJECT

 public:
  explicit CommandManager(QObject* parent = nullptr);
  ~CommandManager() override = default;

  /**
   * @brief Execute a command and add it to history.
   * @param command Command to execute (ownership is transferred).
   * @return true if command executed successfully, false otherwise.
   *
   * If the command executes successfully, it's added to history.
   * Any commands after current_index_ are discarded (user did redo,
   * then executed a new command).
   */
  auto execute(std::unique_ptr<Command> command) -> bool;

  /**
   * @brief Undo the last executed command.
   * @return true if undo was successful, false otherwise.
   */
  auto undo() -> bool;

  /**
   * @brief Redo the last undone command.
   * @return true if redo was successful, false otherwise.
   */
  auto redo() -> bool;

  /**
   * @brief Check if undo is available.
   * @return true if there are commands to undo.
   */
  [[nodiscard]] auto can_undo() const -> bool;

  /**
   * @brief Check if redo is available.
   * @return true if there are commands to redo.
   */
  [[nodiscard]] auto can_redo() const -> bool;

  /**
   * @brief Clear all command history.
   */
  void clear();

  /**
   * @brief Get the description of the next command to undo.
   * @return Description string, or empty if no undo available.
   */
  [[nodiscard]] auto undo_description() const -> std::string;

  /**
   * @brief Get the description of the next command to redo.
   * @return Description string, or empty if no redo available.
   */
  [[nodiscard]] auto redo_description() const -> std::string;

  /**
   * @brief Set maximum history size (0 = unlimited).
   * @param size Maximum number of commands to keep in history.
   */
  void set_max_history_size(size_t size) {
    max_history_size_ = size;
  }

  /**
   * @brief Get current history size.
   * @return Number of commands in history.
   */
  [[nodiscard]] auto history_size() const -> size_t {
    return history_.size();
  }

 signals:
  /**
   * @brief Emitted when undo/redo availability changes.
   */
  void history_changed();

 private:
  void trim_history();

  std::vector<std::unique_ptr<Command>> history_;
  size_t current_index_{0};
  size_t max_history_size_{100};  // Default: keep last 100 commands
};
