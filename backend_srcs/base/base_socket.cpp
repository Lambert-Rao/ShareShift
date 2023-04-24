//
// Created by lambert on 23-4-17.
//

#include "base_socket.h"
#include "event_dispatch.h"

#include <utility>
#include <sys/ioctl.h>
#include <bits/fcntl-linux.h>
#include <sys/fcntl.h>
#include <netinet/tcp.h>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>


/*-------------------SocketMap-------------------*/
using SocketMap = std::map<int, std::shared_ptr<XBaseSocket>>;
static SocketMap g_base_socket_map;
//TODO 存疑 key: socket fd, value: XBaseSocket
static void AddBaseSocket(XBaseSocket* psocket) {
  g_base_socket_map[psocket->GetSocket()] = std::make_shared<XBaseSocket>(*psocket);
}
static void AddBaseSocket(std::shared_ptr<XBaseSocket>psocket) {
  g_base_socket_map[psocket->GetSocket()] = psocket;
}
static void RemoveBaseSocket(XBaseSocket* psocket) {
  g_base_socket_map.erase((psocket->GetSocket()));
}
std::shared_ptr<XBaseSocket> FindBaseSocket(int fd) {
  auto iter = g_base_socket_map.find(fd);
  if (iter != g_base_socket_map.end()) {
    return std::make_shared<XBaseSocket>(*(iter->second));
  } else
    return nullptr;
}

/*-------------------XBaseSocket-------------------*/
XBaseSocket::XBaseSocket() {
  socket_ = kInvalidSocket;
  state_ = SocketState::IDLE;
}
XBaseSocket::~XBaseSocket() = default;

void XBaseSocket::SetSendBufSize(uint32_t send_size) const {
  int ret = setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size));
  if (ret == kSocketError) {
    printf("set SO_SNDBUF failed for fd=%d", socket_);
  }
  socklen_t len = sizeof(send_size);
  int size = 0;
  getsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &size, &len);
  printf("socket=%d send_buf_size=%d", socket_, size);
}
void XBaseSocket::SetRecvBufSize(uint32_t recv_size) const {
  int ret = setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size));
  if (ret == kSocketError) {
    printf("set SO_RCVBUF failed for fd=%d", socket_);
  }

  socklen_t len = sizeof(recv_size);
  int size = 0;
  getsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &size, &len);
  printf("socket=%d recv_buf_size=%d", socket_, size);
}

//TODO callback_?
int XBaseSocket::Listen(const char *server_ip, uint16_t port) {
  local_ip_ = server_ip;
  local_port_ = port;

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == kInvalidSocket) {
    printf("socket failed, err_code=%d, server_ip=%s, port=%u",
           errno, server_ip, port);
    return kNetLibError;
  }

  SetReuseAddr(socket_);
  SetNonBlock(socket_);

  sockaddr_in server_addr{};
  SetAddr(server_ip, port, &server_addr);
  int ret = bind(socket_, (sockaddr *) &server_addr, sizeof(server_addr));
  if (ret == kSocketError) {
    printf("bind failed, err_code=%d, server_ip=%s, port=%u",
           errno, server_ip, port);
    close(socket_);
    return kNetLibError;
  }

  ret = listen(socket_, 64);
  if (ret == kSocketError) {
    printf("listen failed, err_code=%d, server_ip=%s, port=%u",
           errno, server_ip, port);
    close(socket_);
    return kNetLibError;
  }

  state_ = SocketState::LISTENING;
  printf("XBaseSocket::Listen on %s:%d", server_ip, port);

  AddBaseSocket(this);
  XEventDispatch::Instance()->AddEvent(socket_);
}

int XBaseSocket::Connect(const char *server_ip, uint16_t port) {
  printf("XBaseSocket::Connect to %s:%d", server_ip, port);

  remote_ip_ = server_ip;
  remote_port_ = port;

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == kInvalidSocket) {
    printf("socket failed, err_code=%d, server_ip=%s, port=%u",
           errno, server_ip, port);
    return kInvalidHandle;
  }

  SetNonBlock(socket_);
  SetNoDelay(socket_);

  sockaddr_in server_addr{};
  SetAddr(server_ip, port, &server_addr);
  int ret = connect(socket_, (sockaddr *) &server_addr, sizeof(server_addr));
  if (ret == kSocketError && !IsBlock()) {
    printf("connect failed, err_code=%d, server_ip=%s, port=%u",
           errno, server_ip, port);
    close(socket_);
    return kInvalidHandle;
  }
  state_ = SocketState::CONNECTING;
  AddBaseSocket(this);
  XEventDispatch::Instance()->AddEvent(socket_);

  return socket_;
}

