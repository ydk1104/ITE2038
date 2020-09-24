SELECT name, COUNT(*) as cnt From CatchedPokemon, Trainer Where
CatchedPokemon.owner_id = Trainer.id and
owner_id in
(SELECT leader_id From Gym)
GROUP BY owner_id
ORDER BY name;
