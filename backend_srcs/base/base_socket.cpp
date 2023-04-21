//
// Created by lambert on 23-4-17.
//

#include "base_socket.h"
#include "event_dispatch.h"

#include <utility>

enum {
  SOCKET_READ = 0x1,
  SOCKET_WRITE = 0x2,
  SOCKET_EXCEP = 0x4,
  SOCKET_ALL = 0x7
};

/*-------------------SocketMap-------------------*/
using SocketMap = std::map<int, XBaseSocket *>;
static SocketMap g_base_socket_map;
//TODO 存疑 key: socket fd, value: XBaseSocket
static void AddBaseSocket(XBaseSocket *psocket) {
  g_base_socket_map[psocket->GetSocket()] = psocket;
}

/*-------------------XBaseSocket-------------------*/
XBaseSocket::XBaseSocket() {
  socket_ = kInvalidSocket;
  state_ = SocketState::IDLE;
}
XBaseSocket::~XBaseSocket() = default;

void XBaseSocket::SetSendBufSize(uint32_t send_size) {
  int ret = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size));
  if (ret == kSocketError) {
    printf("set SO_SNDBUF failed for fd=%d", socket_);
  }
}
void XBaseSocket::SetRecvBufSize(uint32_t recv_size) {
  int ret = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size));
  if (ret == kSocketError) {
    printf("set SO_RCVBUF failed for fd=%d", socket_);
  }

  socklen_t len = sizeof(recv_size);
  int size = 0;
  getsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &size, &len);
  printf("socket=%d recv_buf_size=%d", socket_, size);
}

//TODO callback?
int XBaseSocket::Listen(const char *server_ip, uint16_t port) {
  local_ip_ = server_ip;
  local_port_ = port;

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == kInvalidSocket) {
    printf("socket failed, err_code=%d, server_ip=%s, port=%u",
           GetErrorCode(), server_ip, port);
    return kNetLibError;
  }

  SetReuseAddr(socket_);
  SetNonBlock(socket_);

  sockaddr_in server_addr{};
  SetAddr(server_ip, port, &server_addr);
  int ret = bind(socket_, (sockaddr *) &server_addr, sizeof(server_addr));
  if(ret == kSocketError){
    printf("bind failed, err_code=%d, server_ip=%s, port=%u",
           GetErrorCode(), server_ip, port);
    close(socket_);
    return kNetLibError;
  }

  ret = listen(socket_, 64);
  if(ret == kSocketError){
    printf("listen failed, err_code=%d, server_ip=%s, port=%u",
           GetErrorCode(), server_ip, port);
    close(socket_);
    return kNetLibError;
  }

  state_ = SocketState::LISTENING;
  printf("XBaseSocket::Listen on %s:%d", server_ip, port);

  AddBaseSocket(this);
  XEventDispatch::Instance()->AddEvent(socket_, SOCKET_READ | SOCKET_EXCEP);
}


