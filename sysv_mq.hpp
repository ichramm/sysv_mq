/*!
 * \file sysv_mq.hpp
 * \author ichramm
 */
#ifndef SYSV_MQ
#define SYSV_MQ

#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <stdexcept>
#include <string>

class sysv_mq {
  static constexpr size_t MSGBUF_SIZE = 512;

  struct msgbuf {
    long mtype;
    char mtext[MSGBUF_SIZE];
  };

  int msgqid_ = -1;

public:
  void open(size_t key) {
    msgqid_ = msgget(static_cast<key_t>(key), IPC_CREAT | 0600);
    if (msgqid_ < 0) {
      std::string error{strerror(errno)}; // TODO: Use strerror_r
      throw std::runtime_error(error);
    }
  }

  void close() { /* noop */
  }

  void send(const std::string &message) {
    if (msgqid_ < 0) {
      throw std::runtime_error("Queue not open");
    }

    if (message.length() >= MSGBUF_SIZE) {
      throw std::runtime_error("Message cannot be longer than 512 bytes");
    }

    msgbuf buf;
    memset(&buf, 0, sizeof(buf));
    buf.mtype = 1;
    memcpy(buf.mtext, &message[0], message.length());

    if (msgsnd(msgqid_, &buf, message.length(), 0) < 0) {
      std::string error{strerror(errno)}; // TODO: Use strerror_r
      throw std::runtime_error(error);
    }
  }

  std::string receive(bool wait = false) {
    if (msgqid_ < 0) {
      throw std::runtime_error("Queue not open");
    }

    msgbuf buf;
    memset(&buf, 0, sizeof(buf));

    if (msgrcv(msgqid_, &buf, MSGBUF_SIZE, 0, wait ? 0 : IPC_NOWAIT) < 0) {
      if (!wait && errno == ENOMSG) {
        return "";
      }
      std::string error{strerror(errno)}; // TODO: Use strerror_r
      throw std::runtime_error(error);
    }

    return std::string{buf.mtext};
  }

  void clear() {
    if (msgqid_ < 0) {
      throw std::runtime_error("Queue not open");
    }

    int res;

    do {
      msgbuf buf;
      res = msgrcv(msgqid_, &buf, sizeof(buf.mtext), 0, IPC_NOWAIT);
    } while (res >= 0);

    if (errno != ENOMSG) {
      std::string error{strerror(errno)}; // TODO: Use strerror_r
      throw std::runtime_error(error);
    }
  }

  void remove() {
    if (msgqid_ < 0) {
      throw std::runtime_error("Queue not open");
    }

    if (msgctl(msgqid_, IPC_RMID, NULL) < 0) {
      std::string error{strerror(errno)}; // TODO: Use strerror_r
      throw std::runtime_error(error);
    }
  }
};

#endif // SYSV_MQ
