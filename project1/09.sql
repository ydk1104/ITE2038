SELECT (SELECT name from Trainer where owner_id = Trainer.id) AS name,AVG(level) from CatchedPokemon where owner_id in
(SELECT id from Trainer where id in (SELECT leader_id from Gym))
GROUP BY owner_id
ORDER BY name
