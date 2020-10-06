#!/usr/bin/env python3

import itertools

output = """
deathknight="PR_Death_Knight_Frost_2H"
source=default
spec=frost
level=60
race=orc
role=attack
position=back
talents=1221212

# This default action priority list is automatically created based on your character.
# It is a attempt to provide you with a action list that is both simple and practicable,
# while resulting in a meaningful and good simulation. It may not result in the absolutely highest possible dps.
# Feel free to edit, adapt and improve it to your own needs.
# SimulationCraft is always looking for updates and improvements to the default action lists.

# Executed before combat begins. Accepts non-harmful actions only.
actions.precombat=flask
actions.precombat+=/food
actions.precombat+=/augmentation
# Snapshot raid buffed stats before combat begins and pre-potting is done.
actions.precombat+=/snapshot_stats
actions.precombat+=/potion

# Executed every time the actor is available.
actions=auto_attack
# Apply Frost Fever and maintain Icy Talons
actions+=/howling_blast,if=!dot.frost_fever.ticking&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
actions+=/glacial_advance,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&spell_targets.glacial_advance>=2&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
actions+=/frost_strike,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
# Action List Calls
actions+=/call_action_list,name=cooldowns
actions+=/call_action_list,name=cold_heart,if=talent.cold_heart.enabled&(buff.cold_heart.stack>=10&(debuff.razorice.stack=5&death_knight.runeforge.razorice|!death_knight.runeforge.razorice)|target.1.time_to_die<=gcd)
actions+=/run_action_list,name=bos_ticking,if=buff.breath_of_sindragosa.up
actions+=/run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa.enabled&((cooldown.breath_of_sindragosa.remains=0&cooldown.pillar_of_frost.remains<10)|(cooldown.breath_of_sindragosa.remains<20&target.1.time_to_die<35))
actions+=/run_action_list,name=obliteration,if=buff.pillar_of_frost.up&talent.obliteration.enabled
actions+=/run_action_list,name=aoe,if=active_enemies>=2
actions+=/call_action_list,name=standard
actions+=/potion,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up
# Racial Abilties
actions+=/blood_fury,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up
actions+=/berserking,if=buff.pillar_of_frost.up
actions+=/arcane_pulse,if=(!buff.pillar_of_frost.up&active_enemies>=2)|!buff.pillar_of_frost.up&(rune.deficit>=5&runic_power.deficit>=60)
actions+=/lights_judgment,if=buff.pillar_of_frost.up
actions+=/ancestral_call,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up
actions+=/fireblood,if=buff.pillar_of_frost.remains<=8&buff.empower_rune_weapon.up
actions+=/bag_of_tricks,if=buff.pillar_of_frost.up&(buff.pillar_of_frost.remains<5&talent.cold_heart.enabled|!talent.cold_heart.enabled&buff.pillar_of_frost.remains<3)&active_enemies=1|buff.seething_rage.up&active_enemies=1

# AoE Rotation
actions.aoe=remorseless_winter,if=talent.gathering_storm.enabled
actions.aoe+=/glacial_advance,if=talent.frostscythe.enabled
actions.aoe+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled&!talent.frostscythe.enabled
actions.aoe+=/frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled
actions.aoe+=/howling_blast,if=buff.rime.up
actions.aoe+=/frostscythe,if=buff.killing_machine.up
actions.aoe+=/glacial_advance,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)
actions.aoe+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit<(15+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/remorseless_winter
actions.aoe+=/frostscythe
actions.aoe+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit>(25+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/obliterate,if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)
actions.aoe+=/glacial_advance
actions.aoe+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&!talent.frostscythe.enabled
actions.aoe+=/frost_strike
actions.aoe+=/horn_of_winter
actions.aoe+=/arcane_torrent

# Breath of Sindragosa pooling rotation : starts 20s before Pillar of Frost + BoS are available
actions.bos_pooling=howling_blast,if=buff.rime.up
actions.bos_pooling+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit>=25&!talent.frostscythe.enabled
actions.bos_pooling+=/obliterate,if=runic_power.deficit>=25
actions.bos_pooling+=/glacial_advance,if=runic_power.deficit<20&spell_targets.glacial_advance>=2&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit<20&!talent.frostscythe.enabled&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frost_strike,if=runic_power.deficit<20&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frostscythe,if=buff.killing_machine.up&runic_power.deficit>(15+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2
actions.bos_pooling+=/frostscythe,if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2
actions.bos_pooling+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.bos_pooling+=/obliterate,if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)
actions.bos_pooling+=/glacial_advance,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&spell_targets.glacial_advance>=2
actions.bos_pooling+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&!talent.frostscythe.enabled
actions.bos_pooling+=/frost_strike,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40

actions.bos_ticking=obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power<=32&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=runic_power<=32
actions.bos_ticking+=/remorseless_winter,if=talent.gathering_storm.enabled
actions.bos_ticking+=/howling_blast,if=buff.rime.up
actions.bos_ticking+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&rune.time_to_5<gcd|runic_power<=45&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=rune.time_to_5<gcd|runic_power<=45
actions.bos_ticking+=/frostscythe,if=buff.killing_machine.up&spell_targets.frostscythe>=2
actions.bos_ticking+=/horn_of_winter,if=runic_power.deficit>=32&rune.time_to_3>gcd
actions.bos_ticking+=/remorseless_winter
actions.bos_ticking+=/frostscythe,if=spell_targets.frostscythe>=2
actions.bos_ticking+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&runic_power.deficit>25|rune>3&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=runic_power.deficit>25|rune>3
actions.bos_ticking+=/arcane_torrent,if=runic_power.deficit>50

# Cold heart conditions
actions.cold_heart=chains_of_ice,if=target.1.time_to_die<gcd|buff.pillar_of_frost.remains<3&buff.cold_heart.stack=20

# Frost cooldowns
actions.cooldowns=empower_rune_weapon,if=talent.obliteration.enabled&(cooldown.pillar_of_frost.ready&rune.time_to_5>gcd&runic_power.deficit>=10|buff.pillar_of_frost.up&rune.time_to_5>gcd)|target.1.time_to_die<20
actions.cooldowns+=/empower_rune_weapon,if=(buff.breath_of_sindragosa.up|target.1.time_to_die<20)&talent.breath_of_sindragosa.enabled&runic_power.deficit>30
actions.cooldowns+=/empower_rune_weapon,if=talent.icecap.enabled&rune<3
actions.cooldowns+=/pillar_of_frost,use_off_gcd=1,if=talent.breath_of_sindragosa.enabled&cooldown.breath_of_sindragosa.remains|!talent.breath_of_sindragosa.enabled
actions.cooldowns+=/breath_of_sindragosa,use_off_gcd=1,if=cooldown.pillar_of_frost.ready&runic_power.deficit<60
actions.cooldowns+=/frostwyrms_fury,if=buff.pillar_of_frost.remains<(3+talent.cold_heart.enabled*1)
actions.cooldowns+=/frostwyrms_fury,if=active_enemies>=2&cooldown.pillar_of_frost.remains+15>target.time_to_die|target.1.time_to_die<gcd
actions.cooldowns+=/hypothermic_presence,if=talent.breath_of_sindragosa.enabled&runic_power.defecit>40&rune>=3&cooldown.pillar_of_frost.up|!talent.breath_of_sindragosa.enabled&runic_power.deficit>=25
actions.cooldowns+=/raise_dead,if=cooldown.pillar_of_frost.remains<3|buff.pillar_of_frost.up&!talent.breath_of_sindragosa.enabled
actions.cooldowns+=/sacrificial_pact,if=pet.ghoul.remains<gcd&active_enemies>=2

# Obliteration rotation
actions.obliteration=remorseless_winter,if=talent.gathering_storm.enabled
actions.obliteration+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3
actions.obliteration+=/obliterate,if=!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3
actions.obliteration+=/frostscythe,if=(buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance)))&spell_targets.frostscythe>=2
actions.obliteration+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))
actions.obliteration+=/obliterate,if=buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))
actions.obliteration+=/glacial_advance,if=(!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd)&spell_targets.glacial_advance>=2|!death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<15)
actions.obliteration+=/frost_strike,if=talent.icy_talons.enabled&buff.icy_talons.remains<gcd|conduit.eradicating_blow.enabled&buff.eradicating_blow.stack=2
actions.obliteration+=/howling_blast,if=buff.rime.up&spell_targets.howling_blast>=2
actions.obliteration+=/frost_strike,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd&!talent.frostscythe.enabled
actions.obliteration+=/frost_strike,if=!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd
actions.obliteration+=/howling_blast,if=buff.rime.up
actions.obliteration+=/obliterate,target_if=(death_knight.runeforge.razorice&(debuff.razorice.stack<5|debuff.razorice.remains<10))&!talent.frostscythe.enabled
actions.obliteration+=/obliterate

# Standard single-target rotation
actions.standard=remorseless_winter
actions.standard+=/frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled|conduit.unleashed_frenzy.enabled&buff.unleashed_frenzy.remains<3
actions.standard+=/glacial_advance,if=!death_knight.runeforge.razorice
actions.standard+=/howling_blast,if=buff.rime.up
actions.standard+=/obliterate,if=!buff.frozen_pulse.up&talent.frozen_pulse.enabled
actions.standard+=/frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)
actions.standard+=/frostscythe,if=buff.killing_machine.up&rune.time_to_4>=gcd
actions.standard+=/obliterate,if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)
actions.standard+=/frost_strike
actions.standard+=/horn_of_winter
actions.standard+=/arcane_torrent

#  1 Helm drop
head=diadem_of_imperious_desire,id=182997,ilevel=233,gems=16crit,enchant=shadowcore_oil

# C/M - Nik
neck=nobles_birthstone_pendant,id=183039,ilevel=226,gems=16crit
# H/M - Hunts
# neck=charm_of_eternal_winter,id=183040,ilevel=226

#  1 Shoulder drop
shoulder=epaulettes_of_overwhelming_force,id=182994,ilevel=226

# C/H Chest - Sludge
chest=rampaging_giants_chestplate,id=182999,ilevel=226,enchant=eternal_stats
# C/M Chest - Xy
# chest=breatplate_of_cautious_calculation,id=182987,ilevel=226,enchant=eternal_stats

# H/V - Kaal
back=crest_of_the_legionnaire_general,id=183032,ilevel=226,enchant=soul_vitality
# V/M - Shriek
# back=cowled_batwing_cloak,id=183034,ilevel=226,enchant=soul_vitality
# C/M - Kael
# back=mantle_of_mantifest_sins,id=183033,ilevel=226,enchant=soul_vitality

#  1 Wrist drop
wrists=hellhound_cuffs,id=183018,ilevel=226,gems=16crit

#  1 Hands drop
hands=colossal_plate_gauntlets,id=182984,ilevel=226,enchant=eternal_strength

#  C/H - Inerva
waist=binding_of_warped_desires,id=183015,ilevel=226,gems=16crit
#  H/M - Kael
# waist=stoic_guardsmans_belt,id=183025,ilevel=226

#  V/M - Kaal
legs=ceremonial_parade_legguards,id=183002,ilevel=233
#  C/M - Hungering
# legs=endlessly_gluttonous_greaves,id=182992,ilevel=226

#  H/M - Shriek
feet=errant_crusaders_greaves,id=183027,ilevel=226
#  C/M - Niklaus
# feet=stoneguard_attendants_boots,id=182983,ilevel=226

#  H/M - Sire
finger1=most_regal_signet_of_sire_denathrius,id=183036,ilevel=233,gems=16crit,enchant=tenet_of_critical_strike
#  C/H - Inerva
finger2=ritualists_treasured_ring,id=183037,ilevel=226,gems=16crit,enchant=tenet_of_critical_strike
#  V/M - Xy
# finger2=hyperlight_band,id=183038,ilevel=226,enchant=tenet_of_critical_strike

#  Trinket section is huge, sorry

#  Sire
trinket1=sanguine_vintage,id=184031,ilevel=233
#  Sire
trinket2=dreadfire_vessel,id=184030,ilevel=233
#  Kaal
# trinket1=stone_legion_heraldry,id=184027,ilevel=233
#  Niklaus
# trinket1=macabre_sheet_music,id=184024,ilevel=233
# Inerva
# trinket1=memory_of_past_sins,id=184025,ilevel=226
# Shriek
# trinket1=skulkers_wing,id=184016,ilevel=226
# Hungering
# trinket1=gluttonous_spike,id=184023,ilevel=226
# Huntsman
# trinket1=bargasts_leash,id=184017,ilevel=226
# Kael
# trinket1=splintered_heart_of_alar,id=184018,ilevel=226

# 2H
#  H/M
main_hand=nathrian_crusaders_bastard_sword,id=182389,ilevel=233,enchant=rune_of_razorice
#  C/V 
# main_hand=nathrian_crusaders_blade,id=182415,ilevel=233,enchant=rune_of_razorice

# DW
# C/M for both
# main_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_razorice
# off_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_the_fallen_crusader
# H/V Alternate
# main_hand=stoneborn_terrorblade,id=182395,ilevel=233,enchant=rune_of_razorice

tabard=,id=31405

flask=spectral_flask_of_power
food=feast_of_gluttonous_hedonism
potion=spectral_strength
"""

