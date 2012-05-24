// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// MoP TODO
// ==========================================================================
// Fire Elemental:
// - Fire blast is on a ~6second cooldown, like spell data indicates
// - Melee swing every 1.4 seconds
// - Seems to inherit owner crit/haste
// - Inherits owner's crit damage bonus
// - Separate SP/INT scaling seems to be gone, seems to inherit ~0.55 of 
//   owner spell power (at 85), needs validation at 90
// - Stats update dynamically
// - Even with flame shock on target and meleeing, elemental suffers from an
//   idle time, around 3.5 seconds. Idle time here is defined as the interval 
//   between the summon event from Fire Elemental Totem, to the first action 
//   performed by the elemental
// - Receives 10.5 hit points per stamina
// 
// Code:
// - Ascendancy
// - Redo Totem system
// - Talents Totemic Restoration, Ancestral Swiftness Instacast
//
// General:
// - Echo of the Elements proc rates nearer to release, currently (~late April 2k12) 
//   tested to 30% for Enhancement, 6% for Elemental / Restoration (1k LB casts both)
// - Class base stats for 87..90
// - Unleashed fury
//   * Flametongue: Additive or Multiplicative modifier (has a new spell data aura type)
//   * Windfury: Static Shock proc% (same as normal proc%?)
// - (Fire|Earth) Elemental scaling with and without Primal Elementalist
// - Searing totem base damage scaling
// - Glyph of Telluric Currents
//   * Additive or Multiplicative with other modifiers (spell data indicates additive)
// - Does Unleash Flame "double cast" still exist?
//
// Enhancement:
// - Lava Lash damage formula, currently weapon_dmg * ( 1.0 + ft_bonus + sf_stack * sf_bonus )
// - Spirit Wolves scaling
// - Maelstrom Weapon PPM (presumed to be 10ppm)
//
// Elemental:
// - Shamanism
//   * Additive or Multiplicative with others (spell data indicates additive)
//   * Affects overloads? (spell data indicates so)
//
// ==========================================================================
// BUGS
// ==========================================================================
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_class_modules.hpp"

// ==========================================================================
// Shaman
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE

struct shaman_t;

#if SC_SHAMAN == 1

enum totem_e { TOTEM_NONE=0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };

enum imbue_e { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE };

struct shaman_melee_attack_t;
struct shaman_spell_t;

struct shaman_td_t : public actor_pair_t
{
  dot_t* dots_flame_shock;
  dot_t* dots_searing_flames;

  buff_t* debuffs_searing_flames;
  buff_t* debuffs_stormstrike;
  buff_t* debuffs_unleashed_fury_ft;

  shaman_td_t( player_t* target, player_t* p ) :
    actor_pair_t( target, p )
  {
    dots_flame_shock    = target -> get_dot( "flame_shock",    p );
    dots_searing_flames = target -> get_dot( "searing_flames", p );

    debuffs_searing_flames = ( buff_creator_t( *this, "searing_flames", p -> find_specialization_spell( "Searing Flames" ) )
			       .chance( p -> dbc.spell( 77661 ) -> proc_chance() )
			       .duration( p -> dbc.spell( 77661 ) -> duration() )
			       .max_stack( p -> dbc.spell( 77661 ) -> max_stacks() ) );

    debuffs_stormstrike = ( buff_creator_t( *this, "stormstrike", p -> find_specialization_spell( "Stormstrike" ) ) );

    debuffs_unleashed_fury_ft = ( buff_creator_t( *this, "unleashed_fury_ft", p -> find_spell( 118470 ) ) );
  }
};

struct shaman_t : public player_t
{
  // Options
  timespan_t wf_delay;
  timespan_t wf_delay_stddev;
  timespan_t uf_expiration_delay;
  timespan_t uf_expiration_delay_stddev;
  double     eoe_proc_chance;

  // Active
  action_t* active_lightning_charge;
  action_t* active_searing_flames_dot;

  // Cached actions
  action_t* action_flame_shock;
  action_t* action_improved_lava_lash;

  // Pets
  pet_t* pet_feral_spirit;
  pet_t* pet_fire_elemental;
  pet_t* guardian_fire_elemental;
  pet_t* pet_earth_elemental;

  // Totems
  action_t* totems[ TOTEM_MAX ];

  // Buffs
  struct
  {
    buff_t* elemental_blast;
    buff_t* elemental_focus;
    buff_t* elemental_mastery;
    buff_t* flurry;
    buff_t* lava_surge;
    buff_t* lightning_shield;
    buff_t* maelstrom_weapon;
    buff_t* shamanistic_rage;
    buff_t* spiritwalkers_grace;
    buff_t* tier13_2pc_caster;
    buff_t* tier13_4pc_caster;
    buff_t* tier13_4pc_healer;
    buff_t* unleash_flame;
    buff_t* unleash_wind;
    buff_t* unleashed_fury_ft;
    buff_t* unleashed_fury_wf;
    buff_t* water_shield;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* earth_elemental;
    cooldown_t* fire_elemental;
    cooldown_t* lava_burst;
    cooldown_t* shock;
    cooldown_t* strike;
    cooldown_t* windfury_weapon;
  } cooldown;

  // Gains
  struct
  {
    gain_t* primal_wisdom;
    gain_t* rolling_thunder;
    gain_t* telluric_currents;
    gain_t* thunderstorm;
    gain_t* water_shield;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* elemental_overload;
    proc_t* lava_surge;
    proc_t* maelstrom_weapon;
    proc_t* rolling_thunder;
    proc_t* static_shock;
    proc_t* swings_clipped_mh;
    proc_t* swings_clipped_oh;
    proc_t* wasted_ls;
    proc_t* wasted_mw;
    proc_t* windfury;

    proc_t* fulmination[7];
    proc_t* maelstrom_weapon_used[6];
  } proc;

  // Random Number Generators
  struct
  {
    rng_t* echo_of_the_elements;
    rng_t* elemental_overload;
    rng_t* lava_surge;
    rng_t* primal_wisdom;
    rng_t* rolling_thunder;
    rng_t* searing_flames;
    rng_t* static_shock;
    rng_t* windfury_delay;
    rng_t* windfury_weapon;
  } rng;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* mail_specialization;

    // Elemental
    const spell_data_t* elemental_focus;
    const spell_data_t* elemental_precision;
    const spell_data_t* elemental_fury;
    const spell_data_t* fulmination;
    const spell_data_t* lava_surge;
    const spell_data_t* rolling_thunder;
    const spell_data_t* shamanism;
    const spell_data_t* spiritual_insight;

    // Enhancement
    const spell_data_t* dual_wield;
    const spell_data_t* flurry;
    const spell_data_t* mental_quickness;
    const spell_data_t* primal_wisdom;
    const spell_data_t* searing_flames;
    const spell_data_t* shamanistic_rage;
    const spell_data_t* static_shock;
    const spell_data_t* maelstrom_weapon;
  } specialization;

  // Masteries
  struct
  {
    const spell_data_t* elemental_overload;
    const spell_data_t* enhanced_elements;
  } mastery;

  // Talents
  struct
  {
    const spell_data_t* call_of_the_elements;
    const spell_data_t* totemic_restoration;

    const spell_data_t* elemental_mastery;
    const spell_data_t* ancestral_swiftness;
    const spell_data_t* echo_of_the_elements;

    const spell_data_t* unleashed_fury;
    const spell_data_t* primal_elementalist;
    const spell_data_t* elemental_blast;
  } talent;

  // Glyphs
  struct
  {
    const spell_data_t* chain_lightning;
    const spell_data_t* fire_elemental_totem;
    const spell_data_t* flame_shock;
    const spell_data_t* lava_lash;
    const spell_data_t* spiritwalkers_grace;
    const spell_data_t* telluric_currents;
    const spell_data_t* thunder;
    const spell_data_t* thunderstorm;
    const spell_data_t* unleashed_lightning;
    const spell_data_t* water_shield;
  } glyph;

  // Misc Spells
  struct
  {
    const spell_data_t* primal_wisdom;
    const spell_data_t* searing_flames;
  } spell;

  // Weapon Enchants
  shaman_melee_attack_t* windfury_mh;
  shaman_melee_attack_t* windfury_oh;
  shaman_spell_t*  flametongue_mh;
  shaman_spell_t*  flametongue_oh;

  target_specific_t<shaman_td_t> target_data;

  shaman_t( sim_t* sim, const std::string& name, race_e r = RACE_TAUREN ) :
    player_t( sim, SHAMAN, name, r ),
    wf_delay( timespan_t::from_seconds( 0.95 ) ), wf_delay_stddev( timespan_t::from_seconds( 0.25 ) ),
    uf_expiration_delay( timespan_t::from_seconds( 0.3 ) ), uf_expiration_delay_stddev( timespan_t::from_seconds( 0.05 ) ),
    eoe_proc_chance( 0 ) 
  {
    target_data.init( "target_data", this );

    // Active
    active_lightning_charge   = 0;
    active_searing_flames_dot = 0;

    action_flame_shock = 0;
    action_improved_lava_lash = 0;

    // Pets
    pet_feral_spirit     = 0;
    pet_fire_elemental  = 0;
    pet_earth_elemental = 0;

    // Totem tracking
    for ( int i = 0; i < TOTEM_MAX; i++ ) totems[ i ] = 0;

    // Cooldowns
    cooldown.earth_elemental      = get_cooldown( "earth_elemental_totem" );
    cooldown.fire_elemental       = get_cooldown( "fire_elemental_totem"  );
    cooldown.lava_burst           = get_cooldown( "lava_burst"            );
    cooldown.shock                = get_cooldown( "shock"                 );
    cooldown.strike               = get_cooldown( "strike"                );
    cooldown.windfury_weapon      = get_cooldown( "windfury_weapon"       );

    // Weapon Enchants
    windfury_mh    = 0;
    windfury_oh    = 0;
    flametongue_mh = 0;
    flametongue_oh = 0;

    create_options();
  }

  // Character Definition
  virtual void      init();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      moving();
  virtual double    composite_attack_speed();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_power( school_e school );
  virtual double    composite_spell_power_multiplier();
  virtual double    composite_player_multiplier( school_e school, action_t* a = NULL );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource() { return RESOURCE_MANA; }
  virtual role_e primary_role();
  virtual void      combat_begin();
  virtual void      arise();

  virtual shaman_td_t* get_target_data( player_t* target )
  {
    shaman_td_t*& td = target_data[ target ];
    if( ! td ) td = new shaman_td_t( target, this );
    return td;
  }

  // Event Tracking
  virtual void regen( timespan_t periodicity );

  // Temporary
  virtual std::string set_default_talents()
  {
    switch ( primary_tree() )
    {
    case SHAMAN_ELEMENTAL:   return "131330"; break;
    case SHAMAN_ENHANCEMENT: return "131330"; break;
    default: break;    
    }

    return player_t::set_default_talents();
  }

  virtual std::string set_default_glyphs()
  {
    switch ( primary_tree() )
    {
    case SHAMAN_ELEMENTAL:   return "fire_elemental_totem/flame_shock/thunder/thunderstorm";
    case SHAMAN_ENHANCEMENT: return "ghost_wolf/lightning_shield/chain_lightning/astral_recall/renewed_life/lava_lash";
    default: break;
    }

    return player_t::set_default_glyphs();
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_melee_attack_t : public melee_attack_t
{
  bool windfury;
  bool flametongue;

  shaman_melee_attack_t( const std::string& token, shaman_t* p, const spell_data_t* s ) :
    melee_attack_t( token, p, s ),
    windfury( true ), flametongue( true )
  {
    special = true;
    may_crit = true;
    may_glance = false;
  }

  shaman_melee_attack_t( shaman_t* p, const spell_data_t* s ) :
    melee_attack_t( "", p, s ),
    windfury( true ), flametongue( true )
  {
    special = true;
    may_crit = true;
    may_glance = false;
  }

  shaman_t* p() { return static_cast< shaman_t* >( player ); }

  shaman_td_t* targetdata( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  virtual void execute();
  virtual void impact_s( action_state_t* );
  virtual double cost();
  virtual double cost_reduction();
};

// ==========================================================================
// Shaman Spell
// ==========================================================================


struct shaman_spell_state_t : public action_state_t
{
  bool eoe_proc;

  shaman_spell_state_t( action_t* spell, player_t* target ) :
    action_state_t( spell, target ),
    eoe_proc( false )
  { }
};

struct shaman_spell_t : public spell_t
{
  double   base_cost_reduction;
  bool     maelstrom;
  bool     overload;
  bool     is_totem;

  // Echo of Elements stuff
  stats_t* eoe_stats;

  shaman_spell_t( const std::string& token, shaman_t* p,
                  const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    spell_t( token, p, s ),
    base_cost_reduction( 0 ), maelstrom( false ), overload( false ), is_totem( false ),
    eoe_stats( 0 )
  {
    parse_options( 0, options );

    may_crit  = true;

    crit_bonus_multiplier *= 1.0 + p -> specialization.elemental_fury -> effect1().percent();
  }

  shaman_spell_t( shaman_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    spell_t( "", p, s ),
    base_cost_reduction( 0 ), maelstrom( false ), overload( false ), is_totem( false ),
    eoe_stats( 0 )
  {
    parse_options( 0, options );

    may_crit  = true;

    crit_bonus_multiplier *= 1.0 + p -> specialization.elemental_fury -> effect1().percent();
  }

  shaman_t* p() { return static_cast< shaman_t* >( player ); }

  shaman_td_t* targetdata( player_t* t = 0 ) { return p() -> get_target_data( t ? t : target ); }

  action_state_t* new_state() { return new shaman_spell_state_t( this, target ); }
  virtual bool   is_direct_damage() { return base_dd_min > 0 && base_dd_max > 0; }
  virtual bool   is_periodic_damage() { return base_td > 0; };
  virtual double cost();
  virtual double cost_reduction();
  virtual double haste() { return composite_haste(); }
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   impact_s( action_state_t* );
  virtual void   schedule_execute();
  virtual bool   usable_moving()
  {
    if ( p() -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero() )
      return true;

    return spell_t::usable_moving();
  }

  virtual double composite_target_crit( player_t* target )
  {
    double c = spell_t::composite_target_crit( target );

    shaman_td_t* td = targetdata( target );
    if ( school == SCHOOL_NATURE && td -> debuffs_stormstrike -> up() )
      c += td -> debuffs_stormstrike -> data().effectN( 1 ).percent();

    return c;
  }

  virtual double composite_haste()
  {
    double h = spell_t::composite_haste();

    if ( p() -> buff.elemental_mastery -> up() )
      h *= 1.0 / ( 1.0 + p() -> buff.elemental_mastery -> data().effectN( 1 ).percent() );

    if ( p() -> buff.tier13_4pc_healer -> up() )
      h *= 1.0 / ( 1.0 + p() -> buff.tier13_4pc_healer -> data().effect1().percent() );

    if ( p() -> talent.ancestral_swiftness -> ok() )
      h *= 1.0 / 1.05;

    return h;
  }

  virtual double composite_da_multiplier()
  {
    double m = spell_t::composite_da_multiplier();

    if ( maelstrom && p() -> buff.maelstrom_weapon -> stack() > 0 )
      m *= 1.0 + p() -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 1 ).percent();

    if ( p() -> buff.elemental_focus -> up() )
      m *= 1.0 + p() -> buff.elemental_focus -> data().effectN( 2 ).percent();

    return m;
  }

  action_state_t* get_state( const action_state_t* o )
  {
    action_state_t* s = spell_t::get_state( o );
    static_cast< shaman_spell_state_t* >( s ) -> eoe_proc = false;
    return s;
  }
  
  void init()
  {
    spell_t::init();

    eoe_stats = p() -> get_stats( name_str + "_eoe", this );
    eoe_stats -> school = school;
  }
};

struct eoe_execute_event_t : public event_t
{
  shaman_spell_t* spell;

  eoe_execute_event_t( shaman_spell_t* s ) :
    event_t( s -> sim, s -> player, "eoe_execute" ),
    spell( s )
  {
    timespan_t delay_duration = sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev );
    sim -> add_event( this, delay_duration );
  }

  void execute()
  {
    assert( spell );

    // EoE proc re-executes the "effect" with the same snapshot stats
    shaman_spell_state_t* ss = static_cast< shaman_spell_state_t* >( spell -> get_state( spell -> execute_state ) );
    ss -> eoe_proc = true;
    ss -> result = spell -> calculate_result( ss -> composite_crit(),
                                              ss -> target -> level );
    if ( spell -> result_is_hit( ss -> result ) )
      ss -> result_amount = spell -> calculate_direct_damage( ss -> result, 0,
                                                              ss -> target -> level,
                                                              ss -> attack_power,
                                                              ss -> spell_power,
                                                              ss -> composite_da_multiplier() );

    spell -> eoe_stats -> add_execute( timespan_t::zero() );
    spell -> schedule_travel_s( ss );
  }
};

// ==========================================================================
// Pet Spirit Wolf
// ==========================================================================

struct feral_spirit_pet_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( feral_spirit_pet_t* player ) :
      melee_attack_t( "wolf_melee", player, spell_data_t::nil() )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      school      = SCHOOL_PHYSICAL;

      // Two wolves
      base_multiplier  *= 2.0;
      // TODO: Wolves get no weapon speed based bonus to damage?
      weapon_power_mod /= player -> main_hand_weapon.swing_time.total_seconds();
    }

    feral_spirit_pet_t* p() { return static_cast<feral_spirit_pet_t*>( player ); }

    virtual void execute()
    {
      shaman_t* o = p() -> o();

      melee_attack_t::execute();

      // Two independent chances to proc it since we model 2 wolf pets as 1 ..
      if ( result_is_hit() )
      {
        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effect1().percent() ) )
        {
          int mwstack = o -> buff.maelstrom_weapon -> check();
          if ( o -> buff.maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buff.maelstrom_weapon -> max_stack() )
              o -> proc.wasted_mw -> occur();

            o -> proc.maelstrom_weapon -> occur();
          }
        }

        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effect1().percent() ) )
        {
          int mwstack = o -> buff.maelstrom_weapon -> check();
          if ( o -> buff.maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buff.maelstrom_weapon -> max_stack() )
              o -> proc.wasted_mw -> occur();

            o -> proc.maelstrom_weapon -> occur();
          }
        }
      }
    }
  };

  melee_t* melee;

  feral_spirit_pet_t( sim_t* sim, shaman_t* owner ) :
    pet_t( sim, owner, "feral_spirit" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 655; // Level 85 Values, approximated
    main_hand_weapon.max_dmg    = 934;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
  }

  shaman_t* o() { return static_cast<shaman_t*>( owner ); }

  virtual void init_base()
  {
    pet_t::init_base();

    // New approximated pet values at 85, roughly the same ratio of str/agi as per the old ones
    // At 85, the wolf has a base attack power of 932
    base.attribute[ ATTR_STRENGTH  ] = 407;
    base.attribute[ ATTR_AGILITY   ] = 138;
    base.attribute[ ATTR_STAMINA   ] = 361;
    base.attribute[ ATTR_INTELLECT ] = 90; // Pet has 90 spell damage :)
    base.attribute[ ATTR_SPIRIT    ] = 109;

    base.attack_power = -20;
    initial.attack_power_per_strength = 2.0;

    melee = new melee_t( this );
  }

  virtual double composite_attack_power()
  {
    double ap           = pet_t::composite_attack_power();
    double ap_per_owner = 0.4944;

    return ap + ap_per_owner * o() -> composite_attack_power_multiplier() * o() -> composite_attack_power();
  }

  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    melee -> execute(); // Kick-off repeating attack
  }

  virtual double composite_attribute_multiplier( attribute_e attr )
  {
    if ( attr == ATTR_STRENGTH || attr == ATTR_AGILITY )
      return 1.0;

    return player_t::composite_attribute_multiplier( attr );
  }

  virtual double composite_attack_power_multiplier()
  {
    return current.attack_power_multiplier;
  }

  virtual double composite_player_multiplier( school_e, action_t* )
  {
    return 1.0;
  }
};

