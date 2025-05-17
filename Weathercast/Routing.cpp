#include <iostream>
#include <restinio/all.hpp>
#include <json_dto/pub.hpp>

using namespace std;

struct weathercast_t
{
	weathercast_t() = default;

	weathercast_t(
		string ID,
		Date Date 
        Time Time
        string Place 
        string title )
		:	m_author{ move( author ) }
		,	m_title{ move( title ) }
	{}

	template < typename JSON_IO >
	void
	json_io( JSON_IO & io )
	{
		io
			& json_dto::mandatory( "author", m_author )
			& json_dto::mandatory( "title", m_title );
	}

	string m_author;
	string m_title;
};

struct locaation
{
	
}

using book_collection_t = vector< book_t >;

namespace rr = restinio::router;
using router_t = rr::express_router_t<>;

class books_handler_t
{
public :
	explicit books_handler_t( book_collection_t & books )
		:	m_books( books )
	{}

	books_handler_t( const books_handler_t & ) = delete;
	books_handler_t( books_handler_t && ) = delete;

	auto on_books_list(
		const restinio::request_handle_t& req, rr::route_params_t ) const
	{
		auto resp = init_resp( req->create_response() );

		resp.set_body(
			"Book collection (book count: " +
				to_string( m_books.size() ) + ")\n" );

		for( size_t i = 0; i < m_books.size(); ++i )
		{
			resp.append_body( to_string( i + 1 ) + ". " );
			const auto & b = m_books[ i ];
			resp.append_body( b.m_title + "[" + b.m_author + "]\n" );
		}

		return resp.done();
	}

private :
	book_collection_t & m_books;

	template < typename RESP >
	static RESP
	init_resp( RESP resp )
	{
		resp
			.append_header( "Server", "RESTinio sample server /v.0.6" )
			.append_header_date_field()
			.append_header( "Content-Type", "text/plain; charset=utf-8" );

		return resp;
	}

	template < typename RESP >
	static void
	mark_as_bad_request( RESP & resp )
	{
		resp.header().status_line( restinio::status_bad_request() );
	}
};

auto server_handler( weathercast_t & weathercast )
{
	auto router = make_unique< router_t >();
	auto handler = make_shared< weathercast_t >( ref(weathercast) );

	auto by = [&]( auto method ) {
		using namespace placeholders;
		return bind( method, handler, _1, _2 );
	};

	auto method_not_allowed = []( const auto & req, auto ) {
			return req->create_response( restinio::status_method_not_allowed() )
					.connection_close()
					.done();
		};

	// Handlers for '/' path.
	router->http_get( "/", by( &weathercast_t::on_books_list ) );
    router->http_get( "/", by( &weathercast_t::on_books_list ) );
    router->http_get( "/", by( &weathercast_t::on_books_list ) );
    

	// Disable all other methods for '/'.
	router->add_handler(
			restinio::router::none_of_methods(
					restinio::http_method_get() ),
			"/", method_not_allowed );

	return router;
}

int main()
{
	using namespace chrono;

	try
	{
		using traits_t =
			restinio::traits_t<
				restinio::asio_timer_manager_t,
				restinio::single_threaded_ostream_logger_t,
				router_t >;

		book_collection_t book_collection{
			{ "Agatha Christie", "Murder on the Orient Express" },
			{ "Agatha Christie", "Sleeping Murder" },
			{ "B. Stroustrup", "The C++ Programming Language" }
		};

		restinio::run(
			restinio::on_this_thread< traits_t >()
				.address( "localhost" )
				.request_handler( server_handler( book_collection ) )
				.read_next_http_message_timelimit( 10s )
				.write_http_response_timelimit( 1s )
				.handle_request_timeout( 1s ) );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}
