// Round-trip test for the tictactoe protobuf wire format.
// Serializes Move and new_game variants, parses them back, and checks fields.

#include "tictactoe.pb.h"

#include <cassert>
#include <cstdio>
#include <string>

static int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL %s:%d: %s\n",                         \
                         __FILE__, __LINE__, #cond);                         \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static void test_move_roundtrip()
{
    tictactoe::GameMessage out;
    auto* mv = out.mutable_move();
    mv->set_row(2);
    mv->set_col(1);
    mv->set_player(1); // X

    std::string wire;
    CHECK(out.SerializeToString(&wire));

    tictactoe::GameMessage in;
    CHECK(in.ParseFromString(wire));
    CHECK(in.has_move());
    CHECK(!in.has_new_game());
    CHECK(in.move().row() == 2u);
    CHECK(in.move().col() == 1u);
    CHECK(in.move().player() == 1u);
}

static void test_new_game_roundtrip()
{
    tictactoe::GameMessage out;
    out.set_new_game(true);

    std::string wire;
    CHECK(out.SerializeToString(&wire));

    tictactoe::GameMessage in;
    CHECK(in.ParseFromString(wire));
    CHECK(in.has_new_game());
    CHECK(!in.has_move());
    CHECK(in.new_game() == true);
}

static void test_parse_rejects_garbage()
{
    tictactoe::GameMessage in;
    // Random bytes that are not a valid protobuf — parse should fail or yield
    // a message with no fields set. Either is fine; we just want no crash.
    const char junk[] = "\xff\xff\xff\xff\xff\xff";
    // ParseFromArray may succeed (unknown fields are tolerated) but should
    // not populate either oneof branch.
    in.ParseFromArray(junk, sizeof(junk) - 1);
    CHECK(!in.has_move() && !in.has_new_game());
}

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    test_move_roundtrip();
    test_new_game_roundtrip();
    test_parse_rejects_garbage();

    google::protobuf::ShutdownProtobufLibrary();

    if (failures) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all protobuf round-trip tests passed\n");
    return 0;
}
