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

# Executed before combat begins. Accepts non-harmful actions only.
actions.precombat=flask
actions.precombat+=/food
actions.precombat+=/augmentation
# Snapshot raid buffed stats before combat begins and pre-potting is done.
actions.precombat+=/snapshot_stats
actions.precombat+=/potion
actions.precombat+=/raise_dead
# actions.precombat+=/variable,name=other_on_use_equipped,value=

# Executed every time the actor is available.
actions=auto_attack
# Apply Frost Fever and maintain Icy Talons
actions+=/howling_blast,if=!dot.frost_fever.ticking&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
actions+=/glacial_advance,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&spell_targets.glacial_advance>=2&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
actions+=/frost_strike,if=buff.icy_talons.remains<=gcd&buff.icy_talons.up&(!talent.breath_of_sindragosa.enabled|cooldown.breath_of_sindragosa.remains>15)
actions+=/use_items
actions+=/potion,if=buff.pillar_of_frost.up&buff.empower_rune_weapon.up
# Action List Calls
actions+=/call_action_list,name=cooldowns
actions+=/call_action_list,name=cold_heart,if=talent.cold_heart.enabled&((buff.cold_heart.stack>=10&debuff.razorice.stack=5)|target.1.time_to_die<=gcd)
actions+=/run_action_list,name=bos_ticking,if=buff.breath_of_sindragosa.up
actions+=/run_action_list,name=bos_pooling,if=talent.breath_of_sindragosa.enabled&((cooldown.breath_of_sindragosa.remains=0&cooldown.pillar_of_frost.remains<10)|(cooldown.breath_of_sindragosa.remains<20&target.1.time_to_die<35))
actions+=/run_action_list,name=obliteration,if=buff.pillar_of_frost.up&talent.obliteration.enabled
actions+=/run_action_list,name=aoe,if=active_enemies>=2
actions+=/call_action_list,name=standard
# Racial Abilities
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
actions.aoe+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled&!talent.frostscythe.enabled
actions.aoe+=/frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled
actions.aoe+=/howling_blast,if=buff.rime.up
actions.aoe+=/frostscythe,if=buff.killing_machine.up
actions.aoe+=/glacial_advance,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)
actions.aoe+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit<(15+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/remorseless_winter
actions.aoe+=/frostscythe
actions.aoe+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>(25+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.aoe+=/obliterate,if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)
actions.aoe+=/glacial_advance
actions.aoe+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled
actions.aoe+=/frost_strike
actions.aoe+=/horn_of_winter
actions.aoe+=/arcane_torrent

# Breath of Sindragosa pooling rotation : starts 20s before Pillar of Frost + BoS are available
actions.bos_pooling=howling_blast,if=buff.rime.up
actions.bos_pooling+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&&runic_power.deficit>=25&!talent.frostscythe.enabled
actions.bos_pooling+=/obliterate,if=runic_power.deficit>=25
actions.bos_pooling+=/glacial_advance,if=runic_power.deficit<20&spell_targets.glacial_advance>=2&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit<20&!talent.frostscythe.enabled&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frost_strike,if=runic_power.deficit<20&cooldown.pillar_of_frost.remains>5
actions.bos_pooling+=/frostscythe,if=buff.killing_machine.up&runic_power.deficit>(15+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2
actions.bos_pooling+=/frostscythe,if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&spell_targets.frostscythe>=2
actions.bos_pooling+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)&!talent.frostscythe.enabled
actions.bos_pooling+=/obliterate,if=runic_power.deficit>=(35+talent.runic_attenuation.enabled*3)
actions.bos_pooling+=/glacial_advance,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&spell_targets.glacial_advance>=2
actions.bos_pooling+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40&!talent.frostscythe.enabled
actions.bos_pooling+=/frost_strike,if=cooldown.pillar_of_frost.remains>rune.time_to_4&runic_power.deficit<40

# Breath of Sindragosa Ticking Rotation
actions.bos_ticking=obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power<=32&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=runic_power<=32
actions.bos_ticking+=/remorseless_winter,if=talent.gathering_storm.enabled
actions.bos_ticking+=/howling_blast,if=buff.rime.up
actions.bos_ticking+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&rune.time_to_5<gcd|runic_power<=45&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=rune.time_to_5<gcd|runic_power<=45
actions.bos_ticking+=/frostscythe,if=buff.killing_machine.up&spell_targets.frostscythe>=2
actions.bos_ticking+=/horn_of_winter,if=runic_power.deficit>=32&rune.time_to_3>gcd
actions.bos_ticking+=/remorseless_winter
actions.bos_ticking+=/frostscythe,if=spell_targets.frostscythe>=2
actions.bos_ticking+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&runic_power.deficit>25|rune>3&!talent.frostscythe.enabled
actions.bos_ticking+=/obliterate,if=runic_power.deficit>25|rune>3
actions.bos_ticking+=/arcane_torrent,if=runic_power.deficit>50

# Cold heart conditions
actions.cold_heart=chains_of_ice,if=buff.cold_heart.stack>5&target.1.time_to_die<gcd|buff.pillar_of_frost.remains<3


# Frost cooldowns
actions.cooldowns=pillar_of_frost,if=(cooldown.empower_rune_weapon.remains|talent.icecap.enabled)&!buff.pillar_of_frost.up
actions.cooldowns+=/breath_of_sindragosa,use_off_gcd=1,if=cooldown.empower_rune_weapon.remains&cooldown.pillar_of_frost.remains
actions.cooldowns+=/empower_rune_weapon,if=cooldown.pillar_of_frost.ready&talent.obliteration.enabled&rune.time_to_5>gcd&runic_power.deficit>=10|target.1.time_to_die<20
actions.cooldowns+=/empower_rune_weapon,if=(cooldown.pillar_of_frost.ready|target.1.time_to_die<20)&talent.breath_of_sindragosa.enabled&runic_power>60
actions.cooldowns+=/empower_rune_weapon,if=talent.icecap.enabled&rune<3
actions.cooldowns+=/frostwyrms_fury,if=buff.pillar_of_frost.remains<(3+talent.cold_heart.enabled*1)
actions.cooldowns+=/frostwyrms_fury,if=active_enemies>=2&cooldown.pillar_of_frost.remains+15>target.time_to_die|target.1.time_to_die<gcd
actions.cooldowns+=/raise_dead
#actions.cooldowns+=/sacrificial_pact,if=(buff.pillar_of_frost.up&buff.pillar_of_frost.remains=<1|cooldown.raise_dead.remains<63)&pet.risen_ghoul.active


# Obliteration rotation
actions.obliteration=remorseless_winter,if=talent.gathering_storm.enabled
actions.obliteration+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3
actions.obliteration+=/obliterate,if=!talent.frostscythe.enabled&!buff.rime.up&spell_targets.howling_blast>=3
actions.obliteration+=/frostscythe,if=(buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance)))&spell_targets.frostscythe>=2
actions.obliteration+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))
actions.obliteration+=/obliterate,if=buff.killing_machine.react|(buff.killing_machine.up&(prev_gcd.1.frost_strike|prev_gcd.1.howling_blast|prev_gcd.1.glacial_advance))
actions.obliteration+=/glacial_advance,if=(!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd)&spell_targets.glacial_advance>=2
actions.obliteration+=/howling_blast,if=buff.rime.up&spell_targets.howling_blast>=2
actions.obliteration+=/frost_strike,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd&!talent.frostscythe.enabled
actions.obliteration+=/frost_strike,if=!buff.rime.up|runic_power.deficit<10|rune.time_to_2>gcd
actions.obliteration+=/howling_blast,if=buff.rime.up
actions.obliteration+=/obliterate,target_if=(debuff.razorice.stack<5|debuff.razorice.remains<10)&!talent.frostscythe.enabled
actions.obliteration+=/obliterate