legendary_data = {'6946': 'absolute_zero',
                    '6945': 'biting_cold',
                    '6944': 'koltira',
                    '7160': 'rage'}

conduit_pot = { '79': 'accelerated_cold',
                '91': 'biting_cold',
                '83': 'eradicating_blow',
                '122': 'unleashed_frenzy'}

covenants = ["night_fae", "venthyr", "necrolord", "kyrian"]

soulbind_data = {
    "night_fae": {
        "niya": [
            "P/320659/P/322721",
            "P/320660/P/P/322721",
            "P/320662/P/322721"
        ],
        "dreamweaver": [
            "319191/P/319210/P",
            "319191/P/319211/P/P",
            "319191/P/319213/P"
        ],
        "korayn": [
            "P/325068/P/P/325066",
            "P/325069/P/325066",
            "P/325601/P/325066"
        ]
    },
    "venthyr": {
        "nadjia": [
            "P/331580/P/331586",
            "P/P/331582/P/331586",
            "P/331584/P/331586"
        ],
        "theotar": [
            "P/P/336243/319983",
            "P/P/336239/319983",
            "P/P/336245/P/319983"
        ],
        "draven": [
            #  Draven is all sorts of busted atm
            #"319973/P/P/340159",
            #  Superior Tactics currently causes an assert issue
            #"332753/P/P/340159",
            #  Hold your ground gets auto created, but is missing an invalidate for the cache
            #"332754/P/P/P/340159",
            #  This one below should be 332754, but we have the path commented out for now
            #"P/P/P/340159"
        ]
    },
    "necrolord": {
        "marileth": [
            "323074/P/P/P",
            "323074/323090/P/P"
        ],
        "emeni": [
            "P/P/P/342156",
            "P/P/323919/342156",
            "P/P/323916/342156"
        ],
        "heirmir": [
            "326504/P/326509",
            "326504/P/326511/P",
            "326504/P/326572",
            "P/P/326509/",
            "P/P/326511/P",
            "P/P/326572"
        ]
    },
    "kyrian": {
        "pelagos": [
            "328257/P/P/328266",
            "328257/P/P/P/328266"
        ],
        "kleia": [
            "P/P/P",
            "P/P/329778"
        ],
        "mikanikos": [
            "P/333935/P/333950",
            "P/P/333950",
            "P/P/P/333950"
        ]
    }
}

