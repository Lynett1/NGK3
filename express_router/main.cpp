#include <iostream>
#include <string>
#include <vector>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>
#include <restinio/websocket/websocket.hpp>
#include <chrono>    

using namespace std; // Skabte problemer

// Definition af strukturen for Sted (Place)
struct place_t
{
    string m_name; // Navn
    double m_lat;  // Lat
    double m_lon;  // Lon

    place_t() = default;

    place_t(string name, double lat, double lon)
        : m_name{move(name)}, m_lat{lat}, m_lon{lon}
    {}

    template <typename JSON_IO>
    void json_io(JSON_IO &io)
    {
        io & json_dto::mandatory("Navn", m_name)
           & json_dto::mandatory("Lat", m_lat)
           & json_dto::mandatory("Lon", m_lon);
    }
};

// Definition af strukturen for Dato og Tid (DateTime)
struct dateTime_t
{
    string m_date; // Dato
    string m_time; // Klokkeslæt

    dateTime_t() = default;

    dateTime_t(string date, string time)
        : m_date{move(date)}, m_time{move(time)}
    {}

    template <typename JSON_IO>
    void json_io(JSON_IO &io)
    {
        io & json_dto::mandatory("Dato", m_date)
           & json_dto::mandatory("Klokkeslæt", m_time);
    }

    bool operator<(const dateTime_t& other) const {
        if (m_date != other.m_date) {
            return m_date < other.m_date; // Sammenlign dato først
        }
        return m_time < other.m_time; // Hvis ens sammenlign klokkeslæt
    }
    bool operator==(const dateTime_t& other) const {
        return m_date == other.m_date && m_time == other.m_time;
    }
};

// Definition af strukturen for Vejrudsigt (Weathercast data)
struct weathercast_t
{
    string m_id;
    dateTime_t m_dateTime;   // Dato og tid
    place_t m_place;         // Sted
    double m_temperature;    // Temperatur
    int m_humidity;          // Luftfugtighed

    weathercast_t() = default;

    weathercast_t(
        string id,
        dateTime_t dateTime,
        place_t place,
        double temperature,
        int humidity)
        : m_id{move(id)},
          m_dateTime{move(dateTime)},
          m_place{move(place)},
          m_temperature{temperature},
          m_humidity{humidity}
    {}

    template <typename JSON_IO>
    void json_io(JSON_IO &io)
    {
        io & json_dto::optional("ID", m_id, "")
           & json_dto::mandatory("Tidspunkt (dato og klokkeslæt)", m_dateTime)
           & json_dto::mandatory("Sted", m_place)
           & json_dto::mandatory("Temperatur", m_temperature)
           & json_dto::mandatory("Luftfugtighed", m_humidity);
    }
};

namespace rr = restinio::router;
namespace rws = restinio::websocket::basic;
using router_t = rr::express_router_t<>;

using ws_registry_t = std::map<std::uint64_t, rws::ws_handle_t>; // Definer WebSocket registry


class weather_handler_t
{
public:
    explicit weather_handler_t(vector<weathercast_t> &weather_data)
        : m_weather_data(weather_data)
        , m_next_id(1) // Initialiser ID
    {
        // Sikre unikt ID
        if (!m_weather_data.empty()) {
            int max_id = 0;
            for (const auto& wc : m_weather_data) {
                try {
                    int current_id = stoi(wc.m_id);
                    if (current_id > max_id) {
                        max_id = current_id;
                    }
                } catch (const exception& e) {
                    
				}
            }
            m_next_id = max_id + 1;
        }
    }

    weather_handler_t(const weather_handler_t &) = delete;
    weather_handler_t(weather_handler_t &&) = delete;

    // Til et unikt ID
    string generate_unique_id() {
        return to_string(m_next_id++);
    }

    // GET ALL
    auto on_get_all_weather(
        const restinio::request_handle_t& req, rr::route_params_t) const
    {
        auto resp = init_json_resp(req->create_response());
        resp.set_body(json_dto::to_json(m_weather_data));
        return resp.done();
    }

