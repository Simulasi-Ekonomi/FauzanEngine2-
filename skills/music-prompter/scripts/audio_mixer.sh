#!/bin/bash
# Skill: music-prompter - Audio Crossfader
# Menggabungkan dua klip musik dengan crossfade 1 detik

if [ "$#" -ne 3 ]; then
    echo "Usage: ./audio_mixer.sh clip1.mp3 clip2.mp3 output.mp3"
    exit 1
fi

ffmpeg -i $1 -i $2 -filter_complex "[0:a][1:a]acrossfade=d=1:c1=tri:c2=tri" $3
