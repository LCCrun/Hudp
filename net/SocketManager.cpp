#include <cstring>		//for memset
#include "SocketManager.h"
#include "NetMsg.h"
#include "Socket.h"
#include "Hudp.h"

using namespace hudp;

CSocketManager::CSocketManager() {

}

CSocketManager::~CSocketManager() {
    _socket_map.clear();
}

NetMsg* CSocketManager::GetMsg() {
    std::unique_lock<std::mutex> lock(_mutex);
    _notify.wait(_mutex, [this]() {return this->_have_msg_socket.size() != 0; });

    auto iter = _have_msg_socket.begin();
    auto msg = _socket_map[*iter]->GetMsgFromPriQueue();
    _have_msg_socket.erase(iter);
    return msg;
}

void CSocketManager::GetSendSocket(const HudpHandle& handle, std::shared_ptr<CSocket>& socket) {
    std::unique_lock<std::mutex> lock(_mutex);
    socket = GetSocket(handle);
}

bool CSocketManager::GetRecvSocket(const HudpHandle& handle, uint16_t flag, std::shared_ptr<CSocket>& socket) {
    // if a normal udp, send to upper direct.
    if (flag & HTF_NORMAL) {
        return false;
    }

    // add to order list.
    std::unique_lock<std::mutex> lock(_mutex);
    socket = GetSocket(handle);
    return true;
}

void CSocketManager::Destroy(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter == _socket_map.end()) {
        return;
    }

    // there should add msg to notify remote side destroy too.
    iter->second.reset();
}

std::shared_ptr<CSocket> CSocketManager::GetSocket(const HudpHandle& handle) {
    auto iter = _socket_map.find(handle);
    if (iter != _socket_map.end()) {
        return iter->second;
    }
    
    std::shared_ptr<CSocket> socket = std::make_shared<CSocket>();
    _socket_map[handle] = socket;
    return socket;
}