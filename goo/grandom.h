/*
 * grandom.h
 *
 * This file is licensed under the GPLv2 or later
 *
 * Pseudo-random number generation
 *
 * Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
 */

#ifndef GRANDOM_H
#define GRANDOM_H

#include "gtypes.h"

/// Fills the given buffer with random bytes
void grandom_fill(Guchar *buff, int size);

/// Returns a random number in [0,1)
double grandom_double();

#endif
