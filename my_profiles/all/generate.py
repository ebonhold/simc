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
            "P/320659/322721",
            "P/320660/P/322721",
            "P/320662/322721"
        ],
        "dreamweaver": [
            "319191/P/319210",
            "319191/P/319211/P",
            "319191/P/319213"
        ],
        "korayn": [
            "P/325068/P/325066",
            "P/325069/325066",
            "P/325601/325066"
        ]
    },
    "venthyr": {
        "nadjia": [
            "P/331580/331586",
            "P/P/331582/331586",
            "P/331584/331586"
        ],
        "theotar": [
            "P/336243/319983",
            "P/336239/319983",
            "P/336245/P/319983"
        ],
        "draven": [
            #  Draven is all sorts of busted atm
            "319973/P/340159",
            #  Superior Tactics currently causes an assert issue
            "332753/P/340159",
            #  Hold your ground gets auto created, but is missing an invalidate for the cache
            "332754/P/P/340159",
            #  This one below should be 332754, but we have the path commented out for now
            "P/P/P/340159"
        ]
    },
    "necrolord": {
        "marileth": [
            "323074/P/P",
            "323074/323090/P"
        ],
        "emeni": [
            "P/P/342156",
            "P/323919/342156",
            "P/323916/342156"
        ],
        "heirmir": [
            "326504/326509",
            "326504/326511/P",
            "326504/326572",
            "P/326509/",
            "P/326511/P",
            "P/326572"
        ]
    },
    "kyrian": {
        "pelagos": [
            "328257/P/328266",
            "328257/P/P/328266"
        ],
        "kleia": [
            "P/P/329791",
            "P/329778/329791B"
        ],
        "mikanikos": [
            "P/333935/333950",
            "P/P/333950"
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

