#include <chrono>
#include "matching_engine_fifo.hpp"
#include <thread>

int main() {
    MatchingEngineFIFO engine;
    engine.create_order(true, 5, 20);
    engine.create_order(true, 5, 15);
    engine.create_order(true, 6, 20);
    engine.create_order(true, 7, 10);
    engine.create_order(true, 6.7, 15);
    engine.print_order_books();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    engine.create_order(false, 5, 90);
    engine.print_order_books();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    engine.create_order(true, 7, 20);
    engine.print_order_books();
    return 0;
}