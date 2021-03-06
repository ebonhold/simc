// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef ITEM_SCALING_HPP
#define ITEM_SCALING_HPP

#include "util/span.hpp"

#include "client_data.hpp"

struct curve_point_t
{
  unsigned curve_id;
  unsigned index;
  float    primary1;
  float    primary2;
  float    secondary1;
  float    secondary2;

  static util::span<const curve_point_t> find( unsigned id, bool ptr );

  static const curve_point_t& nil()
  { return dbc::nil<curve_point_t>; }

  static util::span<const curve_point_t> data( bool ptr );
};

#endif /* ITEM_SCALING_HPP */
