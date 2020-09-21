SELECT AVG(level) from CatchedPokemon where owner_id = (SELECT id from Trainer where name='Red')
