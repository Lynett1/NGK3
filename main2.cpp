#include <iostream>
#include <string>
#include <vector>     // Til lagring af flere weathercast-poster
#include <algorithm>  // Til std::sort og std::find_if
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>
#include <chrono>     // For robust dato/tidsbehandling, selvom strengsammenligning bruges for nu

// For nemheds skyld
using namespace std;

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

// Definition af strukturen for DatoTid (DateTime)
struct dateTime_t
{
    string m_date; // Dato (f.eks. "YYYYMMDD")
    string m_time; // Klokkeslæt (f.eks. "HH:MM")

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

    // Hjælper til sortering/sammenligning af DateTime-objekter
    // Bruges til at finde de "seneste" poster
    bool operator<(const dateTime_t& other) const {
        if (m_date != other.m_date) {
            return m_date < other.m_date; // Sammenlign datoer først
        }
        return m_time < other.m_time; // Hvis datoer er ens, sammenlign klokkeslæt
    }
    bool operator==(const dateTime_t& other) const {
        return m_date == other.m_date && m_time == other.m_time;
    }
};

// Definition af strukturen for Vejrudsigt (Weathercast data)
struct weathercast_t
{
    string m_id;
    dateTime_t m_dateTime;    // Dato og tid
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
    // Konstruktør: Tager en reference til en vektor af vejrdata for at gemme dem
    explicit weather_handler_t(std::vector<weathercast_t> &weather_data)
        : m_weather_data(weather_data)
        , m_next_id(1) // Initialiser ID-tæller
    {
        // Find den største eksisterende ID for at sikre unikke nye ID'er
        if (!m_weather_data.empty()) {
            int max_id = 0;
            for (const auto& wc : m_weather_data) {
                try {
                    int current_id = std::stoi(wc.m_id);
                    if (current_id > max_id) {
                        max_id = current_id;
                    }
                } catch (const std::exception& e) {
                    // Ignorer ikke-numeriske ID'er, hvis de findes.
                    // For en robust løsning bør alle ID'er være konsistente (f.eks. altid numeriske).
                }
            }
            m_next_id = max_id + 1;
        }
    }

    // Undgå kopiering og flytning
    weather_handler_t(const weather_handler_t &) = delete;
    weather_handler_t(weather_handler_t &&) = delete;

    // Hjælpefunktion til at generere et unikt ID
    std::string generate_unique_id() {
        return std::to_string(m_next_id++);
    }

    // GET /weather - Returnerer alle vejrdata (konstant metode, da den ikke ændrer data)
    auto on_get_all_weather(
        const restinio::request_handle_t& req, rr::route_params_t /*params*/) const
    {
        auto resp = init_json_resp(req->create_response());
        resp.set_body(json_dto::to_json(m_weather_data)); // Serialiser hele vektoren
        return resp.done();
    }

    // GET /weather/:id - Returnerer vejrdata efter ID (konstant metode)
    auto on_get_weather_by_id(
        const restinio::request_handle_t& req, rr::route_params_t params) const
    {
        auto resp = init_json_resp(req->create_response());
        const auto id = params["id"]; // Hent ID fra URL-parametre

        auto it = std::find_if(m_weather_data.begin(), m_weather_data.end(),
                               [&](const weathercast_t& wc){ return wc.m_id == id; });

        if (it != m_weather_data.end()) {
            resp.set_body(json_dto::to_json(*it)); // Returner fundet objekt
        } else {
            // Hvis ID ikke findes, returner 404 Not Found
            return req->create_response(restinio::status_not_found())
                       .set_body(R"({"error": "Vejrdata med angivet ID blev ikke fundet"})")
                       .done();
        }
        return resp.done();
    }

    // GET /weather/date/:date - Returnerer vejrdata efter dato (konstant metode)
    auto on_get_weather_by_date(
        const restinio::request_handle_t& req, rr::route_params_t params) const
    {
        auto resp = init_json_resp(req->create_response());
        const auto date_str = params["date"]; // Hent dato-streng fra URL-parametre

        std::vector<weathercast_t> result;
        for (const auto& wc : m_weather_data) {
            if (wc.m_dateTime.m_date == date_str) {
                result.push_back(wc); // Tilføj poster, der matcher datoen
            }
        }

        // Returner tom array hvis ingen data findes for datoen (RESTful praksis for samlinger)
        resp.set_body(json_dto::to_json(result));
        return resp.done();
    }

