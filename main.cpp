// main.cpp

#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <json/json.h>  // Ensure you have jsoncpp installed
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <thread>
#include <mutex>
#include <cstdlib>      // For getenv
#include <sstream>

// Deribit API details
const std::string BASE_URL = "https://test.deribit.com";
const std::string CLIENT_ID = getenv("DERIBIT_CLIENT_ID") ? getenv("DERIBIT_CLIENT_ID") : "";
const std::string CLIENT_SECRET = getenv("DERIBIT_CLIENT_SECRET") ? getenv("DERIBIT_CLIENT_SECRET") : "";
std::string ACCESS_TOKEN;

// Mutex for thread-safe access token updates
std::mutex token_mutex;

// Utility to handle HTTP responses
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Utility to handle HTTP requests with custom headers
std::string httpRequest(const std::string& url, const std::string& params, bool is_post = true, const std::vector<std::string>& headers = {})
{
    CURL* curl;
    CURLcode res;
    std::string response;
    curl = curl_easy_init();

    if(curl) {
        if (!is_post && !params.empty()) {
            std::string full_url = BASE_URL + url + "?" + params;
            curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        } else {
            curl_easy_setopt(curl, CURLOPT_URL, (BASE_URL + url).c_str());
        }

        if(is_post) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        }

        // Set custom headers if any
        struct curl_slist* chunk = NULL;
        for(const auto& header : headers) {
            chunk = curl_slist_append(chunk, header.c_str());
        }
        if(chunk) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        }

        // Enable verbose output for debugging (optional)
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Set timeout for low latency
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        if(chunk) {
            curl_slist_free_all(chunk);
        }

        curl_easy_cleanup(curl);
    }
    return response;
}

// Authenticate and get access token
bool authenticate()
{
    if (CLIENT_ID.empty() || CLIENT_SECRET.empty()) {
        std::cerr << "Error: DERIBIT_CLIENT_ID and DERIBIT_CLIENT_SECRET must be set as environment variables." << std::endl;
        return false;
    }

    std::string url = "/api/v2/public/auth";

    // Create JSON-RPC payload
    Json::Value payload;
    payload["jsonrpc"] = "2.0";
    payload["method"] = "public/auth";
    payload["params"]["grant_type"] = "client_credentials";
    payload["params"]["client_id"] = CLIENT_ID;
    payload["params"]["client_secret"] = CLIENT_SECRET;
    payload["id"] = 1;

    // Convert JSON object to string
    Json::StreamWriterBuilder writer;
    std::string params = Json::writeString(writer, payload);

    // Set headers
    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Accept: application/json"
    };

    // Send POST request with JSON-RPC payload
    std::string response = httpRequest(url, params, true, headers);

    // Parse the JSON response
    Json::CharReaderBuilder readerBuilder;
    Json::Value jsonResponse;
    std::string errs;

    std::istringstream s(response);
    if (!Json::parseFromStream(readerBuilder, s, &jsonResponse, &errs)) {
        std::cerr << "Failed to parse authentication response: " << errs << std::endl;
        return false;
    }

    // Check for successful authentication
    if (jsonResponse.isMember("result") && jsonResponse["result"].isMember("access_token")) {
        std::lock_guard<std::mutex> lock(token_mutex);
        ACCESS_TOKEN = jsonResponse["result"]["access_token"].asString();
        std::cout << "Authenticated successfully. Access Token: " << ACCESS_TOKEN << std::endl;
        return true;
    } else {
        std::cerr << "Authentication failed. Response: " << response << std::endl;
        return false;
    }
}


// Place an Order
void placeOrder(const std::string& symbol, const std::string& side, double quantity, double price)
{
    std::string url = std::string("/api/v2/private/") + ((side == "buy") ? "buy" : "sell");
    std::lock_guard<std::mutex> lock(token_mutex);
    std::string params = "access_token=" + ACCESS_TOKEN +
                         "&instrument_name=" + symbol +
                         "&amount=" + std::to_string(quantity) +
                         "&price=" + std::to_string(price);

    std::string response = httpRequest(url, params);
    std::cout << "Place Order Response: " << response << std::endl;
}


// Cancel an Order
void cancelOrder(const std::string& order_id)
{
    std::string url = "/api/v2/private/cancel";
    std::lock_guard<std::mutex> lock(token_mutex);
    std::string params = "access_token=" + ACCESS_TOKEN + "&order_id=" + order_id;

    std::string response = httpRequest(url, params);
    std::cout << "Cancel Order Response: " << response << std::endl;
}

// Modify an Order
void modifyOrder(const std::string& order_id, double new_price)
{
    std::string url = "/api/v2/private/edit";
    std::lock_guard<std::mutex> lock(token_mutex);
    std::string params = "access_token=" + ACCESS_TOKEN +
                         "&order_id=" + order_id +
                         "&new_price=" + std::to_string(new_price);

    std::string response = httpRequest(url, params);
    std::cout << "Modify Order Response: " << response << std::endl;
}