#  Venthyr
nadjia = [

]

def generate_profileset(talent_label, talents, weapon_type, legendary_id, covenant, soulbind, rank):
    output = ""

    for sb in soulbind_data[covenant][soulbind]:
        num_conduits = sb.count("P")
        for conduits in itertools.combinations(conduit_pot.keys(), num_conduits):
            temp_sb = sb
            profile_conduits = ""
            for z in conduits:
                temp_sb = temp_sb.replace("P", "{}:{}".format(z, rank), 1)
                profile_conduits += "_{}".format(conduit_pot[z])
            profileset_name = "profileset.{}_{}_{}_{}{}_{}".format(weapon_type, talent_label, legendary_data[legendary_id], soulbind, profile_conduits, temp_sb)
            output += "{}=talents={}\n".format(profileset_name, talents)
            output += "{}+=covenant={}\n".format(profileset_name,  covenant)
            output += "{}+=tabard=,id=31405,bonus_id={}\n".format(profileset_name, legendary_id)
            if weapon_type == "dw":
                output += "{}+=main_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_razorice\n".format(profileset_name)
                output += "{}+=off_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_the_fallen_crusader\n".format(profileset_name)
            else:
                output += "{}+=main_hand=nathrian_crusaders_bastard_sword,id=182389,ilevel=233,enchant=rune_of_razorice\n".format(profileset_name)
                
            output += "{}+=soulbind={},{}\n".format(profileset_name, soulbind, temp_sb)
    return output

