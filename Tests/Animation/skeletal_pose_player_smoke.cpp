#include "Animation/SkeletalPosePlayer.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool Near(const float left, const float right) { return std::fabs(left - right) < 0.0001F; }
NeoEngine::SkeletalPoseKeyframe Key(const float time, const float x) { NeoEngine::SkeletalPoseKeyframe key{}; key.time = time; key.translation = {x, 0.0F, 0.0F}; return key; }
}

int main() {
    using namespace NeoEngine;
    SkeletalPoseClip clip; if (!clip.Configure(1U) || !clip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F), Key(1.0F, 2.0F)})) return 1;
    SkeletalPosePlayer player; std::vector<Mat4> output;
    if (!player.Bind(clip, SkeletalPosePlaybackMode::Clamp) || !player.Advance(0.25F, output) || !Near(player.Time(), 0.25F) || output.size() != 1U || !Near(output[0].m[12], 0.5F)) return 1;
    player.SetPaused(true); if (!player.Advance(0.25F, output) || !Near(player.Time(), 0.25F) || !Near(output[0].m[12], 0.5F)) return 1;
    player.SetPaused(false); if (!player.SetSpeed(2.0F) || !player.Advance(0.25F, output) || !Near(player.Time(), 0.75F) || !Near(output[0].m[12], 1.5F)) return 1;
    if (!player.Advance(1.0F, output) || !Near(player.Time(), 1.0F) || !Near(output[0].m[12], 2.0F)) return 1;
    if (!player.Bind(clip, SkeletalPosePlaybackMode::Loop) || !player.Advance(0.75F, output) || !player.Advance(0.5F, output) || !Near(player.Time(), 0.25F) || !Near(output[0].m[12], 0.5F)) return 1;
    const std::vector<Mat4> stable = output; const float stableTime = player.Time();
    if (player.Advance(-0.1F, output) || player.LastError() != SkeletalPosePlayerError::InvalidDelta || !Near(player.Time(), stableTime) || output.size() != stable.size() || !Near(output[0].m[12], stable[0].m[12])) return 1;
    SkeletalPoseClip staticClip; if (!staticClip.Configure(1U) || !staticClip.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F)}) || player.Bind(staticClip, SkeletalPosePlaybackMode::Loop) || player.LastError() != SkeletalPosePlayerError::LoopUnavailable || !Near(player.Time(), stableTime) || !Near(output[0].m[12], stable[0].m[12])) return 1;
    SkeletalPosePlayer snapshotPlayer; { SkeletalPoseClip ephemeral; if (!ephemeral.Configure(1U) || !ephemeral.SetTrack(0U, std::vector<SkeletalPoseKeyframe>{Key(0.0F, 0.0F), Key(1.0F, 4.0F)}) || !snapshotPlayer.Bind(ephemeral, SkeletalPosePlaybackMode::Clamp)) return 1; }
    if (!snapshotPlayer.Advance(0.5F, output) || !Near(snapshotPlayer.Time(), 0.5F) || !Near(output[0].m[12], 2.0F)) return 1;
    if (player.SetSpeed(-1.0F) || player.LastError() != SkeletalPosePlayerError::InvalidSpeed || !Near(player.Speed(), 1.0F)) return 1;
    std::printf("SKELETAL_POSE_PLAYER_SMOKE_OK clamp=1 loop=1 pause=1 speed=1 atomic=1 boundedDelta=1\n");
    return 0;
}