# Standard single-target rotation
actions.standard=remorseless_winter
actions.standard+=/frost_strike,if=cooldown.remorseless_winter.remains<=2*gcd&talent.gathering_storm.enabled
actions.standard+=/howling_blast,if=buff.rime.up
actions.standard+=/obliterate,if=!buff.frozen_pulse.up&talent.frozen_pulse.enabled
actions.standard+=/frost_strike,if=runic_power.deficit<(15+talent.runic_attenuation.enabled*3)
actions.standard+=/frostscythe,if=buff.killing_machine.up&rune.time_to_4>=gcd
actions.standard+=/obliterate,if=runic_power.deficit>(25+talent.runic_attenuation.enabled*3)
actions.standard+=/frost_strike
actions.standard+=/horn_of_winter
actions.standard+=/arcane_torrent

head=,id=178777,bonus_id=1500
neck=,id=178827,bonus_id=1500
shoulders=,id=178820,bonus_id=1500
back=,id=178851,bonus_id=1500
chest=,id=180099,bonus_id=1500
wrists=,id=179354,bonus_id=1500
hands=,id=178840,bonus_id=1500
waist=,id=178842,bonus_id=1500
legs=,id=178818,bonus_id=1500
feet=,id=180101,bonus_id=1500
finger1=,id=178933,bonus_id=1500/6946
finger2=,id=178736,bonus_id=1500
trinket1=,id=178769,bonus_id=1500
trinket2=,id=179342,bonus_id=1500
main_hand=,id=178780,bonus_id=1500,enchant=rune_of_the_fallen_crusader
scale_to_itemlevel=187
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
            output += "{}+=finger1=,id=178933,bonus_id=1500/{}\n".format(profileset_name, legendary_id)
            if weapon_type == "dw":
                output += "{}+=main_hand=,id=178730,ilevel=187,enchant=rune_of_razorice\n".format(profileset_name)
                output += "{}+=off_hand=,id=179340,ilevel=187,enchant=rune_of_the_fallen_crusader\n".format(profileset_name)
            else:
                output += "{}+=main_hand=,id=178780,bonus_id=1500,enchant=rune_of_the_fallen_crusader\n".format(profileset_name)
                
            output += "{}+=soulbind={},{}\n".format(profileset_name, soulbind, temp_sb)
    return output