int XBaseSocket::Send(void *buf, int len) {
  if (state_ != SocketState::CONNECTED) {
    return kNetLibError;
  }

  int ret = send(socket_, (char *) buf, len, 0);
  if (ret == kSocketError) {
    if (IsBlock())ret = 0;
    else printf("send failed, err_code=%d, socket=%d", errno, socket_);
  }
}

int XBaseSocket::Recv(void *buf, int len) {
  return recv(socket_, (char *) buf, len, 0);
}

int XBaseSocket::Close() {
  XEventDispatch::Instance()->RemoveEvent(socket_);
  RemoveBaseSocket(this);
  close(socket_);
  XBaseSocket::~XBaseSocket();
  return 0;
}

//OnRead() is called when socket is readable in XEventDispatch::Dispatch()
void XBaseSocket::OnRead() {
  if (state_ == SocketState::LISTENING) {//if the socket is listening, accept new socket
    AcceptNewSocket();
  } else {//socket is already connected, read data
    u_long avail = 0;// available data size in socket buffer
    int ret = ioctl(socket_, FIONREAD, &avail);//get available data size
    if (ret == kSocketError || avail == 0) {//socket error or no data
      callback_(NetLibMsg::CLOSE, socket_);//use the callback_
    } else {
      callback_(NetLibMsg::READ, socket_);
    }
  }
}

void XBaseSocket::OnWrite() {
  if (state_ == SocketState::CONNECTED) {
    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(socket_, SOL_SOCKET, SO_ERROR, &error, &len);
    if (error)
      callback_(NetLibMsg::CLOSE, socket_);
    else {
      state_ = SocketState::CONNECTED;
      callback_(NetLibMsg::WRITE, socket_);
    }
  } else
    callback_(NetLibMsg::WRITE, socket_);
}

void XBaseSocket::OnClose() {
  state_ = SocketState::CLOSING;
  callback_(NetLibMsg::CLOSE, socket_);
}
void XBaseSocket::SetNonBlock(int fd) {
  int ret = fcntl(fd, F_SETFL, O_NONBLOCK | fcntl(fd, F_GETFL));
  if(ret == kSocketError)
    printf("set nonblock failed, err_code=%d, fd=%d", errno, fd);
}

void XBaseSocket::SetReuseAddr(int fd) {
    int opt = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(ret == kSocketError)
        printf("set reuseaddr failed, err_code=%d, fd=%d", errno, fd);
}

void XBaseSocket::SetNoDelay(int fd) {
    int opt = 1;
    int ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    if(ret == kSocketError)
        printf("set nodelay failed, err_code=%d, fd=%d", errno, fd);
}

void XBaseSocket::SetAddr(const char *ip, const uint16_t port, sockaddr_in *addr) {
  memset(addr, 0, sizeof(sockaddr_in));
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = inet_addr(ip);
  if (addr->sin_addr.s_addr == INADDR_NONE) {
    hostent *host = gethostbyname(ip);
    if (host == NULL) {
      printf("gethostbyname failed, ip=%s, port=%u", ip, port);
      return;
    }

    addr->sin_addr.s_addr = *(uint32_t *)host->h_addr;
  }
}

void XBaseSocket::AcceptNewSocket() {
  int fd = 0;
  sockaddr_in peer_addr{};
    socklen_t addr_len = sizeof(sockaddr_in);
    char ip_str[64];
  while ((fd = accept(socket_, (sockaddr *)&peer_addr, &addr_len)) !=
      kInvalidSocket) {
    auto p_socket = std::make_shared<XBaseSocket>();
    uint32_t ip = ntohl(peer_addr.sin_addr.s_addr);
    uint16_t port = ntohs(peer_addr.sin_port);
    //convert ip to string
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip >> 24,
             (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);


    p_socket->SetSocket(fd);
    p_socket->SetCallback(callback_);
    p_socket->SetState(SocketState::CONNECTED);
    p_socket->SetRemoteIp(ip_str);
    p_socket->SetRemotePort(port);

    SetNoDelay(fd);
    SetNonBlock(fd);
    AddBaseSocket(p_socket);
    XEventDispatch::Instance()->AddEvent(fd);
    callback_(NetLibMsg::CONNECT, fd);
  }
}