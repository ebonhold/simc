#!/usr/bin/env python3

import datetime

combat_log_times = list()

def calc_diff(t1, t2):
  t1 = datetime.datetime.strptime(t1, "%m/%d %H:%M:%S.%f")
  t2 = datetime.datetime.strptime(t2, "%m/%d %H:%M:%S.%f")
  print(t2 - t1)

with open("WoWCombatLog.txt", 'r') as f:
  for line in f:
    line = line.strip()
    if line == "":
      continue
    if "SPELL_ENERGIZE" in line:
      if "Deynda-Torghast" in line:
        if "Apocalypse" in line:
          combat_log_times.append(line[:line.find("  ")])

for i in range(len(combat_log_times)-1):
  calc_diff(combat_log_times[i], combat_log_times[i+1])


