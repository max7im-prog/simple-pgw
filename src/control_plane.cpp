#include <control_plane.h>
#include <random>

uint32_t control_plane::generate_teid() {
    static std::random_device dev;
    static std::mt19937 rnd(dev());
    static std::uniform_int_distribution<uint32_t> distribution(1, UINT32_MAX - 1);
    return distribution(rnd);
}

boost::asio::ip::address_v4 control_plane::generate_ue_id() {
    static std::random_device dev;
    static std::mt19937 rnd(dev());
    static std::uniform_int_distribution<uint8_t> distribution(1, 255);
    boost::asio::ip::address_v4 ret({10, distribution(rnd), distribution(rnd), distribution(rnd)});
    return ret;
}


void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns[apn_name] = apn_gateway;
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    auto it = _pdns.find(cp_teid);
    if (it != _pdns.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4 &ip) const {
    auto it = _pdns_by_ue_ip_addr.find(ip);
    if (it != _pdns_by_ue_ip_addr.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    auto it = _bearers.find(dp_teid);
    if (it != _bearers.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string &apn,
                                                                     boost::asio::ip::address_v4 sgw_addr,
                                                                     uint32_t sgw_cp_teid) {
    uint32_t cp_teid;
    boost::asio::ip::address_v4 ue_ip_addr;

    if (_apns.find(apn) == _apns.end()) {
        return nullptr;
    }

    do {
        cp_teid = generate_teid();
    } while (_pdns.find(cp_teid) != _pdns.end());

    do {
        ue_ip_addr = generate_ue_id();
    } while (_pdns_by_ue_ip_addr.find(ue_ip_addr) != _pdns_by_ue_ip_addr.end());

    auto ret = pdn_connection::create(cp_teid, _apns[apn], ue_ip_addr);
    ret->set_sgw_addr(sgw_addr);
    ret->set_sgw_cp_teid(sgw_cp_teid);
    _pdns_by_ue_ip_addr[ue_ip_addr] = ret;
    _pdns[cp_teid] = ret;
    return ret;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    auto it = _pdns.find(cp_teid);
    if (it != _pdns.end()) {
        auto addr = it->second->get_ue_ip_addr();
        for (const auto &[dp_teid, bearer_ptr]: it->second->_bearers) {
            _bearers.erase(dp_teid);
        }
        _pdns.erase(cp_teid);
        _pdns_by_ue_ip_addr.erase(addr);
    }
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection> &pdn, uint32_t sgw_teid) {
    uint32_t dp_teid = generate_teid();
    do {
        dp_teid = generate_teid();
    } while (_bearers.find(dp_teid) != _bearers.end());
    auto bearer_ptr = std::make_shared<bearer>(dp_teid, *pdn);
    bearer_ptr->set_sgw_dp_teid(sgw_teid);
    _bearers[dp_teid] = bearer_ptr;
    pdn->add_bearer(bearer_ptr);
    return bearer_ptr;
}


void control_plane::delete_bearer(uint32_t dp_teid) {
    auto it = _bearers.find(dp_teid);
    if (it == _bearers.end()) {
        return;
    }
    auto bearer_ptr = it->second;
    auto connection = bearer_ptr->get_pdn_connection();
    if (connection != nullptr) {
        connection->remove_bearer(dp_teid);
    }
    _bearers.erase(dp_teid);
}
