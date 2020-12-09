#pragma once

// Game timing information.
struct game_time {
    int track_loop; // Current loop the track is on, or 0 for lead-in.
    int track_pos;  // Current position within the loop or lead-in.
};
