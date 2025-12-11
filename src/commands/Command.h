#pragma once

#include <memory>
#include <string>

/**
 * @brief Base class for all commands in the Command Pattern.
 *
 * Commands encapsulate operations that can be executed and undone.
 * This enables Undo/Redo functionality throughout the application.
 */
class Command {
 public:
  virtual ~Command() = default;

  /**
   * @brief Execute the command.
   * @return true if command executed successfully, false otherwise.
   */
  virtual auto execute() -> bool = 0;

  /**
   * @brief Undo the command, restoring the previous state.
   * @return true if undo was successful, false otherwise.
   */
  virtual auto undo() -> bool = 0;

  /**
   * @brief Check if this command can be undone.
   * @return true if command can be undone, false otherwise.
   */
  [[nodiscard]] virtual auto can_undo() const -> bool {
    return true;
  }

  /**
   * @brief Get a description of the command for UI display.
   * @return Human-readable description of the command.
   */
  [[nodiscard]] virtual auto description() const -> std::string {
    return "Command";
  }

  /**
   * @brief Merge this command with another command if possible.
   * Used for combining multiple similar operations (e.g., moving an item).
   * @param other The command to merge with.
   * @return true if merge was successful, false otherwise.
   */
  virtual auto merge_with(const Command& other) -> bool {
    (void)other;
    return false;
  }
};
