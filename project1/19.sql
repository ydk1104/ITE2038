SELECT COUNT(DISTINCT type) From CatchedPokemon, Pokemon Where
CatchedPokemon.pid = Pokemon.id and
owner_id in (SELECT leader_id From Gym Where city = 'Sangnok City');