// Get Order Book
void getOrderBook(const std::string& symbol)
{
    std::string url = "/api/v2/public/get_order_book";
    std::string params = "instrument_name=" + symbol + "&depth=10"; // Adjust depth as needed

    std::string response = httpRequest(url, params, false); // GET request
    std::cout << "Order Book Response: " << response << std::endl;
}

// View Current Positions
void viewPositions()
{
    std::string url = "/api/v2/private/get_positions";
    std::lock_guard<std::mutex> lock(token_mutex);
    std::string params = "access_token=" + ACCESS_TOKEN;

    std::string response = httpRequest(url, params);
    std::cout << "Positions Response: " << response << std::endl;
}

// WebSocket Server for Real-time Updates
typedef websocketpp::server<websocketpp::config::asio> server;

class WebSocketServer {
public:
    WebSocketServer() {
        // Initialize Asio
        ws_server.init_asio();

        // Register handlers
        ws_server.set_open_handler(std::bind(&WebSocketServer::on_open, this, std::placeholders::_1));
        ws_server.set_close_handler(std::bind(&WebSocketServer::on_close, this, std::placeholders::_1));
        ws_server.set_message_handler(std::bind(&WebSocketServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void on_open(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        connections.insert(hdl);
        std::cout << "Client connected." << std::endl;
    }

    void on_close(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        connections.erase(hdl);
        std::cout << "Client disconnected." << std::endl;
    }

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        std::string symbol = msg->get_payload();
        {
            std::lock_guard<std::mutex> lock(subscriptions_mutex);
            subscriptions.insert(symbol);
        }
        std::cout << "Client subscribed to: " << symbol << std::endl;
        // Here, you would integrate with Deribit's WebSocket API to fetch real-time data
        // and send updates to subscribed clients.
    }

    void run(uint16_t port) {
        ws_server.listen(port);
        ws_server.start_accept();
        std::cout << "WebSocket Server running on port " << port << std::endl;
        ws_server.run();
    }

    // Function to send order book updates to clients
    void sendOrderBookUpdate(const std::string& symbol, const std::string& update) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        std::lock_guard<std::mutex> lock_sub(subscriptions_mutex);
        for(auto it : connections) {
            // Check if the client is subscribed to this symbol
            // For simplicity, this example sends updates to all clients
            ws_server.send(it, update, websocketpp::frame::opcode::text);
        }
    }

private:
    server ws_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections;
    std::set<std::string> subscriptions;
    std::mutex connections_mutex;
    std::mutex subscriptions_mutex;
};

int main()
{
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Authenticate
    if (!authenticate()) {
        std::cerr << "Authentication failed. Exiting." << std::endl;
        return 1;
    }

    // Start WebSocket Server in a separate thread (Advanced Bonus)
    WebSocketServer ws_server;
    std::thread ws_thread([&ws_server]() {
        ws_server.run(9002); // Port 9002
    });

    // Simple menu-driven interface for testing
    while (true) {
        std::cout << "\n=== Deribit Order Execution System ===\n";
        std::cout << "1. Place Order\n";
        std::cout << "2. Cancel Order\n";
        std::cout << "3. Modify Order\n";
        std::cout << "4. Get Order Book\n";
        std::cout << "5. View Current Positions\n";
        std::cout << "6. Exit\n";
        std::cout << "Select an option: ";

        int choice;
        std::cin >> choice;

        if (choice == 1) {
            std::string symbol, side;
            double quantity, price;
            std::cout << "Enter Symbol (e.g., BTC-PERPETUAL): ";
            std::cin >> symbol;
            std::cout << "Enter Side (buy/sell): ";
            std::cin >> side;
            std::cout << "Enter Quantity: ";
            std::cin >> quantity;
            std::cout << "Enter Price: ";
            std::cin >> price;
            placeOrder(symbol, side, quantity, price);
        }
        else if (choice == 2) {
            std::string order_id;
            std::cout << "Enter Order ID to Cancel: ";
            std::cin >> order_id;
            cancelOrder(order_id);
        }
        else if (choice == 3) {
            std::string order_id;
            double new_price;
            std::cout << "Enter Order ID to Modify: ";
            std::cin >> order_id;
            std::cout << "Enter New Price: ";
            std::cin >> new_price;
            modifyOrder(order_id, new_price);
        }
        else if (choice == 4) {
            std::string symbol;
            std::cout << "Enter Symbol to Get Order Book (e.g., BTC-PERPETUAL): ";
            std::cin >> symbol;
            getOrderBook(symbol);
        }
        else if (choice == 5) {
            viewPositions();
        }
        else if (choice == 6) {
            std::cout << "Exiting..." << std::endl;
            break;
        }
        else {
            std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    // Cleanup CURL
    curl_global_cleanup();

    // Note: In a real application, you would implement a graceful shutdown for the WebSocket server.
    // For simplicity, we'll terminate the program here, which will also stop the WebSocket thread.

    return 0;
}
