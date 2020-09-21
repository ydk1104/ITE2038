SELECT name, pid From CatchedPokemon, Pokemon Where
pid = Pokemon.id and
owner_id in (SELECT id From Trainer Where hometown='Sangnok City')
ORDER BY pid