    // GET /latest_three - Returnerer de tre seneste vejrdata-poster (konstant metode)
    auto on_get_latest_three(
        const restinio::request_handle_t& req, rr::route_params_t /*params*/) const
    {
        auto resp = init_json_resp(req->create_response());

        if (m_weather_data.empty()) {
            resp.set_body("[]"); // Returner tom array, hvis der ikke er data
            return resp.done();
        }

        // Lav en kopi for at sortere, da vi ikke vil ændre den oprindelige rækkefølge
        std::vector<weathercast_t> sorted_data = m_weather_data;

        // Sorter efter dato og derefter tid i faldende rækkefølge (seneste først)
        std::sort(sorted_data.begin(), sorted_data.end(),
                  [](const weathercast_t& a, const weathercast_t& b) {
                      return b.m_dateTime < a.m_dateTime; // Sammenlign b med a for faldende rækkefølge
                  });

        // Tag de første tre (eller færre, hvis der ikke er nok data)
        std::vector<weathercast_t> latest_three;
        for (size_t i = 0; i < std::min((size_t)3, sorted_data.size()); ++i) {
            latest_three.push_back(sorted_data[i]);
        }

        resp.set_body(json_dto::to_json(latest_three));
        return resp.done();
    }

    // POST /weather - Opretter en ny vejrdata-post (ikke-konstant metode, da den ændrer data)
    auto on_post_weather(
        const restinio::request_handle_t& req, rr::route_params_t /*params*/)
    {
        try {
            weathercast_t new_weather = json_dto::from_json<weathercast_t>(req->body());
            new_weather.m_id = generate_unique_id(); // Tildel et nyt unikt ID

            m_weather_data.push_back(new_weather); // Tilføj til vores lager

            auto resp = init_json_resp(req->create_response(restinio::status_created()));
            resp.set_body(json_dto::to_json(new_weather)); // Returner det oprettede objekt
            return resp.done();
        } catch (const std::exception& ex) {
            // Håndter fejl ved JSON-parsning
            return req->create_response(restinio::status_bad_request())
                       .set_body(string("Fejl ved parsning af JSON: ") + ex.what())
                       .done();
        }
    }

    // PUT /weather/:id - Opdaterer en eksisterende vejrdata-post (ikke-konstant metode)
    auto on_put_weather(
        const restinio::request_handle_t& req, rr::route_params_t params)
    {
        const auto id_to_update = params["id"]; // Hent ID fra URL-parametre

        try {
            weathercast_t updated_data = json_dto::from_json<weathercast_t>(req->body());

            auto it = std::find_if(m_weather_data.begin(), m_weather_data.end(),
                                   [&](const weathercast_t& wc){ return wc.m_id == id_to_update; });

            if (it != m_weather_data.end()) {
                // Opdater felterne. Vigtigt: Bevar det originale ID fra URL'en!
                it->m_dateTime = updated_data.m_dateTime;
                it->m_place = updated_data.m_place;
                it->m_temperature = updated_data.m_temperature;
                it->m_humidity = updated_data.m_humidity;

                auto resp = init_json_resp(req->create_response(restinio::status_ok()));
                resp.set_body(json_dto::to_json(*it)); // Returner det opdaterede objekt
                return resp.done();
            } else {
                // Hvis ID ikke findes, returner 404 Not Found
                return req->create_response(restinio::status_not_found())
                           .set_body(R"({"error": "Vejrdata med angivet ID blev ikke fundet til opdatering"})")
                           .done();
            }
        } catch (const std::exception& ex) {
            // Håndter fejl ved JSON-parsning
            return req->create_response(restinio::status_bad_request())
                       .set_body(string("Fejl ved parsning af JSON til opdatering: ") + ex.what())
                       .done();
        }
    }