    // GET ID
    auto on_get_weather_by_id(
        const restinio::request_handle_t& req, rr::route_params_t params) const
    {
        auto resp = init_json_resp(req->create_response());
        const auto id = params["id"]; 

        auto it = find_if(m_weather_data.begin(), m_weather_data.end(),
                               [&](const weathercast_t& wc){ return wc.m_id == id; });

        if (it != m_weather_data.end()) {
            resp.set_body(json_dto::to_json(*it)); 
        } else {
            // Hvis ID ikke findes, fejlkode 404
            return req->create_response(restinio::status_not_found())
                       .set_body(R"({"error": "Vejrdata med angivet ID blev ikke fundet"})")
                       .done();
        }
        return resp.done();
    }

    // GET DATE
    auto on_get_weather_by_date(
        const restinio::request_handle_t& req, rr::route_params_t params) const
    {
        auto resp = init_json_resp(req->create_response());
        const auto date_str = params["date"]; 

        vector<weathercast_t> result;
        for (const auto& wc : m_weather_data) {
            string stored_date_normalized = wc.m_dateTime.m_date;
            stored_date_normalized.erase(
                remove(stored_date_normalized.begin(), stored_date_normalized.end(), '.'),
                stored_date_normalized.end());

            if (stored_date_normalized == date_str) {
                result.push_back(wc); 
            }
        }
        resp.set_body(json_dto::to_json(result));
        return resp.done();
    }

    // GET LATEST_THREE
    auto on_get_latest_three(
        const restinio::request_handle_t& req, rr::route_params_t ) const
    {
        auto resp = init_json_resp(req->create_response());

        if (m_weather_data.empty()) {
            resp.set_body("[]");
            return resp.done();
        }
        vector<weathercast_t> latest_three_uploaded;
        
        size_t start_index = 0;
        if (m_weather_data.size() > 3) {
            start_index = m_weather_data.size() - 3;
        }

        for (size_t i = start_index; i < m_weather_data.size(); ++i) {
            latest_three_uploaded.push_back(m_weather_data[i]);
        }

        resp.set_body(json_dto::to_json(latest_three_uploaded));
        return resp.done();
    }

    // POST
    auto on_post_weather(
        const restinio::request_handle_t& req, rr::route_params_t )
    {
        try {
            weathercast_t new_weather = json_dto::from_json<weathercast_t>(req->body());

            auto it = find_if(m_weather_data.begin(), m_weather_data.end(),
                [&](const weathercast_t& wc) {
                    return wc.m_dateTime == new_weather.m_dateTime;
                });

            if (it != m_weather_data.end()) {
                return req->create_response(restinio::status_conflict()) // 409 Conflict
                           .set_body(R"({"error": "En vejrudsigt med dette tidspunkt eksisterer allerede."})")
                           .done();
            }

            new_weather.m_id = generate_unique_id(); 

            m_weather_data.push_back(new_weather); 
            sendMessage(json_dto::to_json(new_weather)); // opdaterer WebSocket

            auto resp = init_json_resp(req->create_response(restinio::status_created()));
            resp.set_body(json_dto::to_json(new_weather)); 
            return resp.done();
        } catch (const exception& ex) {
            return req->create_response(restinio::status_bad_request())
                       .set_body(string("Fejl ved parsning af JSON: ") + ex.what())
                       .done();
        }
    }

    // PUT ID
    auto on_put_weather(
        const restinio::request_handle_t& req, rr::route_params_t params)
    {
        const auto id_to_update = params["id"]; 

        try {
            weathercast_t updated_data = json_dto::from_json<weathercast_t>(req->body());

            auto it = find_if(m_weather_data.begin(), m_weather_data.end(),
                                   [&](const weathercast_t& wc){ return wc.m_id == id_to_update; });

            if (it != m_weather_data.end()) {
                it->m_dateTime = updated_data.m_dateTime;
                it->m_place = updated_data.m_place;
                it->m_temperature = updated_data.m_temperature;
                it->m_humidity = updated_data.m_humidity;

                sendMessage(json_dto::to_json(*it)); // opdaterer WebSocket

                auto resp = init_json_resp(req->create_response(restinio::status_ok()));
                resp.set_body(json_dto::to_json(*it)); 
                return resp.done();
            } else {
                // Fejlkode 404 hvis ID ikke findes
                return req->create_response(restinio::status_not_found())
                           .set_body(R"({"error": "Vejrdata med angivet ID blev ikke fundet til opdatering"})")
                           .done();
            }
        } catch (const exception& ex) {
            return req->create_response(restinio::status_bad_request())
                       .set_body(string("Fejl ved parsning af JSON til opdatering: ") + ex.what())
                       .done();
        }
    }

