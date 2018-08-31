/*
 * grandom.cc
 *
 * This file is licensed under the GPLv2 or later
 *
 * Pseudo-random number generation
 *
 * Copyright (C) 2012 Fabio D'Urso <fabiodurso@hotmail.it>
 */

#include "grandom.h"

#include <random>

namespace
{

auto& grandom_engine()
{
  static thread_local std::independent_bits_engine<std::default_random_engine, std::numeric_limits<Guchar>::digits, Guchar> engine{
    std::default_random_engine{
      std::random_device{}()
    }
  };
  return engine;
}

}

void grandom_fill(Guchar *buff, int size)
{
  auto& engine = grandom_engine();
  for (int index = 0; index < size; ++index) {
    buff[index] = engine();
  }
}

double grandom_double()
{
  auto& engine = grandom_engine();
  return std::generate_canonical<double, std::numeric_limits<double>::digits>(engine);
}
