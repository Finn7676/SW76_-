# Checkpoint 9: Expansion + AntiOscillation

Base: Version08_CoreOnly. Expansion and sector exploration ON; NavMemory OFF; AntiOscillation ON; Retargeting and Escape OFF. Emergency Split unchanged. No numeric parameters changed.

Important state dependency: the original nav_memory switch discarded all cross-turn navigation state, disabling anti-oscillation too. This candidate preserves observational state when AntiOscillation is enabled: head history, avoidance direction/deadline, and target-distance progress used by its existing stagnation trigger. NavMemory OFF disables previous-target goal injection and retained-target selection. Previous targets are recorded solely for observation; there is no preference for retaining them. This ablates target persistence in route selection, not all state storage. AntiOscillation thresholds and scoring remain unchanged.

Default matches: default and trophy, both sides versus Version07_frozen. Run bash run_checkpoint9.sh; reports winner, end round/reason and total wins/losses. No matches run during preparation. Existing versions remain unchanged.
