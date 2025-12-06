//========================================================================
//
// GooTimer.cc
//
// This file is licensed under GPLv2 or later
//
// Copyright 2005 Jonathan Blandford <jrb@redhat.com>
// Copyright 2007 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2025 Albert Astals cid <aacid@kde.org>
// Inspired by gtimer.c in glib, which is Copyright 2000 by the GLib Team
//
//========================================================================

#include "GooTimer.h"

//------------------------------------------------------------------------
// GooTimer
//------------------------------------------------------------------------

GooTimer::GooTimer()
{
    start();
}

void GooTimer::start()
{
    start_time = std::chrono::steady_clock::now();
    active = true;
}

void GooTimer::stop()
{
    end_time = std::chrono::steady_clock::now();
    active = false;
}

double GooTimer::getElapsed()
{
    if (active) {
        end_time = std::chrono::steady_clock::now();
    }

    const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();

    return microseconds / 1000000.0;
}
