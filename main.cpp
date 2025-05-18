#include <iostream>
#include <string>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>

// For convenience
using namespace std;

// Define the structure for Place (Sted)
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

// Define the structure for DateTime
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
};

// Define the structure for Weathercast data
struct weathercast_t
{
    string m_id;
    dateTime_t m_dateTime;  // Dato og tid
    place_t m_place;        // Sted
    double m_temperature;   // Temperatur
    int m_humidity;         // Luftfugtighed

    weathercast_t() = default;

    weathercast_t(
        string id,
        dateTime_t dateTime, // Corrected: comma instead of semicolon
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
        io & json_dto::mandatory("ID", m_id)
           & json_dto::mandatory("Tidspunkt (dato og klokkeslæt)", m_dateTime)
           & json_dto::mandatory("Sted", m_place)
           & json_dto::mandatory("Temperatur", m_temperature)
           & json_dto::mandatory("Luftfugtighed", m_humidity);
    }
};

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class weather_handler_t
{
public:
    explicit weather_handler_t(weathercast_t &weather_data)
        : m_weather_data(weather_data)
    {}

    weather_handler_t(const weather_handler_t &) = delete;
    weather_handler_t(weather_handler_t &&) = delete;

    auto on_weather_data_get(
        const restinio::request_handle_t& req, rr::route_params_t ) const
    {
        auto resp = init_json_resp(req->create_response());
        resp.set_body(json_dto::to_json(m_weather_data));
        return resp.done();
    }

    auto on_root_get(
        const restinio::request_handle_t& req, rr::route_params_t ) const
    {
        auto resp = init_json_resp(req->create_response(restinio::status_ok()));
        resp.set_body(R"({"message": "Welcome to the Weather API! Access /weather for data."})");
        return resp.done();
    }

private:
    weathercast_t &m_weather_data;

    template <typename RESP>
    static RESP
    init_json_resp(RESP resp)
    {
        resp.append_header("Server", "RESTinio Weather API /v.0.1")
            .append_header_date_field()
            .append_header("Content-Type", "application/json; charset=utf-8");
        return resp;
    }
};

auto server_handler(weathercast_t &weather_data_ref)
{
    auto router = std::make_unique<router_t>();
    auto handler = std::make_shared<weather_handler_t>(std::ref(weather_data_ref));

    auto by = [&](auto method) {
        using namespace std::placeholders;
        return std::bind(method, handler, _1, _2);
    };

    auto method_not_allowed = [](const auto &req, auto) {
        return req->create_response(restinio::status_method_not_allowed())
            .connection_close()
            .done();
    };

    // Handler for GET /weather
    router->http_get("/weather", by(&weather_handler_t::on_weather_data_get));
    // Disable all other methods for '/weather'
    router->add_handler(
        restinio::router::none_of_methods(restinio::http_method_get()),
        "/weather",
        method_not_allowed);

    // Handler for GET /
    router->http_get("/", by(&weather_handler_t::on_root_get));
    // Disable all other methods for '/'
    router->add_handler(
        restinio::router::none_of_methods(restinio::http_method_get()),
        "/",
        method_not_allowed);

    return router;
}

int main()
{
    using namespace chrono;

    try
    {
        // Hardcoded data
        place_t aarhus_n_place{"Aarhus N", 13.692, 19.438};
        dateTime_t current_date_time{"20240415", "10:15"};
        
        weathercast_t hardcoded_weather_data{
            "1",                     // ID
            current_date_time,       // DateTime object
            aarhus_n_place,          // Place
            13.1,                    // Temperature
            70                       // Humidity
        };

        using traits_t =
            restinio::traits_t<
                restinio::asio_timer_manager_t,
                restinio::single_threaded_ostream_logger_t,
                router_t>;

        restinio::run(
            restinio::on_this_thread<traits_t>()
                .address("localhost")
                .port(8080)
                .request_handler(server_handler(hardcoded_weather_data))
                .read_next_http_message_timelimit(10s)
                .write_http_response_timelimit(1s)
                .handle_request_timeout(1s));
    }
    catch (const exception &ex)
    {
        cerr << "Error: " << ex.what() << endl;
        return 1;
    }

    return 0;
}