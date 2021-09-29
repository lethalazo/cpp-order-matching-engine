#ifndef MATCHING_ENGINE_FIFO_HPP
#define MATCHING_ENGINE_FIFO_HPP

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <stdlib.h>

struct LimitOrder {
    bool is_buy;
    bool is_filled;
    double limit_price;
    double total_fill_price;
    double avg_fill_price;
    u_int32_t order_id;
    u_int32_t unfilled_quantity;
    u_int32_t filled_quantity;
};

typedef std::list<std::shared_ptr<LimitOrder> > price_point_t;
typedef std::map<double,std::shared_ptr<price_point_t> > order_book_t;

class MatchingEngineFIFO {
    public:
        MatchingEngineFIFO(){}

        bool create_order(bool is_buy, double price, u_int32_t quantity) {
            if (!quantity || !price) throw("Invalid order parameters.");

            // Create an order.
            next_order_id++;

            std::shared_ptr<LimitOrder> order(new LimitOrder);
            order->order_id = next_order_id;
            order->is_buy = is_buy;
            order->limit_price = price;
            order->unfilled_quantity = quantity;
            order->filled_quantity = 0;

            // Try to match price points depending on the order type.
            if (match_price_points(order)) return true;

            // Push to book if order cn't be pro-actively completely filled.
            // Create an entry in the appropriate order book.
            order_book_t& order_book = is_buy ? buy_orders : sell_orders;

            if (!order_book.count(price)) {
                std::shared_ptr<price_point_t>  list_(new price_point_t);
                order_book[price] = list_;
            }
            order_book[price]->push_back(order);
            return false;
        }

        void print_order_books() {
            std::cout << "=============" << std::endl;
            std::cout << "SELL Orders" << std::endl;
            print_order_book(false);
            std::cout << "BUY Orders" << std::endl;
            print_order_book(true);
            std::cout << "=============" << std::endl;
        }

    private:
        bool fill_order(std::shared_ptr<LimitOrder> order, double fill_price_point, u_int32_t fill_quantity) {
            const double fill_price = fill_price_point * fill_quantity;

            // Update the order with the fill price and filled quantity.
            order->total_fill_price += fill_price;
            order->filled_quantity += fill_quantity;
            order->unfilled_quantity -= fill_quantity;

            // Signal user.
            std::cout << "Partially Filled " << order->order_id << " " <<(order->is_buy ? "BUY " : "SELL ") << fill_quantity << " @ " << fill_price_point << std::endl;

            // Check if order is completely filled.
            if (!order->unfilled_quantity) {
                // Update order data.
                order->avg_fill_price = order->total_fill_price/order->filled_quantity;
                order->is_filled = true;

                // Signal user.
                std::cout << "Completed " << order->order_id << " " << (order->is_buy ? "BUY " : "SELL ") << order->filled_quantity << " @ " << order->limit_price << " w/ Avg. Price " << order->avg_fill_price << std::endl;
                return true;
            }
            return false;
        }

        bool match_orders_in_price_point(std::shared_ptr<LimitOrder> order, std::shared_ptr<price_point_t> pp) {
            // While the current price point has unfilled orders, try to fill the matching orders.
            while (!pp->empty()) {
                std::shared_ptr<LimitOrder> matching_order = pp->front();
                
                // Get the total price of the fill.
                const u_int32_t fill_quantity = std::min(matching_order->unfilled_quantity, order->unfilled_quantity);

                if (fill_order(matching_order, order->limit_price, fill_quantity)) pp->pop_front();
                if (fill_order(order, order->limit_price, fill_quantity)) return true;
            }
            return false;
        }

        bool match_price_points(std::shared_ptr<LimitOrder> order) {
            order_book_t& opposing_order_book = order->is_buy ? sell_orders : buy_orders;

            // As we are using a tree map, we start from the smallest price if the order is a BUY order, else we start from the highest price if the order is a SELL order (reverse).
            order_book_t::iterator it = order->is_buy ? opposing_order_book.begin() : opposing_order_book.end();
            
            order_book_t::iterator end_it = order->is_buy ? opposing_order_book.end() : opposing_order_book.begin();

            // While we are not at the end of the order book...
            while (it != end_it) {
                // Go to the next lower price point if order is SELL order, proactively compensating for the reverse iteration.
                if (!order->is_buy) it--;

                // Can't fill if the price point is higher and the order is a BUY order, or if the price point is lower and the order is a SELL order
                if ((order->is_buy && (it->first > order->limit_price)) || 
                    (!order->is_buy && (it->first < order->limit_price)))
                        break;

                // Get the orders in this price point.
                std::shared_ptr<price_point_t> pp = it->second;
                if (match_orders_in_price_point(order, pp)) return true;

                // Go to the next higher price point if order is BUY order
                if (order->is_buy) it++; 
            }
            return false;
        }

        void print_order_book(bool is_buy) {
            order_book_t& order_book = is_buy ? buy_orders : sell_orders;

            for (order_book_t::iterator it = order_book.begin(); it != order_book.end(); it++) {
                price_point_t pp = *it->second;
                for (std::shared_ptr<LimitOrder> order : pp) std::cout << order->limit_price << ", " << order->unfilled_quantity << std::endl;
            }
        }

        u_int32_t next_order_id;
        order_book_t buy_orders;
        order_book_t sell_orders;
};

#endif