struct earth_elemental_pet_t : public pet_t
{
  double owner_sp;

  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual bool usable_moving() { return true; }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      assert( player -> main_hand_weapon.type != WEAPON_NONE );
      player -> main_hand_attack = new melee_t( player );

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player -> is_moving() ) return false;
      return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct melee_t : public melee_attack_t
  {
    melee_t( earth_elemental_pet_t* player ) :
      melee_attack_t( "earth_melee", player, spell_data_t::nil() )
    {
      school = SCHOOL_PHYSICAL;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon            = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_power_mod  = 0.098475 / base_execute_time.total_seconds();

      base_attack_power_multiplier = 0;
    }

    virtual double swing_haste()
    {
      return 1.0;
    }

    virtual double    available() { return sim -> max_time.total_seconds(); }

    // Earth elemental scales purely with spell power
    virtual double total_attack_power()
    {
      return player -> composite_spell_power( SCHOOL_MAX );
    }

    // Melee swings have a ~3% crit rate on boss level mobs
    virtual double crit_chance( int )
    {
      return 0.03;
    }

    virtual double crit_chance( double, int )
    {
      return 0.03;
    }
  };

  earth_elemental_pet_t( sim_t* sim, shaman_t* owner ) :
    pet_t( sim, owner, "earth_elemental", true /*GUARDIAN*/ ), owner_sp( 0.0 )
  {
    stamina_per_owner   = 1.0;

    // Approximated from lvl 85 earth elemental
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 370; // Level 85 Values, approximated
    main_hand_weapon.max_dmg    = 409;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
  }

  virtual void init_base()
  {
    pet_t::init_base();

    resources.base[ RESOURCE_HEALTH ] = 8000; // Approximated from lvl85 earth elemental in game
    resources.base[ RESOURCE_MANA   ] = 0; //

    // Simple as it gets, travel to target, kick off melee
    action_list_str = "travel/auto_attack,moving=0";
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  virtual void regen( timespan_t /* periodicity */ ) { }

  virtual void summon( timespan_t /* duration */ )
  {
    pet_t::summon();
    owner_sp = owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier();
  }

  virtual double composite_spell_power( school_e )
  {
    return owner_sp;
  }

  virtual double composite_attack_power()
  {
    return 0.0;
  }

  virtual double composite_attack_hit()
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise( weapon_t* )
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }


  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "auto_attack" ) return new auto_melee_attack_t ( this );

    return pet_t::create_action( name, options_str );
  }
};

struct fire_elemental_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> current.distance = 1; }
    virtual timespan_t execute_time() { return timespan_t::from_seconds( player -> current.distance / 10.0 ); }
    virtual bool ready() { return ( player -> current.distance > 1 ); }
    virtual timespan_t gcd() { return timespan_t::zero(); }
    virtual bool usable_moving() { return true; }
  };

  struct fire_elemental_spell_t : public spell_t
  {
    fire_elemental_t* p;
    
    fire_elemental_spell_t( const std::string& t, fire_elemental_t* p, const spell_data_t* s = spell_data_t::nil() ) :
      spell_t( t, p, s ), p( p )
    {
      school    = SCHOOL_FIRE;
      stateless = true;
      crit_bonus_multiplier *= 1.0 + p -> o() -> specialization.elemental_fury -> effectN( 1 ).percent();
    }
  };