    // Root
    auto on_root_get(
        const restinio::request_handle_t& req, rr::route_params_t ) const
    {
        auto resp = init_json_resp(req->create_response(restinio::status_ok()));
        resp.set_body(R"({"message": "Velkommen til Vejr API'et! Tilgå /weather for alle data, /weather/:id for specifikt ID, /weather/date/:date for data på dato, /latest_three for de seneste tre. Brug POST på /weather og PUT på /weather/:id."})");
        return resp.done();
    }

    auto on_live_update(const restinio::request_handle_t &req, rr::route_params_t)
    {
        if (restinio::http_connection_header_t::upgrade ==
            req->header().connection())
        {
            auto wsh = rws::upgrade<
                restinio::traits_t<
                    restinio::asio_timer_manager_t,
                    restinio::single_threaded_ostream_logger_t,
                    router_t
                >
            >(
                *req, rws::activation_t::immediate,
                [this](auto wsh_in, auto m)
                {
                    if (rws::opcode_t::text_frame == m->opcode() ||
                        rws::opcode_t::binary_frame == m->opcode() ||
                        rws::opcode_t::continuation_frame == m->opcode())
                    {
                        wsh_in->send_message(*m);
                    }
                    else if (rws::opcode_t::ping_frame == m->opcode())
                    {
                        auto resp = *m;
                        resp.set_opcode(rws::opcode_t::pong_frame);
                        wsh_in->send_message(resp);
                    }
                    else if (rws::opcode_t::connection_close_frame == m->opcode())
                    {
                        m_registry.erase(wsh_in->connection_id());
                    }
                });
            m_registry.emplace(wsh->connection_id(), wsh);
            init_json_resp(req->create_response()).done();
            return restinio::request_accepted();
        }
        return restinio::request_rejected(); // WebSocket upgrade tjek
    }

private:
    vector<weathercast_t> &m_weather_data; 
    int m_next_id; 
    ws_registry_t m_registry;

    template <typename RESP>
    static RESP
    init_json_resp(RESP resp)
    {
        resp.append_header("Server", "RESTinio Weather API /v.0.2") 
            .append_header_date_field()
            .append_header("Content-Type", "application/json; charset=utf-8")
            .append_header("Access-Control-Allow-Origin", "*"); 
        return resp;
    }

    void sendMessage(std::string message)
    {
        for (auto const& [id, ws_handle] : m_registry) {
            ws_handle->send_message(rws::final_frame, rws::opcode_t::text_frame, message);
        }
    }
};

auto server_handler(vector<weathercast_t> &weather_data_ref)
{
    auto router = std::make_unique<router_t>();
    auto handler = std::make_shared<weather_handler_t>(std::ref(weather_data_ref));

    auto by = [&](auto method) {
        using namespace placeholders;
        return bind(method, handler, _1, _2);
    };
    // CORS for /weather
    router->add_handler(
        restinio::http_method_options(),
        "/weather",
        [](const restinio::request_handle_t& req, rr::route_params_t ) {
            return req->create_response(restinio::status_ok())
                .append_header("Access-Control-Allow-Origin", "*")
                .append_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS") 
                .append_header("Access-Control-Allow-Headers", "Content-Type")
                .append_header("Access-Control-Max-Age", "86400") 
                .done();
        });
    // CORS for /weather/:id
    router->add_handler(
        restinio::http_method_options(),
        R"(/weather/:id([0-9]+))",
        [](const restinio::request_handle_t& req, rr::route_params_t ) {
            return req->create_response(restinio::status_ok())
                .append_header("Access-Control-Allow-Origin", "*")
                .append_header("Access-Control-Allow-Methods", "GET, PUT, OPTIONS")
                .append_header("Access-Control-Allow-Headers", "Content-Type")
                .append_header("Access-Control-Max-Age", "86400")
                .done();
        });
    // CORS for /weather/live (WebSocket)
    router->add_handler(
        restinio::http_method_options(),
        "/weather/live",
        [](const restinio::request_handle_t& req, rr::route_params_t ) {
            return req->create_response(restinio::status_ok())
                .append_header("Access-Control-Allow-Origin", "*")
                .append_header("Access-Control-Allow-Methods", "GET, OPTIONS") // WebSocket upgrade uses GET
                .append_header("Access-Control-Allow-Headers", "Content-Type, Upgrade, Connection")
                .append_header("Access-Control-Max-Age", "86400")
                .done();
        });

    router->http_get("/weather", by(&weather_handler_t::on_get_all_weather));
    router->http_get("/", by(&weather_handler_t::on_root_get));

    // LAB 2 ruter, for de nye krav
    router->http_get(
        R"(/weather/:id([0-9]+))",
        by(&weather_handler_t::on_get_weather_by_id)
    );
    // GET /weather/date/:date
    router->http_get(
        R"(/weather/date/:date([0-9]{8}))",
        by(&weather_handler_t::on_get_weather_by_date)
    );
    // GET /weather/latest_three 
    router->http_get("/weather/latest_three", by(&weather_handler_t::on_get_latest_three));

    // POST /weather
    router->http_post("/weather", by(&weather_handler_t::on_post_weather));

    // PUT /weather/:id
    router->http_put(
        R"(/weather/:id([0-9]+))",
        by(&weather_handler_t::on_put_weather)
    );

    router->http_get("/weather/live", by(&weather_handler_t::on_live_update));  // WebSocket upgrade

    // Catch all for det der ikke håndteres, returnerer 405 Method Not Allowed med CORS-headere
    router->add_handler(
        restinio::router::none_of_methods(),
        ".*", // Match any route
        [](const restinio::request_handle_t& req, auto) {
            return req->create_response(restinio::status_method_not_allowed())
                .append_header("Content-Type", "application/json; charset=utf-8")
                .append_header("Access-Control-Allow-Origin", "*")
                .set_body(R"({"error": "Method not allowed"})")
                .done();
        }
    );

    // Catch all for det der ikke håndteres, returnerer 404 Not Found med CORS-headere
    router->non_matched_request_handler([](const restinio::request_handle_t& req) {
        return req->create_response(restinio::status_not_found())
            .append_header("Content-Type", "application/json; charset=utf-8")
            .append_header("Access-Control-Allow-Origin", "*")
            .set_body(R"({"error": "Route not found"})")
            .done();
    });

    return router;
}

int main()
{
    using namespace chrono;

    try
    {
        vector<weathercast_t> weather_data_storage;

        place_t aarhus_n_place{"Aarhus N", 56.17, 10.22};
        place_t copenhagen_place{"Risskov", 55.67, 12.56};

        weather_data_storage.push_back({
            "1",
            dateTime_t{"2024.04.15", "10:15"},
            aarhus_n_place,
            13.1,
            70
        });
        weather_data_storage.push_back({
            "2",
            dateTime_t{"2024.04.15", "11:30"},
            copenhagen_place,
            15.5,
            65
        });
        weather_data_storage.push_back({
            "3", 
            dateTime_t{"2024.04.16", "09:00"},
            aarhus_n_place,
            10.0,
            80
        });
        weather_data_storage.push_back({
            "4", 
            dateTime_t{"2024.04.16", "14:00"},
            copenhagen_place,
            12.8,
            75
        });
        weather_data_storage.push_back({
            "5",
            dateTime_t{"2024.04.17", "08:30"},
            aarhus_n_place,
            9.5,
            85
        });

        using traits_t =
            restinio::traits_t<
                restinio::asio_timer_manager_t,
                restinio::single_threaded_ostream_logger_t,
                router_t>;

        cout << "Starter server på localhost:8080..." << endl;
        restinio::run(
            restinio::on_this_thread<traits_t>()
                .address("localhost")
                .port(8080)
                .request_handler(server_handler(weather_data_storage))
                .read_next_http_message_timelimit(10s)
                .write_http_response_timelimit(1s)
                .handle_request_timeout(1s));
    }
    catch (const exception &ex)
    {
        cerr << "Fejl: " << ex.what() << endl;
        return 1;
    }

    return 0;
}