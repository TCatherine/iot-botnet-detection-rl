#include "sim.h"
#include "nlohmann/json.hpp"

// for convenience
using json = nlohmann::json;

static const std::string path_config = "../../agents/config.json";
uint16_t number_of_bots, number_of_clients, number_of_iot;
config clear_traffic;
config ddos_traffic;

void parse() {
    std::ifstream ifs(path_config);
    json jf = json::parse(ifs);
    json general_info = jf["GeneralInfo"];

    number_of_bots = general_info.value("Number Bots", 0);
    number_of_iot = general_info.value("Number Agents", 0);
    number_of_clients = general_info.value("Number Clients", 0);

    reward_tp = general_info["Reward"].value("TP", 0);
    reward_tn = general_info["Reward"].value("TN", 0);
    reward_fp = general_info["Reward"].value("FP", 0);
    reward_fn = general_info["Reward"].value("FN", 0);

    json data = jf["Config Clear Traffic"]["DataRate"];
    for (auto it = data.begin(); it != data.end(); it++){
        clear_traffic.data_rate.push_back(it.value());
    }

    data = jf["Config Clear Traffic"]["PacketSize"];
    for (auto it = data.begin(); it != data.end(); it++){
        clear_traffic.packet_size.push_back(it.value());
    }

    data = jf["Config Clear Traffic"]["Delay"];
    for (auto it = data.begin(); it != data.end(); it++){
        clear_traffic.delay.push_back(it.value());
    }

    data = jf["Config Clear Traffic"]["Number Packets"];
    for (auto it = data.begin(); it != data.end(); it++){
        clear_traffic.n_packets.push_back(it.value());
    }

    data = jf["Config DDOS"]["DataRate"];
    for (auto it = data.begin(); it != data.end(); it++){
        ddos_traffic.data_rate.push_back(it.value());
    }

    data = jf["Config DDOS"]["PacketSize"];
    for (auto it = data.begin(); it != data.end(); it++){
        ddos_traffic.packet_size.push_back(it.value());
    }

    data = jf["Config DDOS"]["Delay"];
    for (auto it = data.begin(); it != data.end(); it++){
        ddos_traffic.delay.push_back(it.value());
    }

    data = jf["Config DDOS"]["Number Packets"];
    for (auto it = data.begin(); it != data.end(); it++){
        ddos_traffic.n_packets.push_back(it.value());
    }

}