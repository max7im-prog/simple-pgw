#include <control_plane.h>

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
    auto ret = pdn_connection::create(sgw_cp_teid, _apns[apn], sgw_addr);
    _pdns_by_ue_ip_addr[sgw_addr] = ret;
    _pdns[sgw_cp_teid] = ret;
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
    static uint32_t next_dp_teid = 1;

    uint32_t dp_teid = next_dp_teid++;
    auto bearer_ptr = std::make_shared<bearer>(dp_teid, *pdn);
    bearer_ptr->set_sgw_dp_teid(sgw_teid);

    _bearers[dp_teid] = bearer_ptr;
    pdn->add_bearer(bearer_ptr);

    if (!pdn->get_default_bearer()) {
        pdn->set_default_bearer(bearer_ptr);
    }

    return bearer_ptr;
}


void control_plane::delete_bearer(uint32_t dp_teid) { _bearers.erase(dp_teid); }
