#include <cassert>
#include <cmath>
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
    assert(independentSpam.SendMessage("alice", "Alice", "a1"));
    assert(independentSpam.SendMessage("bob", "Bob", "b1"));
    assert(independentSpam.SendMessage("alice", "Alice", "a2"));
    assert(independentSpam.SendMessage("bob", "Bob", "b2"));
    assert(independentSpam.SendMessage("alice", "Alice", "a3"));
    assert(independentSpam.SendMessage("bob", "Bob", "b3"));
    assert(independentSpam.SendMessage("alice", "Alice", "a4"));
    assert(independentSpam.SendMessage("bob", "Bob", "b4"));
    assert(independentSpam.SendMessage("alice", "Alice", "a5"));
    assert(independentSpam.SendMessage("bob", "Bob", "b5"));
    assert(independentSpam.SendMessage("alice", "Alice", "a6"));
    assert(independentSpam.IsMuted("alice"));
    assert(!independentSpam.SendMessage("bob", "Bob", "b6"));
    assert(independentSpam.IsMuted("bob"));

    ChatSystem blocking;
    blocking.BlockPlayer("moderator", "alice");
    assert(blocking.IsBlocked("alice"));
    assert(blocking.IsBlocked("alice", "moderator"));
    assert(!blocking.IsBlocked("bob", "moderator"));

    assert(blocking.GetChannelMessages(ChatChannel::Global, 0).empty());
    assert(blocking.GetChannelMessages(ChatChannel::Global, -1).empty());
    return 0;
}
