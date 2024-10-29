# Deribit Order Execution System

This project is an Order Execution and Management System designed for trading on the Deribit Test platform. It provides essential trading functionalities along with real-time order book updates to meet the needs of low-latency trading systems.

## Features

- **Place Order**: Allows placing buy or sell orders for any supported symbol.
- **Cancel Order**: Supports canceling active orders by specifying the order ID.
- **Modify Order**: Allows modifying an existing order's price.
- **Get Order Book**: Fetches the latest order book data for a specified symbol.
- **View Current Positions**: Displays the current open positions in the account.
- **Real-time Order Book Updates**: Includes a WebSocket server that clients can connect to, subscribe to symbols, and receive continuous, real-time order book updates.

## Supported Markets

- Spot, Futures, and Options
- All symbols supported by Deribit Test

## Setup and Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/adityaranjan06/DeribitOrderExecution.git
   cd DeribitOrderExecution