    // Rod-handler (konstant metode)
    auto on_root_get(
        const restinio::request_handle_t& req, rr::route_params_t /*params*/) const
    {
        auto resp = init_json_resp(req->create_response(restinio::status_ok()));
        resp.set_body(R"({"message": "Velkommen til Vejr API'et! Tilgå /weather for alle data, /weather/:id for specifikt ID, /weather/date/:date for data på dato, /latest_three for de seneste. Brug POST på /weather og PUT på /weather/:id."})");
        return resp.done();
    }

private:
    std::vector<weathercast_t> &m_weather_data; // Reference til datalageret
    int m_next_id; // Tæller til generering af unikke ID'er

    // Hjælpefunktion til at initialisere JSON-svar med standard-headers
    template <typename RESP>
    static RESP
    init_json_resp(RESP resp)
    {
        resp.append_header("Server", "RESTinio Weather API /v.0.2") // Opdateret version
            .append_header_date_field()
            .append_header("Content-Type", "application/json; charset=utf-8")
            .append_header("Access-Control-Allow-Origin", "*"); // Grundlæggende CORS: tillad alle domæner
        return resp;
    }
};

// Funktion der opsætter serverens routing
auto server_handler(std::vector<weathercast_t> &weather_data_ref)
{
    auto router = std::make_unique<router_t>();
    auto handler = std::make_shared<weather_handler_t>(std::ref(weather_data_ref));

    // Lambda-udtryk for at binde handlerens medlemsfunktioner til routeren
    auto by = [&](auto method) {
        using namespace std::placeholders;
        return std::bind(method, handler, _1, _2);
    };

    // Standard handler for metoder, der ikke er tilladt
    auto method_not_allowed = [](const auto &req, auto) {
        return req->create_response(restinio::status_method_not_allowed())
            .append_header("Access-Control-Allow-Origin", "*")
            .append_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS") // Informer klient om tilladte metoder
            .append_header("Access-Control-Allow-Headers", "Content-Type") // Informer klient om tilladte headers
            .connection_close()
            .done();
    };

    // CORS preflight handler (OPTIONS-anmodninger)
    // Svarer på OPTIONS-anmodninger for alle ruter for at muliggøre CORS
    router->add_handler(
        restinio::router::http_method_options(),
        ".*", // Gælder for alle ruter
        [](const restinio::request_handle_t& req, auto /*params*/) {
            return req->create_response(restinio::status_ok())
                .append_header("Access-Control-Allow-Origin", "*")
                .append_header("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS")
                .append_header("Access-Control-Allow-Headers", "Content-Type")
                .append_header("Access-Control-Max-Age", "86400") // Cache preflight-svar i 24 timer
                .done();
        });


    // LAB 1 ruter (eksisterende og forbedret)
    // Handler for GET /weather (alle data)
    router->http_get("/weather", by(&weather_handler_t::on_get_all_weather));
    // Handler for GET / (rod)
    router->http_get("/", by(&weather_handler_t::on_root_get));


    // LAB 2 ruter (nye krav)
    // GET /weather/:id - Henter vejrdata for et specifikt ID
    // Regex `([0-9]+)` sikrer, at ID er numerisk
    router->http_get(
        R"(/weather/:id([0-9]+))",
        by(&weather_handler_t::on_get_weather_by_id)
    );
    // GET /weather/date/:date - Henter vejrdata for en specifik dato
    // Regex `([0-9]{8})` sikrer YYYYMMDD-format
    router->http_get(
        R"(/weather/date/:date([0-9]{8}))",
        by(&weather_handler_t::on_get_weather_by_date)
    );
    // GET /latest_three - Henter de tre seneste vejrdata-poster
    router->http_get("/latest_three", by(&weather_handler_t::on_get_latest_three));

    // POST /weather - Opretter en ny vejrdata-post
    router->http_post("/weather", by(&weather_handler_t::on_post_weather));

    // PUT /weather/:id - Opdaterer en eksisterende vejrdata-post
    // Regex `([0-9]+)` sikrer, at ID er numerisk
    router->http_put(
        R"(/weather/:id([0-9]+))",
        by(&weather_handler_t::on_put_weather)
    );


    // Catch-all for metoder, der ikke er specifikt håndteret på en rute
    // Returnerer 405 Method Not Allowed med CORS-headere
    router->add_handler(
        restinio::router::unmatched_request_router(),
        method_not_allowed
    );


    return router;
}

int main()
{
    using namespace chrono;

    try
    {
        // Initial hardkodet data, nu gemt i en vektor
        std::vector<weathercast_t> weather_data_storage;

        place_t aarhus_n_place{"Aarhus N", 56.17, 10.22};
        place_t copenhagen_place{"Copenhagen", 55.67, 12.56};

        weather_data_storage.push_back({
            "1", // ID
            dateTime_t{"20240415", "10:15"},
            aarhus_n_place,
            13.1,
            70
        });
        weather_data_storage.push_back({
            "2", // ID
            dateTime_t{"20240415", "11:30"},
            copenhagen_place,
            15.5,
            65
        });
        weather_data_storage.push_back({
            "3", // ID
            dateTime_t{"20240416", "09:00"},
            aarhus_n_place,
            10.0,
            80
        });
        weather_data_storage.push_back({
            "4", // ID
            dateTime_t{"20240416", "14:00"},
            copenhagen_place,
            12.8,
            75
        });
        weather_data_storage.push_back({
            "5", // ID (denne er den seneste ifølge hardkodede data)
            dateTime_t{"20240417", "08:30"},
            aarhus_n_place,
            9.5,
            85
        });


        // Server-egenskaber
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
                .request_handler(server_handler(weather_data_storage)) // Send reference til datalageret
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