def output_ilvl(ilvl):
    profile_suffix = "test"
    output = ""
    output += "profileset.2h_baseline=talents=1221212\n"
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("oblit", "1221212", "2h", legendary_id, covenant, soulbind, 6)

    output += "\n"

    output += "profileset.2h_fsc_baseline=talents=1223212\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("fsc", "1223212", "2h", legendary_id, covenant, soulbind, 6)
    output += "\n"

    output += "profileset.2h_bos_baseline=talents=2223213\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("bos", "2223213", "2h", legendary_id, covenant, soulbind, 6)
    output += "\n"

    output += "profileset.dw_baseline=talents=1221212\n".format(profile_suffix)
    output += "profileset.dw_baseline+=main_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_baseline+=off_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("oblit", "1221212", "dw", legendary_id, covenant, soulbind, 6)
    output += "\n"

    output += "profileset.dw_fsc_baseline=talents=1223212\n".format(profile_suffix)
    output += "profileset.dw_fsc_baseline+=main_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_fsc_baseline+=off_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("fsc", "1223212", "dw", legendary_id, covenant, soulbind, 6)
    output += "\n"

    output += "profileset.dw_bos_baseline=talents=2223213\n".format(profile_suffix)
    output += "profileset.dw_bos_baseline+=main_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_bos_baseline+=off_hand=barbededge_of_the_stone_legion,id=182421,ilevel=233,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("bos", "2223213", "dw", legendary_id, covenant, soulbind, 6)
    output += "\n"

    return output


with open('full_profile.simc', 'w') as f:
    f.write(output)

    for x in [187]:
        f.write(output_ilvl(x))

#print(generate_soulbind("profileset.2h", "night_fae", "niya", 1))

