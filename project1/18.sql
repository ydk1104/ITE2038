SELECT AVG(level) From CatchedPokemon Where owner_id in
(SELECT leader_id From Gym)
