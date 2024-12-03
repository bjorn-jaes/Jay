//
// Copyright (c) 2022 Bjørn Fuglestad, Jaersense AS (bjorn@jaersense.no)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/bjorn-jaes/jay
//

#ifndef J1939_CONNECTION_H
#define J1939_CONNECTION_H

#pragma once

// C++
#include <functional>
#include <queue>
#include <vector>

// Libraries
#include "boost/asio.hpp"

#include "canary/filter.hpp"
#include "canary/raw.hpp"

#include "jay/frame.hpp"
#include "jay/network.hpp"

// Local

/**
 * J1939 Connection analog for reading and sending j1939 messages
 *
 * Callbacks are used to singal start on end of connection.
 * Incomming data is also passed along using a callbacks.
 * Outgoing can frames are also queued before being sent.
 * @note The connection manages its own lifetime
 * @todo look into using different types of queues to store
 * outgoing buffers such as timed queues and so on
 */
class j1939_connection : public std::enable_shared_from_this<j1939_connection>
{
public:
  /**
   * @brief Struct containing callbacks for J1939Connection
   */
  struct callbacks
  {
    // Alias
    using on_self_t = std::function<void(j1939_connection *)>;
    using on_error_t = std::function<void(const std::string, const boost::system::error_code)>;
    using on_frame_t = std::function<void(jay::frame)>;

    /**
     * @brief Callback for when connection is stated
     * @note is optional
     */
    on_self_t on_start;

    /**
     * @brief Callback for when connection is destroyed
     * @note is optional
     */
    on_self_t on_destroy;

    /**
     * @brief Callback for when data is recieved
     * @note is required
     */
    on_frame_t on_read;

    /**
     * @brief Callback for when data is sent
     * @note is optional
     */
    on_frame_t on_send;

    /**
     * @brief Callback for when an error occurs
     *
     * Constains a string indicating where the error happened and
     * an error code detailing the error.
     * @note is required
     */
    on_error_t on_error;
  };

  /**
   * @brief Construct a new Can Connection object
   *
   * @param io_context for performing async io operation
   * @param network containing address name pairs
   */
  j1939_connection(boost::asio::io_context &io_context, const jay::network &network);

  /**
   * @brief Construct a new J1939Connection object
   *
   * @param io_context for performing async io operation
   * @param network containing address name pairs
   * @param callbacks for generated events
   */
  j1939_connection(boost::asio::io_context &io_context, const jay::network &network, callbacks &&callbacks);

  /**
   * @brief Construct a new J1939Connection object
   *
   * @param io_context for performing async io operation
   * @param network containing address name pairs
   * @param callbacks for generated events
   * @param local_name that this connection is sending messages from
   * @param target_name that this connection is sending messages to
   */
  j1939_connection(boost::asio::io_context &io_context,
    const jay::network &network,
    callbacks &&callbacks,
    std::optional<jay::name> local_name,
    std::optional<jay::name> target_name);

  /**
   * @brief Destroy the J1939Connection object
   */
  ~j1939_connection();

  /**
   * @brief Open an j1939 endpoint
   * @param endpoint to open
   * @param filters for incomming j1939 messages
   * @return true if opened endpoint
   * @return false if failed to open endpoint
   */
  [[nodiscard]] bool open(const std::vector<canary::filter> &filters);

  /**
   * Listen for incomming j1939 frames
   */
  void start();

  /// ##################### Set/Get ##################### ///

  inline void bind_callbacks(const callbacks &callbacks) noexcept { callbacks_ = callbacks; }

  /**
   * @brief Set the Callbacks object
   * @param callbacks
   */
  inline void bind_callbacks(callbacks &&callbacks) noexcept { callbacks_ = std::move(callbacks); }

  /**
   * @brief Set the local j1939 name
   * @param name of the device this connection is sending
   * messages from, used for setting source address in messages
   */
  inline void local_name(const jay::name name) noexcept { local_name_ = name; }

  /**
   * @brief Set the Target Name object
   * @param name of the device this connection is sending
   * messages to, used for setting destination address in messages
   */
  inline void target_name(const jay::name name) noexcept { target_name_ = name; }

  /**
   * Get local name on this connection
   * @return optional name, is null_opt if none was set
   */
  [[nodiscard]] inline std::optional<jay::name> local_name() const noexcept { return local_name_; }

  /**
   * Get target name on this connection
   * @return optional name, is null_opt if none was set
   */
  [[nodiscard]] inline std::optional<jay::name> target_name() const noexcept { return target_name_; }

  /**
   * @brief Get the Network reference
   * @return jay::network&
   */
  [[nodiscard]] inline const jay::network &network() const noexcept { return network_; }


  /// ##################### WRITE ##################### ///

  /**
   * Send a frame to socket without any checks
   * @param j1939_frame that will be sent
   */
  void send_raw(const jay::frame &j1939_frame);

  /**
   * Send a broadcast frame to the socket
   * @param j1939_frame that will be broadcast, the source address
   * is set by the socket
   * @throw std::invalid_argument if frame does not contain a
   * broadcast PDU_S or there is if the socket does not have
   * and address
   * are not available
   */
  void broadcast(jay::frame &j1939_frame);

  /**
   * Send frame to connected controller application
   * @param j1939_frame that will be sent, both source address
   * and PDU specifier is set by the socket
   * @return true if written, false if an error occured
   * @throw std::invalid_argument if no connected controller
   * app name has been set
   */
  void send(jay::frame &j1939_frame);

  /**
   * Send frame to specific controller application
   * @param destination - name of the controller application to send to
   * @param j1939_frame that will be sent, both source address
   * and PDU specifier is set by the socket
   * @return true if written, false if an error occured
   * @throw std::invalid_argument if source and destination addresses
   * are not available
   */
  void send_to(const jay::name destination, jay::frame &j1939_frame);

private:
  /**
   * @brief Called when an event failes
   * @param what failed
   * @param error_code for the error
   */
  void on_error(char const *what, boost::system::error_code error_code);

  /**
   * Read data from socket
   */
  void read();

  /**
   * Write frames from qeueu to socket
   */
  void write();

  /**
   * @brief Check if the read frame is from a valid address
   * given the local and target name
   *
   * @return true if frame can be accepted
   * @return false if frame should be discarded
   */
  [[nodiscard]] bool validate_address() const;

private:
  // Injected

  canary::raw::socket socket_; /**< raw CAN-bus socket */
  const jay::network &network_; /**< Network reference for querying network for addresses */
  callbacks callbacks_; /**< Callbacks for generated events */

  std::optional<jay::name> local_name_{}; /**< Optional local j1939 name */
  std::optional<jay::name> target_name_{}; /**< Optional targeted j1939 name */

  // Internal

  jay::frame buffer_{}; /**< Incomming frame buffer */
  std::queue<jay::frame> queue_{}; /**< Outgoing  frame queue */
};

#endif