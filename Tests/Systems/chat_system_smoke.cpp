#include <cassert>
#include <cmath>
#include <string>
#include "../../Source/NeoEngine/Systems/ChatSystem.h"

int main() {
    using namespace NeoEngine;

    ChatSystem chat;
    assert(chat.SendMessage("alice", "Alice", "hello"));
    assert(chat.GetHistory().size() == 1);
    assert(std::isfinite(chat.GetHistory().front().timestamp));

    chat.MutePlayer("alice", 60.0f);
    assert(chat.IsMuted("alice"));
    assert(!chat.SendMessage("alice", "Alice", "muted"));

    ChatSystem independentSpam;
    assert(independentSpam.SendMessage("alice", "Alice", "a"));
    assert(independentSpam.SendMessage("bob", "Bob", "b"));
    assert(independentSpam.SendMessage("alice", "Alice", "a2"));
    assert(independentSpam.SendMessage("bob", "Bob", "b2"));

    ChatSystem blocking;
    blocking.BlockPlayer("moderator", "alice");
    assert(blocking.IsBlocked("alice"));
    assert(blocking.IsBlocked("alice", "moderator"));
    assert(!blocking.IsBlocked("bob", "moderator"));

    const auto global = blocking.GetChannelMessages(ChatChannel::Global, 0);
    assert(global.empty());
    assert(blocking.GetChannelMessages(ChatChannel::Global, -1).empty());

    return 0;
}
