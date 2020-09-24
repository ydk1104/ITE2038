SELECT name, level, nickname From
(SELECT pid, level, nickname From CatchedPokemon Where owner_id in (SELECT leader_id From Gym) and nickname Like 'A%') As temp, Pokemon
Where pid = id
ORDER BY nickname DESC;
