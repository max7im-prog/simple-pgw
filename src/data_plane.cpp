#include <data_plane.h>

data_plane::data_plane(control_plane &control_plane) : _control_plane(control_plane) {}

void data_plane::handle_uplink(uint32_t dp_teid, Packet &&packet) {
    auto bearer = _control_plane.find_bearer_by_dp_teid(dp_teid);
    if (bearer == nullptr) {
        return;
    }
    auto connection = bearer->get_pdn_connection();
    if (connection == nullptr) {
        return;
    }
    auto apn_addr = connection->get_apn_gw();
    forward_packet_to_apn(apn_addr, std::move(packet));
}

void data_plane::handle_downlink(const boost::asio::ip::address_v4 &ue_ip, Packet &&packet) {
    auto connection = _control_plane.find_pdn_by_ip_address(ue_ip);
    if (connection == nullptr) {
        return;
    }
    auto bearer = connection->get_default_bearer();
    if (bearer == nullptr) {
        return;
    }
    auto sgw_addr = connection->get_sgw_address();
    uint32_t sgw_dp_teid = bearer->get_sgw_dp_teid();
    forward_packet_to_sgw(sgw_addr, sgw_dp_teid, std::move(packet));
}
