//========================================================================
//
// GooTimer.cc
//
// This file is licensed under GPLv2 or later
//
// Copyright 2005 Jonathan Blandford <jrb@redhat.com>
// Copyright 2007 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright 2010 Hib Eris <hib@hiberis.nl>
// Copyright 2011, 2025 Albert Astals cid <aacid@kde.org>
// Copyright 2014 Bogdan Cristea <cristeab@gmail.com>
// Copyright 2014 Peter Breitenlohner <peb@mppmu.mpg.de>
// Inspired by gtimer.c in glib, which is Copyright 2000 by the GLib Team
//
//========================================================================

#ifndef GOOTIMER_H
#define GOOTIMER_H

#include <chrono>

#include "poppler_private_export.h"

//------------------------------------------------------------------------
// GooTimer
//------------------------------------------------------------------------

class POPPLER_PRIVATE_EXPORT GooTimer
{
public:
    // Create a new timer.
    GooTimer();

    void start();
    void stop();
    double getElapsed();

private:
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    bool active;
};

#endif