def output_ilvl(ilvl):
    profile_suffix = "test"
    output = ""
    output += "profileset.2h_baseline=talents=1221212\n"
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("oblit", "1221212", "2h", legendary_id, covenant, soulbind, 1)

    output += "\n"

    output += "profileset.2h_fsc_baseline=talents=1223212\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("fsc", "1223212", "2h", legendary_id, covenant, soulbind, 1)
    output += "\n"

    output += "profileset.2h_bos_baseline=talents=2223213\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("bos", "2223213", "2h", legendary_id, covenant, soulbind, 1)
    output += "\n"

    output += "profileset.dw_baseline=talents=1221212\n".format(profile_suffix)
    output += "profileset.dw_baseline+=main_hand=,id=178730,ilevel=187,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_baseline+=off_hand=,id=179340,ilevel=187,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("oblit", "1221212", "dw", legendary_id, covenant, soulbind, 1)
    output += "\n"

    output += "profileset.dw_fsc_baseline=talents=1223212\n".format(profile_suffix)
    output += "profileset.dw_fsc_baseline+=main_hand=,id=178730,ilevel=187,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_fsc_baseline+=off_hand=,id=179340,ilevel=187,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("fsc", "1223212", "dw", legendary_id, covenant, soulbind, 1)
    output += "\n"

    output += "profileset.dw_bos_baseline=talents=2223213\n".format(profile_suffix)
    output += "profileset.dw_bos_baseline+=main_hand=,id=178730,ilevel=187,enchant=rune_of_razorice\n".format(profile_suffix)
    output += "profileset.dw_bos_baseline+=off_hand=,id=179340,ilevel=187,enchant=rune_of_the_fallen_crusader\n".format(profile_suffix)
    for covenant in covenants:
        for soulbind in soulbind_data[covenant]:
            for legendary_id in legendary_data:
                output += generate_profileset("bos", "2223213", "dw", legendary_id, covenant, soulbind, 1)
    output += "\n"

    return output


with open('full_profile.simc', 'w') as f:
    f.write(output)

    for x in [187]:
        f.write(output_ilvl(x))

#print(generate_soulbind("profileset.2h", "night_fae", "niya", 1))

