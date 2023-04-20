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
//TODO callback??
  template<typename...Args>
  void SetCallback(Args&&...args) { callback_ = std::bind(std::forward<Args>(args)...); }
  void SetRemoteIp(char *ip) { remote_ip_ = ip; }
  void SetRemotePort(uint16_t port) { remote_port_ = port; }
  void SetSendBufSize(uint32_t send_size);
  void SetRecvBufSize(uint32_t recv_size);

  [[nodiscard]] const char *GetRemoteIp() { return remote_ip_.c_str(); }
  [[nodiscard]] uint16_t GetRemotePort() const { return remote_port_; }
  [[nodiscard]] const char *GetLocalIp() { return local_ip_.c_str(); }
  [[nodiscard]] uint16_t GetLocalPort() const { return local_port_; }

 public:
  //TODO 这Listen怎么回事，callback？
  template<typename...Args>
  int Listen(const char *server_ip, uint16_t port, util::Callback callback,
             Args...args);
  template<typename...Args>
  int Connect(const char *server_ip, uint16_t port,
              util::Callback callback,Args...args);

  int Send(void *buf, int len);

  int Recv(void *buf, int len);

  int Close();

 public:
  void OnRead();
  void OnWrite();
  void OnClose();

 private: // 私有函数以_ 开头
  int GetErrorCode();
  bool IsBlock(int error_code);

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

  util::Callback callback_{};

  SocketState state_{};
  int socket_{};
};

XBaseSocket *FindBaseSocket(int fd);