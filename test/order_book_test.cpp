#include "gtest/gtest.h"
#include "../order_book.cpp"

/*
TEST(OrderBook, OrderComparisonWorks){

    order order1(".12345678", "5.00", 'b');
    order order2(".12345678", "3.00", 'b');

    ASSERT_EQ(order1, order2);

    //should only round to 8 decimal places. Past that should be equal
    order order3(".123456783", "5.00", 'b');
    order order4(".123456784", "3.00", 'b');

    ASSERT_EQ(order3, order4);
    ASSERT_LT(order3, order4);

    //should only round to 8 decimal places. less then that should not be equal
    order order5(".12345678", "5.00", 'b');
    order order6(".12345679", "3.00", 'b');

    ASSERT_FALSE(order5 == order6);
    ASSERT_NE(order5, order6);
}
*/

TEST(OrderBook, RecordingOrdersWorks){

    order_book* book1 = new order_book;
    ASSERT_EQ(0, book1->book_size('b'));

    book1->print_book();

    ASSERT_TRUE(book1->record_buy(.12345678, 5.00));
    ASSERT_FLOAT_EQ(.12345678, book1->highest_bid().price);
    ASSERT_EQ(1, book1->book_size('b'));
    book1->print_book();

    //higher, should be at front of bids
    ASSERT_TRUE(book1->record_buy(.22345678, 5.00));
    ASSERT_EQ(.22345678, book1->highest_bid().price);
    ASSERT_EQ(2, book1->book_size('b'));
    book1->print_book();

    //shouldn't change buy book
    ASSERT_TRUE(book1->record_sell(.32345678, 2.00));
    ASSERT_FALSE(book1->record_sell(20.33345678, 8.00));

    ASSERT_EQ(.22345678, book1->highest_bid().price);
    ASSERT_EQ(2, book1->book_size('b'));
    book1->print_book();

    //lower, should not change front of bids
    ASSERT_FALSE(book1->record_buy(.02345678, 5.00));
    ASSERT_EQ(.22345678, book1->highest_bid().price);
    ASSERT_EQ(3, book1->book_size('b'));
    book1->print_book();

    //zero size, should remove order
    ASSERT_TRUE(book1->record_buy(.22345678, 0.0000));
    ASSERT_EQ(.12345678, book1->highest_bid().price);
    ASSERT_EQ(2, book1->book_size('b'));
    book1->print_book();

    //zero size, should remove order
    ASSERT_TRUE(book1->record_buy(.12345678, 0.0000));
    ASSERT_EQ(.02345678, book1->highest_bid().price);
    ASSERT_EQ(1, book1->book_size('b'));
    book1->print_book();
}
