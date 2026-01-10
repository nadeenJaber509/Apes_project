# Fix Plan for Ape Simulation Issues

## Issue 1: Negative Energy is Unrealistic
- **Files to modify:** female_ape.c, male_ape.c
- **Changes:**
  - Add energy floor (minimum 0) after each energy deduction
  - Ensure energy never goes below 0 in all energy deduction points

## Issue 2: Endgame Behavior Wastes CPU
- **Files to modify:** simulation.c, female_ape.c, male_ape.c, baby_ape.c
- **Changes:**
  - Add more frequent termination checks in main loops
  - Add early exit conditions when simulation is stopping
  - Use more efficient waiting patterns
  - Reduce unnecessary work when near end

## Issue 3: Baby Tracking Inconsistencies
- **Files to modify:** baby_ape.c
- **Changes:**
  - Fix `calculate_distance_to_basket()` to use basket position
  - Fix `dist_from_dad` calculation to use dad's actual position
  - Add consistent family tracking throughout

## Execution Order
1. Fix Issue 1 (Negative Energy) in female_ape.c and male_ape.c
2. Fix Issue 2 (Endgame CPU Waste) in simulation files
3. Fix Issue 3 (Baby Tracking) in baby_ape.c
4. Rebuild and test

