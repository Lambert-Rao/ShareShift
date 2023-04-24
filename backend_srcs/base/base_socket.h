//
// Created by lambert on 23-4-17.
//

#include <sys/socket.h>
#include <netinet/in.h>

#include <utility>

#include "ss_util.h"

/*-------------------enum-------------------*/
enum SocketState {
  IDLE,
  LISTENING,
  CONNECTING,
  CONNECTED,
  CLOSING
};
/*-------------------XBaseSocket-------------------*/

class XBaseSocket {
 public:
  XBaseSocket();

  virtual ~XBaseSocket();

  [[nodiscard]] int GetSocket() const { return socket_; }
  void SetSocket(int fd) { socket_ = fd; }
  void SetState(SocketState state) { state_ = state; }
//TODO callback_? bind the Args?
  template <typename Func>
  void SetCallback(Func &&func) {
    callback_ = std::forward<Func>(func);
  }

  void SetRemoteIp(char *ip) { remote_ip_ = ip; }
  void SetRemotePort(uint16_t port) { remote_port_ = port; }
  void SetSendBufSize(uint32_t send_size) const;
  void SetRecvBufSize(uint32_t recv_size) const;

  [[nodiscard]] const char *GetRemoteIp() { return remote_ip_.c_str(); }
  [[nodiscard]] uint16_t GetRemotePort() const { return remote_port_; }
  [[nodiscard]] const char *GetLocalIp() { return local_ip_.c_str(); }
  [[nodiscard]] uint16_t GetLocalPort() const { return local_port_; }

 public:
  //TODO 这Listen怎么回事，callback_？
  int Listen(const char *server_ip, uint16_t port);
  int Connect(const char *server_ip, uint16_t port);

  int Send(void *buf, int len);

  int Recv(void *buf, int len);

  int Close();

 public:
  void OnRead();
  void OnWrite();
  void OnClose();

 private: // 私有函数以_ 开头
  bool IsBlock() { return ((errno == EINPROGRESS) || (errno == EWOULDBLOCK)); }

  void SetNonBlock(int fd);
  void SetReuseAddr(int fd);
  void SetNoDelay(int fd);
  void SetAddr(const char *ip, const uint16_t port, sockaddr_in *addr);

  void AcceptNewSocket();

 private:
  std::string remote_ip_{};
  uint16_t remote_port_{};
  std::string local_ip_{};
  uint16_t local_port_{};
  util::SocketCallback callback_{};

  SocketState state_{};
  int socket_{};
};

std::shared_ptr<XBaseSocket> FindBaseSocket(int fd);