/*
  struct fire_shield_t : public fire_elemental_spell_t
  {
    fire_shield_t( fire_elemental_pet_t* player ) :
      fire_elemental_spell_t( "fire_shield", player )
    {
      aoe                       = -1;
      background                = true;
      repeating                 = true;
      may_crit                  = true;
      base_execute_time         = timespan_t::from_seconds( 3.0 );
      base_dd_min = base_dd_max = 89;
      direct_power_mod          = player -> dbc.spell( 13376 ) -> effect1().coeff();
    }

    virtual void execute()
    {
      if ( player -> current.distance <= 11.0 )
        fire_elemental_spell_t::execute();
      else // Out of range, just re-schedule it
        schedule_execute();
    }
  };

  struct fire_nova_t : public fire_elemental_spell_t
  {
    fire_nova_t( fire_elemental_pet_t* player ) :
      fire_elemental_spell_t( "fire_nova", player )
    {
      aoe                  = -1;
      may_crit             = true;
      direct_power_mod     = player -> dbc.spell( 12470 ) -> effect1().coeff();
      cooldown -> duration = timespan_t::from_seconds( player -> rng_ability_cooldown -> range( 30.0, 60.0 ) );

      // 207 = 80
      base_costs[ RESOURCE_MANA ]            = player -> level * 2.750;
      // For now, model the cast time increase as well, see below
      base_execute_time    = player -> dbc.spell( 12470 ) -> cast_time( player -> level );

      base_dd_min          = 583;
      base_dd_max          = 663;
    }

    virtual void execute()
    {
      fire_elemental_pet_t* fe = static_cast< fire_elemental_pet_t* >( player );
      // Randomize next cooldown duration here
      cooldown -> duration = timespan_t::from_seconds( fe -> rng_ability_cooldown -> range( 30.0, 60.0 ) );

      fire_elemental_spell_t::execute();
    }
  };
*/
  struct fire_blast_t : public fire_elemental_spell_t
  {
    fire_blast_t( fire_elemental_t* player ) :
      fire_elemental_spell_t( "fire_blast", player, player -> find_spell( 57984 ) )
    {
      may_crit                    = true;
      base_costs[ RESOURCE_MANA ] = 0;

      base_dd_min        = 1 + ( p -> o() -> level - 10 );
      base_dd_max        = 1 + ( p -> o() -> level - 10 ) + 1;
    }
    
    virtual void execute()
    {
      fire_elemental_spell_t::execute();

      // Reset swing timer
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
        player -> main_hand_attack -> execute_event -> reschedule( player -> main_hand_attack -> execute_time() );
    }

    virtual bool usable_moving()
    {
      return true;
    }
  };

  struct fire_melee_t : public melee_attack_t
  {
    fire_melee_t( fire_elemental_t* player ) :
      melee_attack_t( "fire_melee", player, spell_data_t::nil() )
    {
      special                      = false;
      may_crit                     = true;
      background                   = true;
      repeating                    = true;
      trigger_gcd                  = timespan_t::zero();
      direct_power_mod             = 1.0;
      base_spell_power_multiplier  = 1.0;
      base_attack_power_multiplier = 0.0;
      stateless                    = true;
      school                       = SCHOOL_FIRE;
      crit_bonus_multiplier       *= 1.0 + player -> o() -> specialization.elemental_fury -> effectN( 1 ).percent();
    }

    virtual void execute()
    {
      // If we're casting Fire Nova, we should clip a swing
      if ( time_to_execute > timespan_t::zero() && player -> executing )
        schedule_execute();
      else
        melee_attack_t::execute();
    }
    
    virtual void impact_s( action_state_t* state )
    {
      melee_attack_t::impact_s( state );
      
      fire_elemental_t* p = static_cast< fire_elemental_t* >( player );
      if ( p -> o() -> spec == SHAMAN_ENHANCEMENT )
      {
        shaman_td_t* td = p -> o() -> get_target_data( state -> target );
        td -> debuffs_searing_flames -> trigger();
      }
    }
  };

  struct auto_melee_attack_t : public melee_attack_t
  {
    auto_melee_attack_t( fire_elemental_t* player ) :
      melee_attack_t( "auto_attack", player )
    {
      player -> main_hand_attack = new fire_melee_t( player );
      player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero();
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player  -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  shaman_t* o() { return static_cast< shaman_t* >( owner ); }

  fire_elemental_t( sim_t* sim, shaman_t* owner, bool guardian ) :
    pet_t( sim, owner, "fire_elemental", guardian /*GUARDIAN*/ )
  {
    intellect_per_owner         = 0.0;
    stamina_per_owner           = 1.0;
  }

  virtual void init_base()
  {
    pet_t::init_base();
    
    resources.base[ RESOURCE_HEALTH ] = 32268; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 8908; // Level 85 value

    //mana_per_intellect               = 4.5;
    //hp_per_stamina = 10.5;

    main_hand_weapon.type            = WEAPON_BEAST;
    main_hand_weapon.min_dmg         = 307; // Level 85 Values, approximated
    main_hand_weapon.max_dmg         = 344;
    main_hand_weapon.damage          = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time      = timespan_t::from_seconds( 1.4 );

    // Run menacingly to the target, start meleeing and acting erratically
    //action_list_str                  = "travel/auto_attack/fire_nova/fire_blast";
    action_list_str                  = "travel/fire_blast/auto_attack";
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  virtual void regen( timespan_t periodicity )
  {
    if ( ! recent_cast() )
    {
      resource_gain( RESOURCE_MANA, 10.66 * periodicity.total_seconds() ); // FIXME! Does regen scale with gear???
    }
  }
  
  virtual double composite_player_multiplier( school_e school, action_t* a = 0 )
  {
    return pet_t::composite_player_multiplier( school, a ) * ( ( type == PLAYER_PET ) ? 1.5 : 1.0 );
  }

  virtual double composite_spell_power( school_e school )
  {
    return owner -> composite_spell_power( school ) * 0.55;
  }
  
  virtual double composite_attack_power()
  {
    return 0.0;
  }

  virtual double composite_spell_crit()
  {
    return owner -> composite_spell_crit();
  }
  
  virtual double composite_attack_crit( weapon_t* w = 0 )
  {
    return owner -> composite_attack_crit( w );
  }
  
  virtual double composite_spell_haste()
  {
    return owner -> composite_spell_haste();
  }
  
  virtual double composite_attack_haste()
  {
    return owner -> composite_attack_haste();
  }

  virtual double composite_attack_hit()
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise( weapon_t* )
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t     ( this );
//    if ( name == "fire_nova"   ) return new fire_nova_t  ( this );
    if ( name == "fire_blast"  ) return new fire_blast_t ( this );
    if ( name == "auto_attack" ) return new auto_melee_attack_t ( this );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    p -> flametongue_mh -> execute();
  else
    p -> flametongue_oh -> execute();
}

// trigger_windfury_weapon ==================================================

struct windfury_delay_event_t : public event_t
{
  shaman_melee_attack_t* wf;

  windfury_delay_event_t( shaman_melee_attack_t* wf, timespan_t delay ) :
    event_t( wf -> p() -> sim, wf -> p(), "windfury_delay_event" ), wf( wf )
  {
    sim -> add_event( this, delay );
  }

  virtual void execute()
  {
    shaman_t* p = wf -> p();

    p -> proc.windfury -> occur();
    wf -> execute();
    wf -> execute();
    wf -> execute();
  }
};

static bool trigger_windfury_weapon( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();
  shaman_melee_attack_t* wf = 0;

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    wf = p -> windfury_mh;
  else
    wf = p -> windfury_oh;

  if ( p -> cooldown.windfury_weapon -> remains() > timespan_t::zero() ) return false;

  if ( p -> rng.windfury_weapon -> roll( wf -> data().proc_chance() ) )
  {
    p -> cooldown.windfury_weapon -> start( p -> rng.windfury_delay -> gauss( timespan_t::from_seconds( 3.0 ), timespan_t::from_seconds( 0.3 ) ) );

    // Delay windfury by some time, up to about a second
    new ( p -> sim ) windfury_delay_event_t( wf, p -> rng.windfury_delay -> gauss( p -> wf_delay, p -> wf_delay_stddev ) );
    return true;
  }
  return false;
}

// trigger_rolling_thunder ==================================================

static bool trigger_rolling_thunder ( shaman_spell_t* s )
{
  shaman_t* p = s -> p();

  if ( ! p -> buff.lightning_shield -> check() )
    return false;

  if ( p -> rng.rolling_thunder -> roll( p -> specialization.rolling_thunder -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_MANA,
                        p -> dbc.spell( 88765 ) -> effect1().percent() * p -> resources.max[ RESOURCE_MANA ],
                        p -> gain.rolling_thunder );

    if ( p -> buff.lightning_shield -> check() == p -> buff.lightning_shield -> max_stack() )
      p -> proc.wasted_ls -> occur();

    p -> buff.lightning_shield -> trigger();
    p -> proc.rolling_thunder  -> occur();
    return true;
  }

  return false;
}

// trigger_static_shock =====================================================

static bool trigger_static_shock ( shaman_melee_attack_t* a )
{
  shaman_t* p = a -> p();

  if ( ! p -> buff.lightning_shield -> stack() )
    return false;

  if ( p -> rng.static_shock -> roll( p -> specialization.static_shock -> proc_chance() ) )
  {
    p -> active_lightning_charge -> execute();
    p -> proc.static_shock -> occur();
    return true;
  }
  return false;
}

// trigger_improved_lava_lash ===============================================

static bool trigger_improved_lava_lash( shaman_melee_attack_t* a )
{
  struct improved_lava_lash_t : public shaman_spell_t
  {
    rng_t* imp_ll_rng;
    cooldown_t* imp_ll_fs_cd;
    stats_t* fs_dummy_stat;

    improved_lava_lash_t( shaman_t* p ) :
      shaman_spell_t( "improved_lava_lash", p ),
      imp_ll_rng( 0 ), imp_ll_fs_cd( 0 )
    {
      aoe = 4;
      may_miss = may_crit = false;
      proc = true;
      callbacks = false;
      background = true;
      stateless = true;

      imp_ll_rng = sim -> get_rng( "improved_ll" );
      imp_ll_fs_cd = player -> get_cooldown( "improved_ll_fs_cooldown" );
      imp_ll_fs_cd -> duration = timespan_t::zero();
      fs_dummy_stat = player -> get_stats( "flame_shock_dummy" );
    }

    // Exclude targets with your flame shock on
    size_t available_targets( std::vector< player_t* >& tl )
    {
      for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
      {
        if ( sim -> actor_list[ i ] -> current.sleeping )
          continue;

        if ( ! sim -> actor_list[ i ] -> is_enemy() )
          continue;

        if ( sim -> actor_list[ i ] == target )
          continue;

        shaman_td_t* td = targetdata( sim -> actor_list[ i ] );

        if ( td -> dots_flame_shock -> ticking )
          continue;

        tl.push_back( sim -> actor_list[ i ] );
      }

      return tl.size();
    }

    std::vector< player_t* > target_list()
    {
      std::vector< player_t* > t;

      size_t total_targets = available_targets( t );

      // Reduce targets to aoe amount by removing random entries from the
      // target list until it's at aoe amount
      while ( total_targets > static_cast< size_t >( aoe ) )
      {
        t.erase( t.begin() + static_cast< size_t >( imp_ll_rng -> range( 0, total_targets ) ) );
        total_targets--;
      }

      return t;
    }

    // Impact on any target triggers a flame shock; for now, cache the
    // relevant parts of the spell and return them after execute has finished
    void impact_s( action_state_t* state )
    {
      if ( sim -> debug )
        sim -> output( "%s spreads Flame Shock (off of %s) on %s",
                       player -> name(),
                       target -> name(),
                       state -> target -> name() );

      shaman_t* p = this -> p();

      double dd_min = p -> action_flame_shock -> base_dd_min,
             dd_max = p -> action_flame_shock -> base_dd_max,
             coeff = p -> action_flame_shock -> direct_power_mod,
             real_base_cost = p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ];
      player_t* original_target = p -> action_flame_shock -> target;
      cooldown_t* original_cd = p -> action_flame_shock -> cooldown;
      stats_t* original_stats = p -> action_flame_shock -> stats;

      p -> action_flame_shock -> base_dd_min = 0;
      p -> action_flame_shock -> base_dd_max = 0;
      p -> action_flame_shock -> direct_power_mod = 0;
      p -> action_flame_shock -> background = true;
      p -> action_flame_shock -> callbacks = false;
      p -> action_flame_shock -> proc = true;
      p -> action_flame_shock -> may_crit = false;
      p -> action_flame_shock -> may_miss = false;
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = 0;
      p -> action_flame_shock -> target = state -> target;
      p -> action_flame_shock -> cooldown = imp_ll_fs_cd;
      p -> action_flame_shock -> stats = fs_dummy_stat;

      p -> action_flame_shock -> execute();

      p -> action_flame_shock -> base_dd_min = dd_min;
      p -> action_flame_shock -> base_dd_max = dd_max;
      p -> action_flame_shock -> direct_power_mod = coeff;
      p -> action_flame_shock -> background = false;
      p -> action_flame_shock -> callbacks = true;
      p -> action_flame_shock -> proc = false;
      p -> action_flame_shock -> may_crit = true;
      p -> action_flame_shock -> may_miss = true;
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = real_base_cost;
      p -> action_flame_shock -> target = original_target;
      p -> action_flame_shock -> cooldown = original_cd;
      p -> action_flame_shock -> stats = original_stats;

      // Hide the Flame Shock dummy stat and improved_lava_lash from reports
      fs_dummy_stat -> num_executes = 0;
      stats -> num_executes = 0;

      release_state( state );
    }
  };

  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( a -> sim -> num_enemies == 1 )
    return false;

  shaman_td_t* t = a -> targetdata( a -> target );

  if ( ! t -> dots_flame_shock -> ticking )
    return false;

  shaman_t* p = a -> p();

  if ( p -> glyph.lava_lash -> ok() )
    return false;

  if ( ! p -> action_flame_shock )
  {
    p -> action_flame_shock = p -> find_action( "flame_shock" );
    assert( p -> action_flame_shock );
  }

  if ( ! p -> action_improved_lava_lash )
  {
    p -> action_improved_lava_lash = new improved_lava_lash_t( p );
    p -> action_improved_lava_lash -> init();
  }

  // Splash from the action's target
  p -> action_improved_lava_lash -> target = a -> target;
  p -> action_improved_lava_lash -> execute();

  return true;
}

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct lava_burst_overload_t : public shaman_spell_t
{
  lava_burst_overload_t( shaman_t* player, bool dtr = false ) :
    shaman_spell_t( "lava_burst_overload", player, player -> dbc.spell( 77451 ) )
  {
    overload             = true;
    background           = true;
    stateless            = true;
    base_execute_time    = timespan_t::zero();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double composite_target_crit( player_t* target )
  {
    if ( targetdata( target ) -> dots_flame_shock -> ticking )
      return 1.0;
    else
      return shaman_spell_t::composite_target_crit( target );
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( shaman_t* player, bool dtr = false ) :
    shaman_spell_t( "lightning_bolt_overload", player, player -> dbc.spell( 45284 ) )
  {
    overload             = true;
    background           = true;
    stateless            = true;
    base_execute_time    = timespan_t::zero();

    direct_power_mod  += player -> specialization.shamanism -> effectN( 1 ).percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      trigger_rolling_thunder( this );
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  int glyph_targets;

  chain_lightning_overload_t( shaman_t* player, bool dtr = false ) :
    shaman_spell_t( "chain_lightning_overload", player, player -> dbc.spell( 45297 ) ),
    glyph_targets( 0 )
  {
    overload             = true;
    background           = true;
    stateless            = true;
    base_execute_time    = timespan_t::zero();

    direct_power_mod  += player -> specialization.shamanism -> effectN( 2 ).percent();

    aoe = 2;
    if ( player -> glyph.chain_lightning -> ok() )
    {
      base_multiplier *= 1.0 + player -> glyph.chain_lightning -> effectN( 2 ).percent();
      aoe             += ( int ) player -> glyph.chain_lightning -> effectN( 1 ).base_value();
    }

    base_add_multiplier = data().effectN( 1 ).chain_multiplier();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      trigger_rolling_thunder( this );
  }
};

struct searing_flames_t : public shaman_spell_t
{
  searing_flames_t( shaman_t* player ) :
    shaman_spell_t( player, player -> find_spell( 77661 ) )
  {
    background       = true;
    may_miss         = false;
    tick_may_crit    = true;
    proc             = true;
    dot_behavior     = DOT_REFRESH;
    hasted_ticks     = true;
    may_crit         = true;
    stateless        = true;
    update_flags     = 0;

    // Override spell data values, as they seem wrong, instead
    // make searing totem calculate the damage portion of the
    // spell, without using any tick power modifiers or
    // player based multipliers here
    base_td          = 0.0;
    tick_power_mod   = 0.0;
  }

  virtual void init()
  {
    shaman_spell_t::init();

    // Override snapshot flags
    snapshot_flags = STATE_HASTE | STATE_CRIT | STATE_TGT_CRIT | STATE_TGT_MUL_TA;
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    return targetdata( target ) -> debuffs_searing_flames -> stack();
  }

  void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );
    targetdata( target ) -> debuffs_searing_flames -> expire();
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  int threshold;

  lightning_charge_t( const std::string& n, shaman_t* player, bool dtr = false ) :
    shaman_spell_t( n, player, player -> dbc.spell( 26364 ) ),
    threshold( static_cast< int >( player -> specialization.fulmination -> effectN( 1 ).base_value() ) )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    background       = true;
    may_crit         = true;
    stateless        = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_charge_t( n, player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double composite_da_multiplier()
  {
    double m = shaman_spell_t::composite_da_multiplier();

    if ( threshold > 0 )
    {
      int consuming_stack =  p() -> buff.lightning_shield -> check() - threshold;
      if ( consuming_stack > 0 )
        m *= consuming_stack;
    }

    return m;
  }
};

struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( shaman_t* player ) :
    shaman_spell_t( "unleash_flame", player, player -> dbc.spell( 73683 ) )
  {
    harmful              = true;
    background           = true;
    //proc                 = true;
    stateless            = true;

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = timespan_t::zero();
  }

  virtual void execute()
  {
    if ( p() -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }

    if ( p() -> off_hand_weapon.type != WEAPON_NONE &&
         p() -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_spell_t( n, player, player -> dbc.spell( 8024 ) )
  {
    may_crit           = true;
    background         = true;
    proc               = true;
    stateless          = true;
    direct_power_mod   = 1.0;
    base_costs[ RESOURCE_MANA ] = 0.0;

    base_dd_min = w -> swing_time.total_seconds() / 4.0 * player -> dbc.effect_min( data().effectN( 2 ).id(), player -> level ) / 25.0;
    base_dd_max = base_dd_min;

    if ( player -> primary_tree() == SHAMAN_ENHANCEMENT )
    {
      snapshot_flags               = STATE_AP;
      base_attack_power_multiplier = w -> swing_time.total_seconds() / 4.0 * 0.1253;
      base_spell_power_multiplier  = 0;
    }
    else
    {
      base_attack_power_multiplier = 0;
      base_spell_power_multiplier  = w -> swing_time.total_seconds() / 4.0 * 0.1253;
    }
  }

  virtual double composite_attack_power()
  {
    double ap = shaman_spell_t::composite_attack_power();

    ap += p() -> composite_attack_power();

    return ap;
  }

  virtual double composite_attack_power_multiplier()
  {
    double m = shaman_spell_t::composite_attack_power_multiplier();

    m *= p() -> composite_attack_power_multiplier();

    return m;
  }
};

struct windfury_weapon_melee_attack_t : public shaman_melee_attack_t
{
  windfury_weapon_melee_attack_t( const std::string& n, shaman_t* player, weapon_t* w ) :
    shaman_melee_attack_t( n, player, player -> dbc.spell( 33757 ) )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    stats -> school  = SCHOOL_PHYSICAL;
    background       = true;
    callbacks        = false; // Windfury does not proc any On-Equip procs, apparently
    stateless        = true;
  }

  virtual double composite_attack_power()
  {
    double ap = shaman_melee_attack_t::composite_attack_power();

    return ap + weapon -> buff_value;
  }
};

struct unleash_wind_t : public shaman_melee_attack_t
{
  unleash_wind_t( shaman_t* player ) :
    shaman_melee_attack_t( "unleash_wind", player, player -> dbc.spell( 73681 ) )
  {
    background            = true;
    windfury              = false;
    may_dodge = may_parry = false;
    stateless             = true;

    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = timespan_t::zero();
  }

  void execute()
  {
    // Figure out weapons
    if ( p() -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p() -> main_hand_weapon );
      shaman_melee_attack_t::execute();
    }

    if ( p() -> off_hand_weapon.type != WEAPON_NONE &&
         p() -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p() -> off_hand_weapon );
      shaman_melee_attack_t::execute();
    }
  }
};

struct stormstrike_melee_attack_t : public shaman_melee_attack_t
{
  stormstrike_melee_attack_t( const std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    shaman_melee_attack_t( n, player, s )
  {
    background           = true;
    may_miss             = false;
    may_dodge            = false;
    may_parry            = false;
    weapon               = w;
    stateless            = true;
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_melee_attack_t::execute ===========================================

void shaman_melee_attack_t::execute()
{
  melee_attack_t::execute();

  p() -> buff.spiritwalkers_grace -> up();
}

void shaman_melee_attack_t::impact_s( action_state_t* state )
{
  melee_attack_t::impact_s( state );

  if ( result_is_hit( state -> result ) && ! proc )
  {
    int mwstack = p() -> buff.maelstrom_weapon -> check();
    // TODO: Chance is based on Rank 3, i.e., 10 PPM?
    double chance = weapon -> proc_chance_on_swing( 10.0 );

    if ( p() -> spec == SHAMAN_ENHANCEMENT && 
         p() -> buff.maelstrom_weapon -> trigger( 1, -1, chance ) )
    {
      if ( mwstack == p() -> buff.maelstrom_weapon -> max_stack() )
        p() -> proc.wasted_mw -> occur();

      p() -> proc.maelstrom_weapon -> occur();
    }

    if ( windfury && weapon -> buff_type == WINDFURY_IMBUE )
      trigger_windfury_weapon( this );

    if ( flametongue && weapon -> buff_type == FLAMETONGUE_IMBUE )
      trigger_flametongue_weapon( this );

    if ( p() -> rng.primal_wisdom -> roll( p() -> specialization.primal_wisdom -> proc_chance() ) )
    {
      double amount = p() -> spell.primal_wisdom -> effect1().percent() * p() -> resources.base[ RESOURCE_MANA ];
      p() -> resource_gain( RESOURCE_MANA, amount, p() -> gain.primal_wisdom );
    }
  }
}

// shaman_melee_attack_t::cost_reduction ==========================================

double shaman_melee_attack_t::cost_reduction()
{
  if ( p() -> buff.shamanistic_rage -> up() )
    return p() -> buff.shamanistic_rage -> data().effectN( 1 ).percent();

  return 0.0;
}

// shaman_melee_attack_t::cost ====================================================

double shaman_melee_attack_t::cost()
{
  double c = melee_attack_t::cost();
  c *= 1.0 + cost_reduction();
  if ( c < 0 ) c = 0.0;
  return c;
}

// Melee Attack =============================================================

struct melee_t : public shaman_melee_attack_t
{
  int sync_weapons;

  melee_t( const std::string& name, shaman_t* player, int sw ) :
    shaman_melee_attack_t( name, player, spell_data_t::nil() ), sync_weapons( sw )
  {
    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    stateless   = true;
    special     = false;
    may_glance  = true;
    school      = SCHOOL_PHYSICAL;

    if ( p() -> spec == SHAMAN_ENHANCEMENT && p() -> dual_wield() ) base_hit -= 0.19;
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = shaman_melee_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero() && p() -> executing )
    {
      if ( sim -> debug ) sim -> output( "Executing '%s' during melee (%s).", p() -> executing -> name(), util::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
    {
      shaman_melee_attack_t::execute();
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_melee_attack_t::impact_s( state );

    if ( result_is_hit( state -> result ) && p() -> buff.unleashed_fury_wf -> up() )
      trigger_static_shock( this );

    if ( state -> result == RESULT_CRIT )
      p() -> buff.flurry -> trigger( p() -> buff.flurry -> data().initial_stacks() );
  }

  void schedule_execute()
  {
    shaman_melee_attack_t::schedule_execute();
    // Clipped swings do not eat unleash wind buffs
    if ( time_to_execute > timespan_t::zero() && ! p() -> executing )
    {
      p() -> buff.unleash_wind -> decrement();
      p() -> buff.flurry       -> decrement();
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_melee_attack_t
{
  int sync_weapons;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( "auto_attack", player, spell_data_t::nil() ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( p() -> main_hand_weapon.type != WEAPON_NONE );
    p() -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p() -> main_hand_attack -> weapon = &( p() -> main_hand_weapon );
    p() -> main_hand_attack -> base_execute_time = p() -> main_hand_weapon.swing_time;

    if ( p() -> off_hand_weapon.type != WEAPON_NONE && p() -> spec == SHAMAN_ENHANCEMENT )
    {
      if ( ! p() -> dual_wield() ) return;
      p() -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p() -> off_hand_attack -> weapon = &( p() -> off_hand_weapon );
      p() -> off_hand_attack -> base_execute_time = p() -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    p() -> main_hand_attack -> schedule_execute();
    if ( p() -> off_hand_attack )
      p() -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( p() -> is_moving() ) return false;
    return ( p() -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_melee_attack_t
{
  double ft_bonus;
  double sf_bonus;

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( player, player -> find_class_spell( "Lava Lash" ) ),
    ft_bonus( data().effectN( 2 ).percent() ),
    sf_bonus( player -> spell.searing_flames -> effectN( 1 ).percent() )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    school = SCHOOL_FIRE;

    parse_options( NULL, options_str );

    stateless           = true;
    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background        = true; // Do not allow execution.
  }

  // Lava Lash multiplier calculation from
  // http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p11/#post1935780
  // MoP: Vastly simplified, most bonuses are gone
  virtual double composite_target_multiplier( player_t* target )
  {
    double m = shaman_melee_attack_t::composite_target_multiplier( target );

    m *= 1.0 + ( targetdata( target ) -> debuffs_searing_flames -> check() * sf_bonus ) +
               ( weapon -> buff_type == FLAMETONGUE_IMBUE ) * ft_bonus;

    return m;
  }

  void impact_s( action_state_t* state )
  {
    shaman_melee_attack_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      shaman_td_t* td = targetdata( state -> target );
      td -> debuffs_searing_flames -> expire();
      if ( p() -> active_searing_flames_dot )
        p() -> active_searing_flames_dot -> get_dot() -> cancel();

      trigger_static_shock( this );
      if ( td -> dots_flame_shock -> ticking )
        trigger_improved_lava_lash( this );
    }
  }
};

// Primal Strike Attack =====================================================

struct primal_strike_t : public shaman_melee_attack_t
{
  primal_strike_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( player, player -> find_class_spell( "Primal Strike" ) )
  {
    parse_options( NULL, options_str );

    stateless            = true;
    weapon               = &( p() -> main_hand_weapon );
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();
  }

  void impact_s( action_state_t* state )
  {
    shaman_melee_attack_t::impact_s( state );
    if ( result_is_hit( state -> result ) )
      trigger_static_shock( this );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_melee_attack_t
{
  stormstrike_melee_attack_t * stormstrike_mh;
  stormstrike_melee_attack_t * stormstrike_oh;

  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_melee_attack_t( "stormstrike", player, player -> find_class_spell( "Stormstrike" ) ),
    stormstrike_mh( 0 ), stormstrike_oh( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    parse_options( NULL, options_str );

    stateless            = true;
    weapon               = &( p() -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p() -> cooldown.strike;
    cooldown -> duration = p() -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_melee_attack_t
    // stormstrike_melee_attack_t( std::string& n, shaman_t* player, const spell_data_t* s, weapon_t* w ) :
    stormstrike_mh = new stormstrike_melee_attack_t( "stormstrike_mh", player, data().effectN( 2 ).trigger(), &( player -> main_hand_weapon ) );
    add_child( stormstrike_mh );

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_melee_attack_t( "stormstrike_oh", player, data().effectN( 3 ).trigger(), &( player -> off_hand_weapon ) );
      add_child( stormstrike_oh );
    }
  }

  void impact_s( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_melee_attack_t
    melee_attack_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      shaman_td_t* td = targetdata( state -> target );
      td -> debuffs_stormstrike -> trigger();

      stormstrike_mh -> execute();
      if ( stormstrike_oh ) stormstrike_oh -> execute();

      bool shock = trigger_static_shock( this );
      if ( !shock && stormstrike_oh ) trigger_static_shock( this );
    }
  }
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

// shaman_spell_t::cost_reduction ===========================================

double shaman_spell_t::cost_reduction()
{
  double cr = base_cost_reduction;

  if ( ( harmful || is_totem ) && p() -> buff.shamanistic_rage -> up() )
    cr += p() -> buff.shamanistic_rage -> data().effectN( 1 ).percent();

  if ( harmful && callbacks && ! proc && p() -> buff.elemental_focus -> up() )
    cr += p() -> buff.elemental_focus -> data().effectN( 1 ).percent();

  if ( ( execute_time() == timespan_t::zero() && ! harmful ) || harmful || is_totem )
    cr += p() -> specialization.mental_quickness -> effectN( 2 ).percent();

  return cr;
}

// shaman_spell_t::cost =====================================================

double shaman_spell_t::cost()
{
  double c = spell_t::cost();

  c *= 1.0 + cost_reduction();

  if ( c < 0 ) c = 0;
  return c;
}

// shaman_spell_t::consume_resource =========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  if ( harmful && callbacks && ! proc && resource_consumed > 0 && p() -> buff.elemental_focus -> up() )
    p() -> buff.elemental_focus -> decrement();
}

// shaman_spell_t::execute =================================================

void shaman_spell_t::execute()
{
  spell_t::execute();

  if ( ! is_totem && ! proc && school == SCHOOL_FIRE )
    p() -> buff.unleash_flame -> expire();

  // Record maelstrom weapon stack usage
  if ( maelstrom && p() -> primary_tree() == SHAMAN_ENHANCEMENT )
    p() -> proc.maelstrom_weapon_used[ p() -> buff.maelstrom_weapon -> check() ] -> occur();

  if ( harmful && ! proc && is_direct_damage() && ! is_dtr_action &&
       p() -> talent.echo_of_the_elements -> ok() &&
       p() -> rng.echo_of_the_elements -> roll( p() -> eoe_proc_chance ) )
  {
    if ( sim -> debug ) sim -> output( "Echo of the Elements procs for %s", name() );
    new ( sim ) eoe_execute_event_t( this );
  }

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero.
  if ( execute_time() > timespan_t::zero() )
  {
    if ( ! maelstrom || p() -> buff.maelstrom_weapon -> check() == 0 )
    {
      if ( sim -> debug )
      {
        sim -> output( "Resetting swing timers for '%s', maelstrom=%d, stacks=%d",
                       name_str.c_str(), maelstrom, p() -> buff.maelstrom_weapon -> check() );
      }

      timespan_t time_to_next_hit;

      // Non-maelstromable spell finishes casting, reset swing timers
      if ( p() -> main_hand_attack && p() -> main_hand_attack -> execute_event )
      {
        time_to_next_hit = p() -> main_hand_attack -> execute_time();
        p() -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }

      // Offhand
      if ( p() -> off_hand_attack && p() -> off_hand_attack -> execute_event )
      {
        time_to_next_hit = player -> off_hand_attack -> execute_time();
        p() -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// shaman_spell_t::impact ==================================================

void shaman_spell_t::impact_s( action_state_t* state )
{
  shaman_spell_state_t* ss = static_cast< shaman_spell_state_t* >( state );
  if ( ss -> eoe_proc )
  {
    stats_t* tmp_stats = stats;
    stats = eoe_stats;
    is_dtr_action = true;
    spell_t::impact_s( state );
    is_dtr_action = false;
    stats = tmp_stats;
  }
  else
    spell_t::impact_s( state );

  // Triggers wont happen for procs or totems
  if ( proc || ! callbacks )
    return;

  if ( is_direct_damage() && result_is_hit( state -> result ) && state -> result == RESULT_CRIT )
  {
    // Overloads dont trigger elemental focus
    if ( ! overload && p() -> primary_tree() == SHAMAN_ELEMENTAL )
      p() -> buff.elemental_focus -> trigger( p() -> buff.elemental_focus -> data().initial_stacks() );
  }
}

// shaman_spell_t::schedule_execute =========================================

void shaman_spell_t::schedule_execute()
{
  if ( sim -> log )
  {
    sim -> output( "%s schedules execute for %s", p() -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( ! background )
  {
    p() -> executing = this;
    p() -> gcd_ready = sim -> current_time + gcd();
    if ( p() -> action_queued && sim -> strict_gcd_queue )
    {
      p() -> gcd_ready -= sim -> queue_gcd_reduction;
    }
  }
}

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Bloodlust" ), options_str )
  {
    harmful = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> current.sleeping || p -> buffs.exhaustion -> check() || p -> type == PLAYER_GUARDIAN )
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( p() -> buffs.exhaustion -> check() )
      return false;

    if (  p() -> buffs.bloodlust -> cooldown -> remains() > timespan_t::zero() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ====================================================

struct chain_lightning_t : public shaman_spell_t
{
  int      glyph_targets;
  chain_lightning_overload_t* overload;

  chain_lightning_t( shaman_t* player, const std::string& options_str, bool dtr = false ) :
    shaman_spell_t( player, player -> find_class_spell( "Chain Lightning" ), options_str ),
    glyph_targets( 0 )
  {
    maelstrom             = true;
    base_execute_time    += p() -> specialization.shamanism        -> effectN( 3 ).time_value();
    cooldown -> duration += p() -> specialization.elemental_fury   -> effectN( 2 ).time_value();
    base_multiplier      += p() -> glyph.chain_lightning -> effectN( 2 ).percent() +
                            p() -> specialization.shamanism        -> effectN( 2 ).percent();
    aoe                   = ( 2 + ( int ) p() -> glyph.chain_lightning -> effectN( 1 ).base_value() );
    base_add_multiplier   = data().effectN( 1 ).chain_multiplier();

    overload              = new chain_lightning_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_t( p(), options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    p() -> buff.maelstrom_weapon -> expire();

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      double overload_chance = p() -> composite_mastery() *
                               p() -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0 / 3.0;

      if ( overload_chance && p() -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p() -> set_bonus.tier13_4pc_caster() )
          p() -> buff.tier13_4pc_caster -> trigger();
      }
    }
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = shaman_spell_t::execute_time();
    t *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> buff.maelstrom_weapon -> data().effectN( 1 ).percent();
    return t;
  }

  double cost_reduction()
  {
    double cr = shaman_spell_t::cost_reduction();
    cr += p() -> buff.maelstrom_weapon -> stack() * p() -> buff.maelstrom_weapon -> data().effectN( 2 ).percent();
    return cr;
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_talent_spell( "Elemental Mastery" ), options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.elemental_mastery -> trigger();
    if ( p() -> set_bonus.tier13_2pc_caster() )
      p() -> buff.tier13_2pc_caster -> trigger();
  }
};

// Call of the Elements Spell ===============================================

struct call_of_the_elements_t : public shaman_spell_t
{
  call_of_the_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_talent_spell( "Call of the Elements" ), options_str )
  {
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    // For now, reset earth / fire elemental totem cooldowns

    if ( p() -> cooldown.earth_elemental -> duration < timespan_t::from_seconds( 5 * 60.0 ) )
      p() -> cooldown.earth_elemental -> reset();

    if ( p() -> cooldown.fire_elemental -> duration < timespan_t::from_seconds( 5 * 60.0 ) )
      p() -> cooldown.fire_elemental -> reset();
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  player_t* emit_target;

  fire_nova_explosion_t( shaman_t* player ) :
    shaman_spell_t( "fire_nova_explosion", player, player -> find_spell( 8349 ) ),
    emit_target( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    aoe        = -1;
    background = true;
    callbacks  = false;
    stateless  = true;
  }

  double action_da_multiplier()
  {
    if ( p() -> buff.unleash_flame -> up() )
      return p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return 0.0;
  }

  // Fire nova does not damage the main target.
  size_t available_targets( std::vector< player_t* >& tl )
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> current.sleeping &&
             sim -> actor_list[ i ] -> is_enemy() &&
             sim -> actor_list[ i ] != emit_target )
        tl.push_back( sim -> actor_list[ i ] );
    }

    return tl.size();
  }

  void reset()
  {
    shaman_spell_t::reset();

    emit_target = 0;
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_explosion_t* explosion;

  fire_nova_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Fire Nova" ), options_str ),
    explosion( 0 )
  {
    aoe       = -1;
    may_crit  = false;
    may_miss  = false;
    callbacks = false;
    explosion = new fire_nova_explosion_t( player );
    stateless = true;

    add_child( explosion );
  }

  virtual bool ready()
  {
    shaman_td_t* td = targetdata( target );

    if ( ! td -> dots_flame_shock -> ticking )
      return false;

    return shaman_spell_t::ready();
  }

  void impact_s( action_state_t* state )
  {
    explosion -> emit_target = state -> target;
    explosion -> execute();
  }

  // Fire nova is emitted on all targets with a flame shock from us .. so
  std::vector< player_t* > target_list()
  {
    std::vector< player_t* > t;

    for ( player_t* e = sim -> target_list; e; e = e -> next )
    {
      if ( e -> current.sleeping || ! e -> is_enemy() )
        continue;

      shaman_td_t* td = targetdata( e );
      if ( td -> dots_flame_shock -> ticking )
        t.push_back( e );
    }

    return t;
  }
};

// Earthquake Spell =========================================================

struct earthquake_rumble_t : public shaman_spell_t
{
  earthquake_rumble_t( shaman_t* player ) :
    shaman_spell_t( "earthquake_rumble", player, player -> find_spell( 77478 ) )
  {
    harmful = true;
    aoe = -1;
    background = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
    callbacks = false;
    stateless = true;
  }

  virtual double composite_spell_power()
  {
    double sp = shaman_spell_t::composite_spell_power();

    sp += player -> composite_spell_power( SCHOOL_NATURE );

    return sp;
  }

  double armor()
  {
    return 0.0;
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_rumble_t* quake;

  earthquake_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Earthquake" ), options_str ),
    quake( 0 )
  {
    base_td = 0;
    base_dd_min = base_dd_max = direct_power_mod = 0;
    harmful = true;
    may_miss = false;
    may_miss = may_crit = may_dodge = may_parry = false;
    num_ticks = ( int ) data().duration().total_seconds();
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = false;
    stateless = true;

    quake = new earthquake_rumble_t( player );

    add_child( quake );
  }

  void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    quake -> execute();
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  lava_burst_overload_t* overload;

  lava_burst_t( shaman_t* player, const std::string& options_str, bool dtr = false ) :
    shaman_spell_t( player, player -> find_class_spell( "Lava Burst" ), options_str )
  {
    stateless        = true;
    overload         = new lava_burst_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double action_multiplier()
  {
    double m = shaman_spell_t::action_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m += p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  virtual double action_da_multiplier()
  {
    double m = shaman_spell_t::action_da_multiplier();

    shaman_td_t* td = targetdata( target );
    if ( td -> debuffs_unleashed_fury_ft -> up() )
      m *= 1.0 + td -> debuffs_unleashed_fury_ft -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_crit( player_t* target )
  {
    if ( targetdata( target ) -> dots_flame_shock -> ticking )
      return 1.0;
    else
      return shaman_spell_t::composite_target_crit( target );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if ( p() -> buff.lava_surge -> check() )
      p() -> buff.lava_surge -> expire();
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buff.lava_surge -> up() )
      return timespan_t::zero();

    return shaman_spell_t::execute_time();
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      double overload_chance = p() -> composite_mastery() *
                               p() -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0;

      if ( overload_chance && p() -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p() -> set_bonus.tier13_4pc_caster() )
          p() -> buff.tier13_4pc_caster -> trigger();
      }
    }
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  lightning_bolt_overload_t* overload;

  lightning_bolt_t( shaman_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( player, player -> find_class_spell( "Lightning Bolt" ), options_str )
  {
    maelstrom          = true;
    stateless          = true;
    base_multiplier   += player -> glyph.telluric_currents -> effectN( 1 ).percent() +
                         player -> specialization.shamanism -> effectN( 1 ).percent();
    base_execute_time += player -> specialization.shamanism -> effectN( 3 ).time_value();
    base_execute_time *= 1.0 + player -> glyph.unleashed_lightning -> effectN( 2 ).percent();
    overload           = new lightning_bolt_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double action_da_multiplier()
  {
    double m = shaman_spell_t::action_da_multiplier();

    shaman_td_t* td = targetdata( target );
    if ( td -> debuffs_unleashed_fury_ft -> up() )
      m *= 1.0 + td -> debuffs_unleashed_fury_ft -> data().effectN( 1 ).percent();

    return m;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.maelstrom_weapon -> expire();
  }

  virtual timespan_t execute_time()
  {
    timespan_t t = shaman_spell_t::execute_time();
    t *= 1.0 + p() -> buff.maelstrom_weapon -> stack() * p() -> buff.maelstrom_weapon -> data().effectN( 1 ).percent();
    return t;
  }

  virtual double cost_reduction()
  {
    double cr = shaman_spell_t::cost_reduction();
    cr += p() -> buff.maelstrom_weapon -> stack() * p() -> buff.maelstrom_weapon -> data().effectN( 2 ).percent();
    return cr;
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      double overload_chance = p() -> composite_mastery() *
                               p() -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0;

      if ( overload_chance && p() -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p() -> set_bonus.tier13_4pc_caster() )
          p() -> buff.tier13_4pc_caster -> trigger();
      }

      if ( p() -> glyph.telluric_currents -> ok() )
      {
        double mana_gain = p() -> glyph.telluric_currents -> effectN( 2 ).percent();
        if ( p() -> spec == SHAMAN_ELEMENTAL || p() -> spec == SHAMAN_RESTORATION )
          mana_gain *= 1.2;
        p() -> resource_gain( RESOURCE_MANA,
                              state -> result_amount * mana_gain,
                              p() -> gain.telluric_currents );
      }
    }
  }

  virtual bool usable_moving()
  {
    if ( p() -> glyph.unleashed_lightning -> ok() )
      return true;

    return shaman_spell_t::usable_moving();
  }
};

// Elemental Blast Spell ====================================================

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( shaman_t* player, const std::string& options_str, bool dtr = false ) :
    shaman_spell_t( player, player -> find_talent_spell( "Elemental Blast" ), options_str )
  {
    stateless   = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new elemental_blast_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      p() -> buff.elemental_blast -> trigger();
  }
};


// Shamanistic Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Shamanistic Rage" ), options_str )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    harmful   = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct feral_spirit_spell_t : public shaman_spell_t
{
  feral_spirit_spell_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Feral Spirit" ), options_str )
  {
    check_spec( SHAMAN_ENHANCEMENT );

    harmful   = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> pet_feral_spirit -> summon( data().duration() );
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;

  thunderstorm_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Thunderstorm" ), options_str ), bonus( 0 )
  {
    check_spec( SHAMAN_ELEMENTAL );

    stateless             = true;
    cooldown -> duration += player -> glyph.thunder -> effectN( 1 ).time_value();
    bonus                 = data().effectN( 2 ).percent() +
                            player -> glyph.thunderstorm -> effectN( 1 ).percent();
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    player -> resource_gain( data().effectN( 2 ).resource_gain_type(),
                             player -> resources.max[ data().effectN( 2 ).resource_gain_type() ] * bonus,
                             p() -> gain.thunderstorm );
  }
};

// Unleash Elements Spell ===================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;

  unleash_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Unleash Elements" ), options_str )
  {
    may_crit    = false;
    may_miss    = false;
    stateless   = true;

    wind        = new unleash_wind_t( player );
    flame       = new unleash_flame_t( player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    wind  -> execute();
    flame -> execute();

    // You get the buffs, regardless of hit/miss
    if ( p() -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      p() -> buff.unleash_flame -> trigger();
      if ( p() -> talent.unleashed_fury -> ok() )
        p() -> buff.unleashed_fury_ft -> trigger();
    }
    else if ( p() -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      p() -> buff.unleash_wind -> trigger( p() -> buff.unleash_wind -> data().initial_stacks() );
      if ( p() -> talent.unleashed_fury -> ok() )
        p() -> buff.unleashed_fury_wf -> trigger();
    }

    if ( p() -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p() -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
        p() -> buff.unleash_flame -> trigger();
      else if ( p() -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
        p() -> buff.unleash_wind -> trigger( p() -> buff.unleash_wind -> data().initial_stacks() );
    }
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Spiritwalker's Grace" ), options_str )
  {
    may_miss = may_crit = harmful = callbacks = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.spiritwalkers_grace -> trigger();
    if ( p() -> set_bonus.tier13_4pc_heal() )
      p() -> buff.tier13_4pc_healer -> trigger();
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  struct lightning_charge_delay_t : public event_t
  {
    buff_t* buff;
    int consume_stacks;
    int consume_threshold;

    lightning_charge_delay_t( shaman_t* p, buff_t* b, int consume, int consume_threshold ) :
      event_t( p -> sim, p, "lightning_charge_delay_t" ), buff( b ),
      consume_stacks( consume ), consume_threshold( consume_threshold )
    {
      sim -> add_event( this, timespan_t::from_seconds( 0.001 ) );
    }

    void execute()
    {
      if ( ( buff -> check() - consume_threshold ) > 0 )
        buff -> decrement( consume_stacks );
    }
  };

  int consume_threshold;

  earth_shock_t( shaman_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( player, player -> find_class_spell( "Earth Shock" ), options_str ),
    consume_threshold( ( int ) player -> specialization.fulmination -> effectN( 1 ).base_value() )
  {
    stateless            = true;
    cooldown             = player -> cooldown.shock;
    cooldown -> duration = data().cooldown();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new earth_shock_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( consume_threshold == 0 )
      return;

    if ( result_is_hit( execute_state -> result ) )
    {
      int consuming_stacks = p() -> buff.lightning_shield -> stack() - consume_threshold;
      if ( consuming_stacks > 0 && ! is_dtr_action )
      {
        p() -> active_lightning_charge -> execute();
        if ( ! is_dtr_action )
          new ( p() -> sim ) lightning_charge_delay_t( p(), p() -> buff.lightning_shield, consuming_stacks, consume_threshold );
        p() -> proc.fulmination[ consuming_stacks ] -> occur();
      }
    }
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( ! sim -> overrides.weakened_blows )
        state -> target -> debuffs.weakened_blows -> trigger();
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( shaman_t* player, const std::string& options_str, bool dtr = false ) :
    shaman_spell_t( player, player -> find_class_spell( "Flame Shock" ), options_str )
  {
    may_trigger_dtr       = false; // Disable the dot ticks procing DTR
    tick_may_crit         = true;
    stateless             = true;
    dot_behavior          = DOT_REFRESH;
    num_ticks             = ( int ) floor( ( ( double ) num_ticks ) * ( 1.0 + player -> glyph.flame_shock -> effectN( 1 ).percent() ) );
    cooldown              = player -> cooldown.shock;
    cooldown -> duration  = data().cooldown();
    base_dd_multiplier   += player -> glyph.flame_shock -> effectN( 2 ).percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new flame_shock_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  double action_da_multiplier()
  {
    double m = shaman_spell_t::action_da_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m += p() -> buff.unleash_flame -> data().effectN( 2 ).percent();

    return m;
  }

  double action_ta_multiplier()
  {
    double m = shaman_spell_t::action_ta_multiplier();

    if ( p() -> buff.unleash_flame -> up() )
      m += p() -> buff.unleash_flame -> data().effectN( 3 ).percent();

    return m;
  }

  virtual void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    if ( p() -> rng.lava_surge -> roll ( p() -> specialization.lava_surge -> proc_chance() ) )
    {
      p() -> proc.lava_surge -> occur();
      p() -> cooldown.lava_burst -> reset( p() -> cooldown.lava_burst -> remains() > timespan_t::zero() );
    }
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Frost Shock" ), options_str )
  {
    stateless            = true;
    cooldown             = player -> cooldown.shock;
    cooldown -> duration = data().cooldown();
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Wind Shear" ), options_str )
  {
    may_miss = may_crit = false;
    stateless = true;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Totem Spells
// ==========================================================================

struct shaman_totem_t : public shaman_spell_t
{
  timespan_t totem_duration;
  double totem_bonus;
  totem_e totem;

  shaman_totem_t( const std::string& token, const std::string& totem_name, shaman_t* player, const std::string& options_str, totem_e t ) :
    shaman_spell_t( token, player, player -> find_class_spell( totem_name ), options_str ), totem_duration( timespan_t::zero() ), totem_bonus( 0 ), totem( t )
  {
    is_totem             = true;
    harmful              = false;
    hasted_ticks         = false;
    callbacks            = false;
    totem_duration       = data().duration();
    // Model all totems as ticking "dots" for now, this will cause them to properly
    // "fade", so we can recast them if the fight length is long enough and optimal_raid=0
    num_ticks            = 1;
    base_tick_time       = totem_duration;
  }

  // Simulate a totem "drop", this is a simplified action_t::execute()
  virtual void execute()
  {
    if ( p() -> totems[ totem ] )
    {
      p() -> totems[ totem ] -> cancel();
      p() -> totems[ totem ] -> get_dot() -> cancel();
    }

    if ( sim -> log )
      sim -> output( "%s performs %s", p() -> name(), name() );

    result = RESULT_HIT; tick_dmg = 0; direct_dmg = 0;

    consume_resource();
    update_ready();
    schedule_travel( target );

    p() -> totems[ totem ] = this;

    stats -> add_execute( time_to_execute );
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );

    if ( sim -> log )
      sim -> output( "%s destroys %s", p() -> name(), p() -> totems[ totem ] -> name() );
    p() -> totems[ totem ] = 0;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug )
      sim -> output( "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

    player_buff(); // Totems recalculate stats on every "tick"

    if ( aoe == -1 || aoe > 0 )
    {
      std::vector< player_t* > tl = target_list();

      for ( size_t t = 0; t < tl.size(); t++ )
      {
        target_debuff( tl[ t ], DMG_OVER_TIME );

        result = calculate_result( total_crit(), tl[ t ] -> level );

        if ( result_is_hit() )
        {
          direct_dmg = calculate_direct_damage( result, t + 1, tl[ t ] -> level, total_attack_power(), total_spell_power(), total_dd_multiplier() );

          if ( direct_dmg > 0 )
          {
            tick_dmg = direct_dmg;
            direct_dmg = 0;
            assess_damage( tl[ t ], tick_dmg, DMG_OVER_TIME, result );
          }
        }
        else
        {
          if ( sim -> log )
            sim -> output( "%s avoids %s (%s)", target -> name(), name(), util::result_type_string( result ) );
        }

      }

      stats -> add_tick( d -> time_to_tick );
    }
    else
    {
      target_debuff( target, DMG_DIRECT );
      result = calculate_result( total_crit(), target -> level );

      if ( result_is_hit() )
      {
        direct_dmg = calculate_direct_damage( result, 0, target -> level, total_attack_power(), total_spell_power(), total_dd_multiplier() );

        if ( direct_dmg > 0 )
        {
          tick_dmg = direct_dmg;
          direct_dmg = 0;
          assess_damage( target, tick_dmg, DMG_OVER_TIME, result );
        }
      }
      else
      {
        if ( sim -> log )
          sim -> output( "%s avoids %s (%s)", target -> name(), name(), util::result_type_string( result ) );
      }

      stats -> add_tick( d -> time_to_tick );
    }
  }

  virtual timespan_t gcd()
  {
    if ( harmful )
      return shaman_spell_t::gcd();

    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero();
  }

  virtual bool ready()
  {
    if ( p() -> totems[ totem ] )
      return false;

    return spell_t::ready();
  }

  virtual void reset()
  {
    shaman_spell_t::reset();

    if ( p() -> totems[ totem ] == this )
      p() -> totems[ totem ] = 0;
  }
};

// Earth Elemental Totem Spell ==============================================

struct earth_elemental_totem_t : public shaman_totem_t
{
  earth_elemental_totem_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "earth_elemental_totem", "Earth Elemental Totem", player, options_str, TOTEM_EARTH )
  {
    // Skip a pointless cancel call (and debug=1 cancel line)
    dot_behavior = DOT_REFRESH;
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    p() -> pet_earth_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );

    p() -> pet_earth_elemental -> dismiss();
  }

  // Earth Elemental Totem will always override any earth totem you have
  virtual bool ready()
  {
    return shaman_spell_t::ready();
  }
};

// Fire Elemental Totem Spell ===============================================

struct fire_elemental_totem_t : public shaman_totem_t
{
  fire_elemental_totem_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "fire_elemental_totem", "Fire Elemental Totem", player, options_str, TOTEM_FIRE )
  {
    cooldown -> duration *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 1 ).percent();
    totem_duration       *= 1.0 + p() -> glyph.fire_elemental_totem -> effectN( 2 ).percent();
    base_tick_time        = totem_duration;
    // Skip a pointless cancel call (and debug=1 cancel line)
    dot_behavior = DOT_REFRESH;
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    if ( p() -> talent.primal_elementalist -> ok() )
      p() -> pet_fire_elemental -> summon();
    else
      p() -> guardian_fire_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );
    if ( p() -> talent.primal_elementalist -> ok() )
      p() -> pet_fire_elemental -> dismiss();
    else
      p () -> guardian_fire_elemental -> dismiss();
  }

  // Allow Fire Elemental Totem to override any active fire totems
  virtual bool ready()
  {
    return shaman_spell_t::ready();
  }
};

// Magma Totem Spell ========================================================

struct magma_totem_t : public shaman_totem_t
{
  magma_totem_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "magma_totem", "Magma Totem", player, options_str, TOTEM_FIRE )
  {
    const spell_data_t* trigger;

    aoe               = -1;
    harmful           = true;
    may_crit          = true;
    // Magma Totem is not a real DoT, but rather a pet that is spawned.

    // Spell id 8188 does the triggering of magma totem's aura
    base_tick_time    = p() -> dbc.spell( 8188 ) -> effectN( 1 ).period();
    num_ticks         = ( int ) ( totem_duration / base_tick_time );

    // Fill out scaling data
    trigger           = p() -> dbc.spell( p() -> dbc.spell( 8188 ) -> effectN( 1 ).trigger_spell_id() );
    // Also kludge totem school to fire for accurate damage
    school            = trigger -> get_school_type();
    stats -> school   = school;

    base_dd_min       = p() -> dbc.effect_min( trigger -> effectN( 1 ).id(), p() -> level );
    base_dd_max       = p() -> dbc.effect_max( trigger -> effectN( 1 ).id(), p() -> level );
    direct_power_mod  = trigger -> effectN( 1 ).coeff();
  }

  virtual void execute()
  {
    player_buff();
    shaman_totem_t::execute();
  }
};

// Mana Tide Totem Spell ====================================================

struct mana_tide_totem_t : public shaman_totem_t
{
  mana_tide_totem_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_tide_totem", "Mana Tide Totem", player, options_str, TOTEM_WATER )
  {
    check_spec( SHAMAN_RESTORATION );
    // Mana tide effect bonus is in a separate spell, we dont need other info
    // from there anymore, as mana tide does not pulse anymore
    totem_bonus  = p() -> dbc.spell( 16191 ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      // Change buff duration based on totem duration
      p -> buffs.mana_tide -> buff_duration = totem_duration;
      p -> buffs.mana_tide -> trigger( 1, totem_bonus );
    }

    update_ready();
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return ( player -> resources.pct( RESOURCE_MANA ) < 0.75 );
  }
};

// Searing Totem Spell ======================================================

struct searing_totem_t : public shaman_totem_t
{
  searing_totem_t( shaman_t* player, const std::string& options_str ) :
    shaman_totem_t( "searing_totem", "Searing Totem", player, options_str, TOTEM_FIRE )
  {
    harmful   = true;
    may_crit  = true;

    // TODO: Make sure Searing Totem scaling information is now correct
    base_dd_min      = p() -> dbc.effect_min( p() -> dbc.spell( 3606 ) -> effectN( 1 ).id(), p() -> level );
    base_dd_max      = p() -> dbc.effect_max( p() -> dbc.spell( 3606 ) -> effectN( 1 ).id(), p() -> level );
    direct_power_mod = p() -> dbc.spell( 3606 ) -> effectN( 1 ).coeff();
    // TODO: Is the shoot time of Searing Totme now 1.5 seconds, like spell data says?
    base_tick_time   = timespan_t::from_seconds( 1.6 );
    travel_speed     = 0; // TODO: Searing bolt has a real travel time, however modeling it is another issue entirely
    range            = p() -> dbc.spell( 3606 ) -> max_range();
    num_ticks        = ( int ) ( totem_duration / base_tick_time );
    // Also kludge totem school to fire
    school           = p() -> dbc.spell( 3606 ) -> get_school_type();
    stats -> school  = school;
    p() -> active_searing_flames_dot = new searing_flames_t( p() );
  }

  virtual void tick( dot_t* d )
  {
    shaman_td_t* td = targetdata( target );
    shaman_totem_t::tick( d );
    if ( result_is_hit() && p() -> specialization.searing_flames -> ok() &&
         td -> debuffs_searing_flames -> trigger() )
    {
      double new_base_td = tick_dmg;
      // Searing flame dot treats all hits as.. HITS
      if ( result == RESULT_CRIT )
        new_base_td /= 1.0 + total_crit_bonus();

      p() -> active_searing_flames_dot -> base_td = new_base_td / p() -> active_searing_flames_dot -> num_ticks;
      p() -> active_searing_flames_dot -> execute();
    }
  }
};

// ==========================================================================
// Shaman Weapon Imbues
// ==========================================================================

// Flametongue Weapon Spell =================================================

struct flametongue_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  flametongue_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Flametongue Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    std::string weapon_str;

    option_t options[] =
    {
      { "weapon", OPT_STRING,  &weapon_str },
      { 0,        OPT_UNKNOWN, 0           }
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: flametongue_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    // Spell damage scaling is defined in "Flametongue Weapon (Passive), id 10400"
    bonus_power  = player -> dbc.spell( 10400 ) -> effectN( 2 ).percent();
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> flametongue_mh = new flametongue_weapon_spell_t( "flametongue_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> flametongue_oh = new flametongue_weapon_spell_t( "flametongue_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    return false;
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Windfury Weapon" ) ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    check_spec( SHAMAN_ENHANCEMENT );
    std::string weapon_str;

    option_t options[] =
    {
      { "weapon", OPT_STRING,  &weapon_str },
      { 0,        OPT_UNKNOWN, 0           }
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: windfury_weapon: weapon option must be one of main/off/both\n", p() -> name() );
      sim -> cancel();
    }

    bonus_power  = data().effectN( 1 ).average( player );
    harmful      = false;
    may_miss     = false;

    if ( main )
      player -> windfury_mh = new windfury_weapon_melee_attack_t( "windfury_mh", player, &( player -> main_hand_weapon ) );

    if ( off )
      player -> windfury_oh = new windfury_weapon_melee_attack_t( "windfury_oh", player, &( player -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      p() -> main_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      p() -> off_hand_weapon.buff_type  = WINDFURY_IMBUE;
      p() -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( p() -> main_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    if ( off && ( p() -> off_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    return false;
  }
};

// ==========================================================================
// Shaman Shields
// ==========================================================================

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Lightning Shield" ), options_str )
  {
    harmful = false;

    player -> active_lightning_charge = new lightning_charge_t( player -> primary_tree() == SHAMAN_ELEMENTAL ? "fulmination" : "lightning_shield", player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.water_shield     -> expire();
    p() -> buff.lightning_shield -> trigger( data().initial_stacks() );
  }

  virtual bool ready()
  {
    if ( p() -> buff.lightning_shield -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Water Shield Spell =======================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;

  water_shield_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( player, player -> find_class_spell( "Water Shield" ), options_str ),
    bonus( 0.0 )
  {
    harmful      = false;
    bonus        = data().effectN( 2 ).average( player );
    bonus       *= 1.0 + player -> glyph.water_shield -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    p() -> buff.lightning_shield -> expire();
    p() -> buff.water_shield -> trigger( data().initial_stacks(), bonus );
  }

  virtual bool ready()
  {
    if ( p() -> buff.water_shield -> check() )
      return false;
    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct unleash_flame_buff_t : public buff_t
{
  struct unleash_flame_expiration_delay_t : public event_t
  {
    unleash_flame_buff_t* buff;

    unleash_flame_expiration_delay_t( shaman_t* player, unleash_flame_buff_t* b ) :
      event_t( player -> sim, player, "unleash_flame_expiration_delay" ), buff( b )
    {
      sim -> add_event( this, sim -> gauss( player -> uf_expiration_delay,
                                            player -> uf_expiration_delay_stddev ) );
    }

    virtual void execute()
    {
      // Call real expire after a delay
      buff -> buff_t::expire();
      buff -> expiration_delay = 0;
    }
  };

  unleash_flame_expiration_delay_t* expiration_delay;

  unleash_flame_buff_t( shaman_t* p ) :
    buff_t( buff_creator_t( p, 73683, "unleash_flame" ) ),
    expiration_delay( 0 )
  {}

  void reset()
  {
    buff_t::reset();
    event_t::cancel( expiration_delay );
  }

  void expire()
  {
    // Active player's Unleash Flame buff has a short aura expiration delay, which allows
    // "Double Casting" with a single buff
    if ( ! player -> current.sleeping )
    {
      if ( current_stack <= 0 ) return;
      if ( expiration_delay ) return;
      expiration_delay = new ( sim ) unleash_flame_expiration_delay_t( static_cast<shaman_t*>( player ), this );
    }
    // If the p() is sleeping, make sure the existing buff behavior works (i.e., a call to
    // expire) and additionally, make _absolutely sure_ that any pending expiration delay
    // is canceled
    else
    {
      buff_t::expire();
      event_t::cancel( expiration_delay );
    }
  }
};

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  option_t shaman_options[] =
  {
    { "wf_delay",                   OPT_TIMESPAN, &( wf_delay                   ) },
    { "wf_delay_stddev",            OPT_TIMESPAN, &( wf_delay_stddev            ) },
    { "uf_expiration_delay",        OPT_TIMESPAN, &( uf_expiration_delay        ) },
    { "uf_expiration_delay_stddev", OPT_TIMESPAN, &( uf_expiration_delay_stddev ) },
    { "eoe_proc_chance",            OPT_FLT,      &( eoe_proc_chance            ) },
    { NULL,                         OPT_UNKNOWN,  NULL                            }
  };

  option_t::copy( options, shaman_options );
}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "call_of_the_elements"    ) return new     call_of_the_elements_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "earth_elemental_totem"   ) return new    earth_elemental_totem_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "fire_elemental_totem"    ) return new     fire_elemental_totem_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "magma_totem"             ) return new              magma_totem_t( this, options_str );
  if ( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options_str );
  if ( name == "primal_strike"           ) return new            primal_strike_t( this, options_str );
  if ( name == "searing_totem"           ) return new            searing_totem_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "feral_spirit"             ) return new        feral_spirit_spell_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "unleash_elements"        ) return new         unleash_elements_t( this, options_str );
  if ( name == "water_shield"            ) return new             water_shield_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "feral_spirit"         ) return new feral_spirit_pet_t    ( sim, this );
  if ( pet_name == "fire_elemental_pet"  ) return new fire_elemental_t ( sim, this, false );
  if ( pet_name == "fire_elemental_guardian"  ) return new fire_elemental_t ( sim, this, true );
  if ( pet_name == "earth_elemental"     ) return new earth_elemental_pet_t( sim, this );

  return 0;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  pet_feral_spirit     = create_pet( "feral_spirit"     );
  pet_fire_elemental  = create_pet( "fire_elemental_pet"  );
  guardian_fire_elemental = create_pet( "fire_elemental_guardian" );
  pet_earth_elemental = create_pet( "earth_elemental" );
}

// shaman_t::init ===========================================================

void shaman_t::init()
{
  player_t::init();

  if ( eoe_proc_chance == 0 )
  {
    if ( primary_tree() == SHAMAN_ENHANCEMENT )
      eoe_proc_chance = 0.30; /// Tested, ~30% (1k LB casts)
    else
      eoe_proc_chance = 0.06; // Tested, ~6% (1k LB casts)
  }
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105780, 105816, 105866, 105872,     0,     0, 105764, 105876 }, // Tier13
    {      0,      0,      0,      0,     0,     0,      0,      0 },
  };
  sets                               = new set_bonus_array_t( this, set_bonuses );

  // Generic
  specialization.mail_specialization = find_specialization_spell( "Mail Specialization" );

  // Elemental
  specialization.elemental_focus     = find_specialization_spell( "Elemental Focus" );
  specialization.elemental_fury      = find_specialization_spell( "Elemental Fury" );
  specialization.elemental_precision = find_specialization_spell( "Elemental Precision" );
  specialization.fulmination         = find_specialization_spell( "Fulmination" );
  specialization.lava_surge          = find_specialization_spell( "Lava Surge" );
  specialization.rolling_thunder     = find_specialization_spell( "Rolling Thunder" );
  specialization.shamanism           = find_specialization_spell( "Shamanism" );
  specialization.spiritual_insight   = find_specialization_spell( "Spiritual Insight" );

  // Enhancement
  specialization.dual_wield          = find_specialization_spell( "Dual Wield" );
  specialization.flurry              = find_specialization_spell( "Flurry" );
  specialization.maelstrom_weapon    = find_specialization_spell( "Maelstrom Weapon" );
  specialization.mental_quickness    = find_specialization_spell( "Mental Quickness" );
  specialization.primal_wisdom       = find_specialization_spell( "Primal Wisdom" );
  specialization.searing_flames      = find_specialization_spell( "Searing Flames" );
  specialization.shamanistic_rage    = find_specialization_spell( "Shamanistic Rage" );
  specialization.static_shock        = find_specialization_spell( "Static Shock" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( SHAMAN_ELEMENTAL   );
  mastery.enhanced_elements          = find_mastery_spell( SHAMAN_ENHANCEMENT );

  // Talents
  talent.call_of_the_elements        = find_talent_spell( "Call of the Elements" );
  talent.totemic_restoration         = find_talent_spell( "Totemic Restoration"  );

  talent.elemental_mastery           = find_talent_spell( "Elemental Mastery"    );
  talent.ancestral_swiftness         = find_talent_spell( "Ancestral Swiftness"  );
  talent.echo_of_the_elements        = find_talent_spell( "Echo of the Elements" );

  talent.unleashed_fury              = find_talent_spell( "Unleashed Fury"       );
  talent.primal_elementalist         = find_talent_spell( "Primal Elementalist"  );
  talent.elemental_blast             = find_talent_spell( "Elemental Blast"      );

  // Glyphs
  glyph.chain_lightning              = find_glyph_spell( "Glyph of Chain Lightning" );
  glyph.fire_elemental_totem         = find_glyph_spell( "Glyph of Fire Elemental Totem" );
  glyph.flame_shock                  = find_glyph_spell( "Glyph of Flame Shock" );
  glyph.lava_lash                    = find_glyph_spell( "Glyph of Lava Lash" );
  glyph.spiritwalkers_grace          = find_glyph_spell( "Glyph of Spiritwalker's Grace" );
  glyph.telluric_currents            = find_glyph_spell( "Glyph of Telluric Currents" );
  glyph.thunder                      = find_glyph_spell( "Glyph of Thunder" );
  glyph.thunderstorm                 = find_glyph_spell( "Glyph of Thunderstorm" );
  glyph.unleashed_lightning          = find_glyph_spell( "Glyph of Unleashed Lightning" );
  glyph.water_shield                 = find_glyph_spell( "Glyph of Water Shield" );

  // Misc spells
  spell.primal_wisdom                = find_spell( 63375 );
  spell.searing_flames               = find_spell( 77657 );

  player_t::init_spells();
}

// shaman_t::init_base ======================================================

void shaman_t::init_base()
{
  player_t::init_base();

  base.attack_power = ( level * 2 ) - 30;
  initial.attack_power_per_strength = 1.0;
  initial.attack_power_per_agility  = 2.0;
  initial.spell_power_per_intellect = 1.0;

  if ( primary_tree() == SHAMAN_ELEMENTAL )
    resources.initial_multiplier[ RESOURCE_MANA ] = specialization.spiritual_insight -> effectN( 1 ).percent();

  current.distance = ( primary_tree() == SHAMAN_ENHANCEMENT ) ? 3 : 30;
  initial.distance = current.distance;

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;

  if ( false && primary_tree() == SHAMAN_ENHANCEMENT )
    ready_type = READY_TRIGGER;
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  if ( primary_tree() == SHAMAN_ENHANCEMENT )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = true;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors != 0;
    scales_with[ STAT_HIT_RATING2           ] = true;
    scales_with[ STAT_SPIRIT                ] = false;
    scales_with[ STAT_SPELL_POWER           ] = false;
    scales_with[ STAT_INTELLECT             ] = false;
  }

  // Elemental Precision treats Spirit like Spell Hit Rating, no need to calculte for Enha though
  if ( specialization.elemental_precision -> ok() && sim -> scaling -> scale_stat == STAT_SPIRIT )
  {
    double v = sim -> scaling -> scale_value;
    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      initial.attribute[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// shaman_t::init_buffs =====================================================

void shaman_t::init_buffs()
{
  player_t::init_buffs();

  buff.elemental_blast     = buff_creator_t( this, "elemental_blast", dbc.spell( 118522 ) );
  buff.elemental_focus     = buff_creator_t( this, "elemental_focus",   specialization.elemental_focus -> effectN( 1 ).trigger() )
                             .activated( false );
  buff.elemental_mastery   = buff_creator_t( this, "elemental_mastery", talent.elemental_mastery )
                             .chance( 1.0 );
  buff.flurry              = buff_creator_t( this, "flurry",            specialization.flurry -> effectN( 1 ).trigger() )
                             .chance( specialization.flurry -> proc_chance() )
                             .activated( false );
  buff.lava_surge          = buff_creator_t( this, "lava_surge",        specialization.lava_surge )
                             .activated( false );
  buff.lightning_shield    = buff_creator_t( this, "lightning_shield", find_class_spell( "Lightning Shield" ) )
                             .max_stack( ( primary_tree() == SHAMAN_ELEMENTAL )
                                         ? static_cast< int >( specialization.rolling_thunder -> effectN( 1 ).base_value() )
                                         : find_class_spell( "Lightning Shield" ) -> initial_stacks() );
  buff.maelstrom_weapon    = buff_creator_t( this, "maelstrom_weapon",  specialization.maelstrom_weapon -> effectN( 1 ).trigger() )
                             .chance( specialization.maelstrom_weapon -> proc_chance() )
                             .activated( false );
  buff.shamanistic_rage    = buff_creator_t( this, "shamanistic_rage",  specialization.shamanistic_rage );
  buff.spiritwalkers_grace = buff_creator_t( this, "spiritwalkers_grace", find_class_spell( "Spiritwalker's Grace" ) )
                             .chance( 1.0 )
                             .duration( find_class_spell( "Spiritwalker's Grace" ) -> duration() +
                                        glyph.spiritwalkers_grace -> effectN( 1 ).time_value() +
                                        sets -> set( SET_T13_4PC_HEAL ) -> effectN( 1 ).time_value() );
  buff.unleash_flame       = new unleash_flame_buff_t( this );
  buff.unleash_wind        = buff_creator_t( this, "unleash_wind", dbc.spell( 73681 ) );
  buff.unleashed_fury_wf   = buff_creator_t( this, "unleashed_fury_wf", dbc.spell( 118472 ) );
  buff.water_shield        = buff_creator_t( this, "water_shield", find_class_spell( "Water Shield" ) );

  // Tier13 set bonuses
  buff.tier13_2pc_caster   = stat_buff_creator_t( buff_creator_t( this, "tier13_2pc_caster", dbc.spell( 105779 ) ) )
                             .stat( STAT_MASTERY_RATING )
                             .amount( dbc.spell( 105779 ) -> effectN( 1 ).base_value() );
  buff.tier13_4pc_caster   = stat_buff_creator_t( buff_creator_t( this, "tier13_4pc_caster", dbc.spell( 105821 ) ) )
                             .stat( STAT_HASTE_RATING )
                             .amount( dbc.spell( 105821 ) -> effectN( 1 ).base_value() );
  buff.tier13_4pc_healer   = buff_creator_t( this, "tier13_4pc_healer", dbc.spell( 105877 ) );
}

// shaman_t::init_values ====================================================

void shaman_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    initial.attribute[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_heal() )
    initial.attribute[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_heal() )
    initial.attribute[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_melee() )
    initial.attribute[ ATTR_AGILITY ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    initial.attribute[ ATTR_AGILITY ]   += 90;
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gain.primal_wisdom        = get_gain( "primal_wisdom"     );
  gain.rolling_thunder      = get_gain( "rolling_thunder"   );
  gain.telluric_currents    = get_gain( "telluric_currents" );
  gain.thunderstorm         = get_gain( "thunderstorm"      );
  gain.water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.elemental_overload = get_proc( "elemental_overload"      );
  proc.lava_surge         = get_proc( "lava_surge"              );
  proc.maelstrom_weapon   = get_proc( "maelstrom_weapon"        );
  proc.static_shock       = get_proc( "static_shock"            );
  proc.swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  proc.swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  proc.rolling_thunder    = get_proc( "rolling_thunder"         );
  proc.wasted_ls          = get_proc( "wasted_lightning_shield" );
  proc.wasted_mw          = get_proc( "wasted_maelstrom_weapon" );
  proc.windfury           = get_proc( "windfury"                );

  for ( int i = 0; i < 7; i++ )
  {
    proc.fulmination[ i ] = get_proc( "fulmination_" + util::to_string( i ) );
  }

  for ( int i = 0; i < 6; i++ )
  {
    proc.maelstrom_weapon_used[ i ] = get_proc( "maelstrom_weapon_stack_" + util::to_string( i ) );
  }
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();
  rng.echo_of_the_elements = get_rng( "echo_of_the_elements" );
  rng.elemental_overload   = get_rng( "elemental_overload"   );
  rng.lava_surge           = get_rng( "lava_surge"           );
  rng.primal_wisdom        = get_rng( "primal_wisdom"        );
  rng.rolling_thunder      = get_rng( "rolling_thunder"      );
  rng.searing_flames       = get_rng( "searing_flames"       );
  rng.static_shock         = get_rng( "static_shock"         );
  rng.windfury_delay       = get_rng( "windfury_delay"       );
  rng.windfury_weapon      = get_rng( "windfury_weapon"      );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_actions()
{
  if ( ! ( primary_role() == ROLE_ATTACK && primary_tree() == SHAMAN_ENHANCEMENT ) &&
       ! ( primary_role() == ROLE_SPELL  && primary_tree() == SHAMAN_ELEMENTAL   ) )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s's role or spec isn't supported yet.", name() );
    quiet = true;
    return;
  }

  if ( primary_tree() == SHAMAN_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( primary_tree() == SHAMAN_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman healing for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }
  
  if ( ! action_list_str.empty() )
  {
    player_t::init_actions();
    return;
  }

  clear_action_priority_lists();

  
  std::string use_items_str;
  int num_items = ( int ) items.size();
  for ( int i=0; i < num_items; i++ )
  {
    if ( items[ i ].use.active() )
    {
      use_items_str += "/use_item,name=";
      use_items_str += items[ i ].name();
    }
  }

  std::ostringstream s;

  // Flask
  if ( level >= 80 ) s << "flask,precombat=1,type=";
  if ( primary_role() == ROLE_ATTACK )
    s << ( ( level > 85 ) ? "spring_blossoms" : ( level >= 80 ) ? "winds" : "" );
  else
    s << ( ( level > 85 ) ? "warm_sun" : ( level >= 80 ) ? "draconic_mind" : "" );

  // Food
  if ( level >= 80 ) s << "/food,precombat=1,type=";
  s << ( ( level > 85 ) ? "great_pandaren_banquet" : ( level >= 80 ) ? "seafood_magnifique_feast" : "" );

  // Weapon Enchants
  if ( spec == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    if ( level >= 30 ) s << "/windfury_weapon,precombat=1,weapon=main";
    if ( off_hand_weapon.type != WEAPON_NONE )
      if ( level >= 10 ) s << "/flametongue_weapon,precombat=1,weapon=off";
  }
  else
  {
    if ( level >= 10 ) s << "/flametongue_weapon,precombat=1,weapon=main";
    if ( spec == SHAMAN_ENHANCEMENT && off_hand_weapon.type != WEAPON_NONE )
      if ( level >= 10 ) s << "/flametongue_weapon,precombat=1,weapon=off";
  }

  // Active Shield, presume any non-restoration / healer wants lightning shield
  if ( spec != SHAMAN_RESTORATION || primary_role() != ROLE_HEAL )
  {
    if ( level >= 8  ) s << "/lightning_shield,precombat=1";
  }
  else
  {
    if ( level >= 20 ) s << "/water_shield,precombat=1";
  }

  // Snapshot stats
  s << "/snapshot_stats,precombat=1";

  // Prepotion (work around for now, until snapshot_stats stop putting things into combat)
  if ( primary_role() == ROLE_ATTACK )
    s << ( ( level > 85 ) ? "/virmens_bite_potion" : ( level >= 80 ) ? "/tolvir_potion" : "" );
  else
    s << ( ( level > 85 ) ? "/jinyu_potion" : ( level >= 80 ) ? "/volcanic_potion" : "" );
  if ( level >= 80 ) s << ",precombat=1";
  
  // All Shamans Bloodlust and Wind Shear by default
  if ( level >= 16 ) s << "/wind_shear";
  if ( level >= 70 ) s << "/bloodlust,if=target.health.pct<=25|target.time_to_die<=60";

  // Potion use
  if ( primary_role() == ROLE_ATTACK )
    s << ( ( level > 85 ) ? "/virmens_bite_potion" : ( level >= 80 ) ? "/tolvir_potion" : "" );
  else
    s << ( ( level > 85 ) ? "/jinyu_potion" : ( level >= 80 ) ? "/volcanic_potion" : "" );
  if ( level >= 80 ) s << ",if=buff.bloodlust.react|target.time_to_die<=40";

  // Melee turns on auto attack
  if ( primary_role() == ROLE_ATTACK )
    s << "/auto_attack";

  // On use stuff and racial / profession abilities
  s << use_items_str;
  s << init_use_profession_actions();
  s << init_use_racial_actions();

  if ( talent.call_of_the_elements -> ok() )
    s << "/call_of_the_elements,if=cooldown.fire_elemental_totem.remains>0";

  if ( spec == SHAMAN_ENHANCEMENT && primary_role() == ROLE_ATTACK )
  {
    if ( level >= 66 )     s << "/fire_elemental_totem,if=!ticking";
    if ( level >= 16 )     s << "/searing_totem";
    if ( level >= 26 )     s << "/stormstrike";
    else if ( level >= 3 ) s << "/primal_strike";
    if ( level >= 10 )     s << "/lava_lash";
    s << "/lightning_bolt,if=buff.maelstrom_weapon.react=5|(set_bonus.tier13_4pc_melee=1&buff.maelstrom_weapon.react>=4&pet.feral_spirit.active)";
    if ( level >= 81 ) s << "/unleash_elements";
    if ( level >= 12 ) s << "/flame_shock,if=!ticking|buff.unleash_flame.up";
    if ( level >= 6  ) s << "/earth_shock";
    if ( level >= 60 ) s << "/feral_spirit";
    if ( level >= 58 ) s << "/earth_elemental_totem";
    if ( level >= 20 ) s << "/fire_nova,if=target.adds>1";
    if ( level >= 85 ) s << "/spiritwalkers_grace,moving=1";
    s << "/lightning_bolt,if=buff.maelstrom_weapon.react>1";
  }
  else if ( spec == SHAMAN_ELEMENTAL && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_DPS ) )
  {
    if ( level >= 66 ) s << "/fire_elemental_totem,if=!ticking";
    if ( talent.elemental_mastery -> ok() )
      s << "/elemental_mastery";
    if ( set_bonus.tier13_4pc_heal() && level >= 85 )
      s << "/spiritwalkers_grace,if=!buff.bloodlust.react|target.time_to_die<=25";
    if ( ! glyph.unleashed_lightning -> ok() && level >= 81 )
      s << "/unleash_elements,moving=1";
    if ( level >= 12 ) s << "/flame_shock,if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)";
    if ( level >= 34 ) s << "/lava_burst,if=dot.flame_shock.remains>cast_time&cooldown_react";
    if ( specialization.fulmination -> ok() && level >= 6 )
    {
      s << "/earth_shock,if=buff.lightning_shield.react=buff.lightning_shield.max_stack";
      s << "/earth_shock,if=buff.lightning_shield.react>buff.lightning_shield.max_stack-3&dot.flame_shock.remains>cooldown&dot.flame_shock.remains<cooldown+action.flame_shock.tick_time";
    }
    if ( level >= 58 ) s << "/earth_elemental_totem,if=!ticking";
    if ( level >= 16 ) s << "/searing_totem";
    if ( level >= 85 )
    {
      s << "/spiritwalkers_grace,moving=1";
      if ( glyph.unleashed_lightning -> ok() )
        s << ",if=cooldown.lava_burst.remains=0";
    }
    if ( level >= 28 ) s << "/chain_lightning,if=target.adds>2";
    s << "/lightning_bolt";
    if ( level >= 10 ) s << "/thunderstorm";
  }
  else if ( primary_role() == ROLE_SPELL )
  {
    if ( level >= 66 ) s << "/fire_elemental_totem,if=!ticking";
    if ( level >= 16 ) s << "/searing_totem";
    if ( level >= 58 ) s << "/earth_elemental_totem";
    if ( level >= 85 ) s << "/spiritwalkers_grace,moving=1";
    if ( level >= 12 ) s << "/flame_shock,if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)";
    if ( level >= 28 ) s << "/chain_lightning,if=target.adds>2&mana.pct>25";
    s << "/lightning_bolt";
  }
  else if ( primary_role() == ROLE_ATTACK )
  {
    if ( level >= 66 ) s << "/fire_elemental_totem,if=!ticking";
    if ( level >= 16 ) s << "/searing_totem";
    if ( level >= 3  ) s << "/primal_strike";
    if ( level >= 81 ) s << "/unleash_elements";
    if ( level >= 12 ) s << "/flame_shock,if=!ticking|buff.unleash_flame.up";
    if ( level >= 6  ) s << "/earth_shock";
    if ( level >= 58 ) s << "/earth_elemental_totem";
    if ( level >= 85 ) s << "/spiritwalkers_grace,moving=1";
    s << "/lightning_bolt,moving=1";
  }
  action_list_str = s.str();
  action_list_default = 1;

  player_t::init_actions();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( level == 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg &&
         executing &&
         sim -> roll( current.skill ) &&
         swg -> cooldown -> remains() == timespan_t::zero() &&
         resource_available( swg -> current_resource(), swg -> cost() ) )
    {
      // Elemental has to do some additional checking here
      if ( primary_tree() == SHAMAN_ELEMENTAL )
      {
        // Elemental executes SWG mid-cast during a movement event, if
        // 1) The profile does not have Glyph of Unleashed Lightning and is executing
        //    a Lighting Bolt
        // 2) The profile is executing a Lava Burst
        // 3) The profile is casting Chain Lightning
        if ( ( ! glyph.unleashed_lightning -> ok() && executing -> id == 403 ) ||
             ( executing -> id == 51505 ) ||
             ( executing -> id == 421 ) )
        {
          if ( sim -> log )
            sim -> output( "spiritwalkers_grace during spell cast, next cast (%s) should finish",
                           executing -> name_str.c_str() );
          swg -> execute();
        }
      }
      // Other trees blindly execute Spiritwalker's Grace if there's a spell cast
      // executing during movement event
      else
      {
        swg -> execute();
        if ( sim -> log )
          sim -> output( "spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         executing -> name_str.c_str() );
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( attribute_e attr )
{
  if ( attr == ATTR_AGILITY || attr == ATTR_INTELLECT )
    return specialization.mail_specialization -> effectN( 1 ).percent();

  return 0.0;
}

// shaman_t::composite_spell_hit ============================================

double shaman_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  hit += ( specialization.elemental_precision -> ok() *
    ( spirit() - base.attribute[ ATTR_SPIRIT ] ) ) / rating.spell_hit;

  return hit;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_attack_speed()
{
  double speed = player_t::composite_attack_speed();

  if ( buff.flurry -> up() )
    speed *= 1.0 / ( 1.0 + buff.flurry -> data().effectN( 1 ).percent() );

  if ( buff.unleash_wind -> up() )
    speed *= 1.0 / ( 1.0 + buff.unleash_wind -> data().effectN( 2 ).percent() );

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( school_e school )
{
  double sp = 0;

  if ( primary_tree() == SHAMAN_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * composite_attack_power() * specialization.mental_quickness -> effectN( 1 ).percent();
  else
    sp = player_t::composite_spell_power( school );

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier()
{
  if ( primary_tree() == SHAMAN_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( school != SCHOOL_PHYSICAL && school != SCHOOL_BLEED )
  {
    if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + main_hand_weapon.buff_value;
    else if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + off_hand_weapon.buff_value;
  }

  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    m *= 1.0 + composite_mastery() * mastery.enhanced_elements -> effectN( 2 ).base_value() / 10000.0;

  return m;
}

// shaman_t::regen  =========================================================

void shaman_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.water_shield -> up() )
  {
    double water_shield_regen = periodicity.total_seconds() * buff.water_shield -> value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gain.water_shield );
  }
}

// shaman_t::combat_begin ===================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  // TODO: Trigger Shaman auras based on specialization
}

// shaman_t::arise() ========================================================

void shaman_t::arise()
{
  player_t::arise();

  if ( ! sim -> overrides.mastery && dbc.spell( 116956 ) -> is_level( level ) )
    sim -> auras.mastery -> trigger();

  if ( ! sim -> overrides.spell_power_multiplier && dbc.spell( 77747 ) -> is_level( level ) )
    sim -> auras.spell_power_multiplier -> trigger();

  // MoP TODO: Add level checks when the auras appear in spell data
  if ( primary_tree() == SHAMAN_ENHANCEMENT && ! sim -> overrides.attack_haste ) sim -> auras.attack_haste           -> trigger();
  if ( primary_tree() == SHAMAN_ELEMENTAL   && ! sim -> overrides.spell_haste  ) sim -> auras.spell_haste            -> trigger();
}

// shaman_t::decode_set =====================================================

int shaman_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "spiritwalkers" ) )
  {
    bool is_caster = ( strstr( s, "headpiece"     ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "hauberk"       ) ||
                       strstr( s, "kilt"          ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "helmet"         ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "cuirass"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_heal  = ( strstr( s, "faceguard"      ) ||
                      strstr( s, "mantle"         ) ||
                      strstr( s, "tunic"          ) ||
                      strstr( s, "legwraps"       ) ||
                      strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "_gladiators_linked_"   ) )     return SET_PVP_MELEE;
  if ( strstr( s, "_gladiators_mail_"     ) )     return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_ringmail_" ) )     return SET_PVP_MELEE;

  return SET_NONE;
}

// shaman_t::primary_role ===================================================

role_e shaman_t::primary_role()
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( primary_tree() == SHAMAN_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( primary_tree() == SHAMAN_ENHANCEMENT )
    return ROLE_ATTACK;

  else if ( primary_tree() == SHAMAN_ELEMENTAL )
    return ROLE_SPELL;

  return player_t::primary_role();
}

#endif // SC_SHAMAN

} // ANONYMOUS NAMESPACE

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// class_modules::create::shaman  =================================================

player_t* class_modules::create::shaman( sim_t* sim, const std::string& name, race_e r )
{
  return sc_create_class<shaman_t,SC_SHAMAN>()( "Shaman", sim, name, r );
}

// player_t::shaman_init ====================================================

void class_modules::init::shaman( sim_t* sim )
{
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.bloodlust  = buff_creator_t( p, "bloodlust", p -> find_spell( 2825 ) )
                            .max_stack( 1 );

    p -> buffs.exhaustion = buff_creator_t( p, "exhaustion", p -> find_spell( 57723 ) )
                            .max_stack( 1 )
                            .quiet( true );

    p -> buffs.mana_tide  = buff_creator_t( p, "mana_tide", p -> find_spell( 16191 ) )
                            .duration( p -> find_spell( 16190 )-> duration() );
  }
}

// player_t::shaman_combat_begin ============================================

void class_modules::combat_begin::shaman( sim_t* )
{
}

void class_modules::combat_end::shaman( sim_t* )
{
}

